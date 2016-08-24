// s_only.cpp

/*    Copyright 2009 10gen Inc.
 *
 *    This program is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the GNU Affero General Public License in all respects
 *    for all of the code used other than as permitted herein. If you modify
 *    file(s) with this exception, you may extend this exception to your
 *    version of the file(s), but you are not obligated to do so. If you do not
 *    wish to do so, delete this exception statement from your version. If you
 *    delete this exception statement from all source files in the program,
 *    then also delete it in the license file.
 */

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kSharding

#include "mongo/platform/basic.h"

#include <tuple>

#include "mongo/db/auth/authorization_manager.h"
#include "mongo/db/auth/authorization_manager_global.h"
#include "mongo/db/auth/authorization_session.h"
#include "mongo/db/client.h"
#include "mongo/db/commands.h"
#include "mongo/db/service_context.h"
#include "mongo/db/stats/counters.h"
#include "mongo/rpc/metadata.h"
#include "mongo/rpc/reply_builder_interface.h"
#include "mongo/rpc/request_interface.h"
#include "mongo/s/cluster_last_error_info.h"
#include "mongo/util/assert_util.h"
#include "mongo/util/concurrency/thread_name.h"
#include "mongo/util/log.h"
#include "mongo/s/grid.h"
#include "mongo/s/catalog/catalog_cache.h"
#include "mongo/s/config.h"

#include "mongo/db/query/find_common.h"

namespace mongo {

using std::string;
using std::stringstream;


bool isMongos() {
    return true;
}

/** When this callback is run, we record a shard that we've used for useful work
 *  in an operation to be read later by getLastError()
*/
void usingAShardConnection(const std::string& addr) {
    ClusterLastErrorInfo::get(cc()).addShardHost(addr);
}

// called into by the web server. For now we just translate the parameters
// to their old style equivalents.
void Command::execCommand(OperationContext* txn,
                          Command* command,
                          const rpc::RequestInterface& request,
                          rpc::ReplyBuilderInterface* replyBuilder) {
    int queryFlags = 0;
    BSONObj cmdObj;

    std::tie(cmdObj, queryFlags) = uassertStatusOK(
        rpc::downconvertRequestMetadata(request.getCommandArgs(), request.getMetadata()));

    std::string db = request.getDatabase().rawData();
    BSONObjBuilder result;

    execCommandClientBasic(txn,
                           command,
                           *txn->getClient(),
                           queryFlags,
                           request.getDatabase().rawData(),
                           cmdObj,
                           result);

    replyBuilder->setCommandReply(result.done()).setMetadata(rpc::makeEmptyMetadata());
}

void Command::execCommandClientBasic(OperationContext* txn,
                                     Command* c,
                                     ClientBasic& client,
                                     int queryOptions,
                                     const char* ns,
                                     BSONObj& cmdObj,
                                     BSONObjBuilder& result) {
    std::string dbname = nsToDatabase(ns);

    if (cmdObj.getBoolField("help")) {
        stringstream help;
        help << "help for: " << c->name << " ";
        c->help(help);
        result.append("help", help.str());
        result.append("lockType", c->isWriteCommandForConfigServer() ? 1 : 0);
        appendCommandStatus(result, true, "");
        return;
    }

    Status status = _checkAuthorization(c, &client, dbname, cmdObj);
    if (!status.isOK()) {
        appendCommandStatus(result, status);
        return;
    }

    c->_commandsExecuted.increment();

    if (c->shouldAffectCommandCounter()) {
        globalOpCounters.gotCommand();
    }

    std::string errmsg;
    bool ok = false;
    try {
        ok = c->run(txn, dbname, cmdObj, queryOptions, errmsg, result);
    } catch (const DBException& e) {
        result.resetToEmpty();
        const int code = e.getCode();

        // Codes for StaleConfigException
        if (code == ErrorCodes::RecvStaleConfig || code == ErrorCodes::SendStaleConfig) {
            throw;
        }

        errmsg = e.what();
        result.append("code", code);
    }

    if (!ok) {
        c->_commandsFailed.increment();
    }

    appendCommandStatus(result, ok, errmsg);
}

void Command::runAgainstRegistered(OperationContext* txn,
                                   const char* ns,
                                   BSONObj& jsobj,
                                   BSONObjBuilder& anObjBuilder,
                                   int queryOptions) {
    // It should be impossible for this uassert to fail since there should be no way to get
    // into this function with any other collection name.
    uassert(16618,
            "Illegal attempt to run a command against a namespace other than $cmd.",
            nsToCollectionSubstring(ns) == "$cmd");
   
    std::string dbname = nsToDatabase(ns);
    BSONElement e = jsobj.firstElement();
    std::string commandName = e.fieldName();
    BSONObj query_condition;

    //log()<<"collname in runAgainstRegistered: "<<collName;
    //auto status = grid.catalogCache()->getDatabase(txn, dbname);
    auto status = grid.implicitCreateDb(txn, dbname);
    uassertStatusOK(status.getStatus());
    std::shared_ptr<DBConfig> conf = status.getValue();
    BSONObjBuilder bdr;
    bool is_rtree = false;
    if (commandName == "insert" || commandName == "delete")
    {
        std::string collName = e.String();
        bdr.append("NAMESPACE", dbname+"."+collName);
        is_rtree  = conf->checkRtreeExist(txn,bdr.obj());
        //log()<<"is rtree?:"<< is_rtree; 
    }
    if (commandName == "insert"&&is_rtree)
    {
        commandName = "insertIndexedDoc";
    }
    if (commandName == "delete"&&is_rtree)
    {
        BSONObj deleteObj = jsobj["deletes"].Array()[0].Obj();
        BSONObj queryObj = deleteObj["q"].Obj();
        if (queryObj.hasField("id"))
            commandName = "deleteGeoByID";
        else if ("Polygon" == queryObj["type"].str()||"MultiPolygon" == queryObj["type"].str())
        {
            std::string collName = e.String();
            commandName = "deleteContainedGeoObj";
            BSONObjBuilder cmdObj;
            cmdObj.append("collection",collName);
            cmdObj.append("condition",queryObj);
            jsobj = cmdObj.obj();
            log()<<"check delete cmd:"<<jsobj;
        }       
    }
    if(commandName == "find")
    {
        std::string collName = e.String();
        string columnName;
        //log() << "ns:" << q.ns << ", collname: " << collName << endl;
        BSONObjBuilder bdr;

        bool is_rtree = false;
  
        bool is_registered = false;

        bool is_command_geowithin = false;
        
        bool is_command_geointersects = false;

        bool is_command_geonear = false;

        bool is_type_polygon = false;
    
        bool is_type_point = false;

        bdr.append("NAMESPACE", dbname+"."+collName);
        auto database_status = grid.catalogCache()->getDatabase(txn, dbname);
        uassertStatusOK(database_status.getStatus());
        std::shared_ptr<DBConfig> conf = database_status.getValue();
        BSONObj geometadata = conf->getGeometry(txn,bdr.obj());
        if (!geometadata.isEmpty())
            is_rtree = geometadata["INDEX_TYPE"].Int() != 0 ? true : false;
        columnName = geometadata["COLUMN_NAME"].str();
        //log() << "geowithin columnName:" << std::string(q.query.firstElement().fieldName() );
        
        BSONElement geowithincomm;
        BSONObj geometry;
        BSONObj type;
        BSONObj filter = jsobj["filter"].Obj();

        if (!filter.isEmpty() && columnName.compare(string(filter.firstElement().fieldName())) == 0 && filter.firstElement().isABSONObj())
        {
          
            is_registered = true;
            geowithincomm = filter.firstElement().Obj().firstElement();
            if ("$geoWithin" == string(geowithincomm.fieldName()))
            {
                is_command_geowithin = true;
      
                geometry = geowithincomm.Obj();
 
                if (geometry.firstElement().fieldName()!=NULL&&"$geometry" == string(geometry.firstElement().fieldName()))
                {
                    
                    query_condition = geometry["$geometry"].Obj();
                    if ("Polygon" == query_condition["type"].str()||"MultiPolygon" == query_condition["type"].str())
                        is_type_polygon = true;
                }
            }
            else if ("$geoIntersects" == string(geowithincomm.fieldName()))
            {
                is_command_geointersects = true;
                geometry = geowithincomm.Obj();
                query_condition = geometry["$geometry"].Obj();
            }
            else if ("$near" == string(geowithincomm.fieldName()))
            {
                is_command_geonear = true;
                geometry = geowithincomm.Obj();
                if (geometry.firstElement().fieldName()!=NULL&&(geometry.hasField("$geometry")))
                {
                    double maxDistance = 100;
                    if(geometry.hasField("$maxDistance"))
                        maxDistance= geometry["$maxDistance"].numberDouble();
                    double minDistance = 0;
                    if(geometry.hasField("$minDistance"))
                        minDistance = geometry["$minDistance"].numberDouble();
                        
                     //log()<<"maxDistance:"<<maxDistance;
                     //log()<<"minDistance:"<<minDistance;
                    BSONObjBuilder query_condition_builder;
                    query_condition_builder.appendElements(geometry["$geometry"].Obj());
                    query_condition_builder.append("$maxDistance",maxDistance);
                    query_condition_builder.append("$minDistance",minDistance);
                    query_condition = query_condition_builder.obj();
                    if ("Point" == query_condition["type"].str())
                        is_type_point = true;
                }
            }
        }

        if (is_rtree&&is_registered&&is_command_geowithin&&is_type_polygon)
        {
     
            commandName = "geoWithinSearch"; 
         
  
            BSONObjBuilder cmdObj;
            cmdObj.append("collection",collName);
            cmdObj.append("condition",query_condition);
            jsobj = cmdObj.obj();

        }
        if(is_rtree&&is_registered&&is_command_geointersects)
        {
            commandName = "geoIntersectSearch"; 
            BSONObjBuilder cmdObj;
            cmdObj.append("collection",collName);
            cmdObj.append("condition",query_condition);
            jsobj = cmdObj.obj();
        }
        if(is_rtree&&is_registered&&is_command_geonear&&is_type_point)
        {
            commandName = "geoNearSearch"; 
            BSONObjBuilder cmdObj;
            cmdObj.append("collection",collName);
            cmdObj.append("condition",query_condition);
            jsobj = cmdObj.obj();
        }
    }

    Command* c = e.type() ? Command::findCommand(commandName) : NULL;
    if (!c) {
        Command::appendCommandStatus(
            anObjBuilder, false, str::stream() << "no such cmd: " << commandName);
        anObjBuilder.append("code", ErrorCodes::CommandNotFound);
        Command::unknownCommands.increment();
        return;
    }

    execCommandClientBasic(txn, c, cc(), queryOptions, ns, jsobj, anObjBuilder);
}

void Command::registerError(OperationContext* txn, const DBException& exception) {}

}  // namespace mongo

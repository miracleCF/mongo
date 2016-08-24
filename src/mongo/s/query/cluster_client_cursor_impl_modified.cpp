/**
 *    Copyright (C) 2015 MongoDB Inc.
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
 *    must comply with the GNU Affero General Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kQuery

#include "mongo/platform/basic.h"

#include "mongo/s/query/cluster_client_cursor_impl.h"
#include<iostream>

#include "mongo/s/query/router_stage_limit.h"
#include "mongo/s/query/router_stage_merge.h"
#include "mongo/s/query/router_stage_mock.h"
#include "mongo/s/query/router_stage_remove_sortkey.h"
#include "mongo/s/query/router_stage_skip.h"
#include "mongo/stdx/memory.h"
#include "mongo/s/grid.h"
#include "mongo/s/catalog/catalog_cache.h"
#include "mongo/s/config.h"
#include "mongo/db/query/find_common.h"

using namespace std;

namespace mongo {

ClusterClientCursorGuard::ClusterClientCursorGuard(std::unique_ptr<ClusterClientCursor> ccc)
    : _ccc(std::move(ccc)) {}

ClusterClientCursorGuard::~ClusterClientCursorGuard() {
    if (_ccc && !_ccc->remotesExhausted()) {
        _ccc->kill();
    }
}

#if defined(_MSC_VER) && _MSC_VER < 1900
ClusterClientCursorGuard::ClusterClientCursorGuard(ClusterClientCursorGuard&& other)
    : _ccc(std::move(other._ccc)) {}

ClusterClientCursorGuard& ClusterClientCursorGuard::operator=(ClusterClientCursorGuard&& other) {
    _ccc = std::move(other._ccc);
    return *this;
}
#endif

ClusterClientCursor* ClusterClientCursorGuard::operator->() {
    return _ccc.get();
}

std::unique_ptr<ClusterClientCursor> ClusterClientCursorGuard::releaseCursor() {
    return std::move(_ccc);
}

ClusterClientCursorGuard ClusterClientCursorImpl::make(executor::TaskExecutor* executor,
                                                       ClusterClientCursorParams&& params) {
    std::unique_ptr<ClusterClientCursor> cursor(
        new ClusterClientCursorImpl(executor, std::move(params)));
    return ClusterClientCursorGuard(std::move(cursor));
}

ClusterClientCursorGuard ClusterClientCursorImpl::make(OperationContext* txn,
                                            const char* ns,
                                            BSONObj& jsobj,
                                            BSONObjBuilder& anObjBuilder,
                                            int queryOptions) {
    std::unique_ptr<ClusterClientCursor> cursor(
        new ClusterClientCursorImpl(txn,ns,jsobj,anObjBuilder,queryOptions));
    return  ClusterClientCursorGuard(std::move(cursor));
}

ClusterClientCursorImpl::ClusterClientCursorImpl(executor::TaskExecutor* executor,
                                                 ClusterClientCursorParams&& params)
    : _isTailable(params.isTailable), _root(buildMergerPlan(executor, std::move(params))) {}

ClusterClientCursorImpl::ClusterClientCursorImpl(std::unique_ptr<RouterStageMock> root)
    : _root(std::move(root)) {}
    
ClusterClientCursorImpl::ClusterClientCursorImpl(OperationContext* txn,
                            const char* ns,
                            BSONObj& jsobj,
                            BSONObjBuilder& anObjBuilder,
                            int queryOptions):_isRtree(true){
    BSONElement e = jsobj.firstElement();
    std::string dbname = nsToDatabase(ns);
    std::string collName = e.String();
    std::string columnName;
    BSONObj query_condition;
    std::string commandName = e.fieldName();//cmd name
    //log() << "ns:" << q.ns << ", collname: " << collName << endl;
    BSONObjBuilder bdr;
    //Whether the collection creates rtree index
    //bool is_rtree = false;
    //Whether the field that query is registered
    bool is_registered = false;
    //Whether the cmd is geowithin
    bool is_command_geowithin = false;
    //Whether the cmd is geoIntersects
    bool is_command_geointersects = false;
    //Whether the cmd is geonear
    bool is_command_geonear = false;
    //Whether the query type is polygon
    bool is_type_polygon = false;
    //Whether the query type is point        
    bool is_type_point = false;

    bdr.append("NAMESPACE", dbname+"."+collName);
    auto database_status = grid.catalogCache()->getDatabase(txn, dbname);
    uassertStatusOK(database_status.getStatus());
    std::shared_ptr<DBConfig> conf = database_status.getValue();
    BSONObj geometadata = conf->getGeometry(txn,bdr.obj());
    //if (!geometadata.isEmpty())
        //is_rtree = geometadata["INDEX_TYPE"].Int() != 0 ? true : false;
    columnName = geometadata["COLUMN_NAME"].str();
    BSONElement geowithincomm;
    BSONObj geometry;
    BSONObj type;
    BSONObj filter = jsobj["filter"].Obj();
    
    if (!filter.isEmpty() && columnName.compare(std::string(filter.firstElement().fieldName())) == 0 && filter.firstElement().isABSONObj())
    {
        //log() << "cao:" << filter.firstElement().Obj();
        is_registered = true;
        geowithincomm = filter.firstElement().Obj().firstElement();
        if ("$geoWithin" == std::string(geowithincomm.fieldName()))
        {
            is_command_geowithin = true;
            //log() << "geowithin : " << string(geowithincomm.fieldName()) << endl; ;
            geometry = geowithincomm.Obj();
            //log() << "geometry cmd:" << geometry << endl;
            //log() << "geometry cmd name:" << geometry.firstElement().fieldName() << endl;
            if (geometry.firstElement().fieldName()!=NULL&&"$geometry" == std::string(geometry.firstElement().fieldName()))
            {
                //log() << "last step";
                query_condition = geometry["$geometry"].Obj();
                if ("Polygon" == query_condition["type"].str()||"MultiPolygon" == query_condition["type"].str())
                    is_type_polygon = true;
            }
        }
        else if ("$geoIntersects" == std::string(geowithincomm.fieldName()))
        {
            is_command_geointersects = true;
            geometry = geowithincomm.Obj();
            query_condition = geometry["$geometry"].Obj();
        }
        else if ("$near" == std::string(geowithincomm.fieldName()))
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
                    
                   // log()<<"maxDistance:"<<maxDistance;
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
    if (is_registered&&is_command_geowithin&&is_type_polygon)
    {
        commandName = "geoWithinSearch";  
        BSONObjBuilder cmdObj;
        cmdObj.append("collection",collName);
        cmdObj.append("condition",query_condition);
        jsobj = cmdObj.obj();
    }
    if(is_registered&&is_command_geointersects)
    {
        commandName = "geoIntersectSearch"; 
        BSONObjBuilder cmdObj;
        cmdObj.append("collection",collName);
        cmdObj.append("condition",query_condition);
        jsobj = cmdObj.obj();
    }
    if(is_registered&&is_command_geonear&&is_type_point)
    {
        commandName = "geoNearSearch"; 
        BSONObjBuilder cmdObj;
        cmdObj.append("collection",collName);
        cmdObj.append("condition",query_condition);
        jsobj = cmdObj.obj();
    }
    //log()<<"before run, check the command name:"<<commandName;
    _command = e.type() ? Command::findCommand(commandName) : NULL;

    if (jsobj.getBoolField("help")) {
        std::stringstream help;
        help << "help for: " << _command->name << " ";
        _command->help(help);
        anObjBuilder.append("help", help.str());
        anObjBuilder.append("lockType", _command->isWriteCommandForConfigServer() ? 1 : 0);
        //appendCommandStatus(anObjBuilder, true, "");
        return;
    }
     /*
    _command->_commandsExecuted.increment();

    if (_command->shouldAffectCommandCounter()) {
        globalOpCounters.gotCommand();
    }
    */
    std::string errmsg;
    bool ok = false;
    try {
        ok = _command->run(txn, dbname, jsobj, queryOptions, errmsg, anObjBuilder);
    } catch (const DBException& e) {
        anObjBuilder.resetToEmpty();
        const int code = e.getCode();

        // Codes for StaleConfigException
        if (code == ErrorCodes::RecvStaleConfig || code == ErrorCodes::SendStaleConfig) {
            throw;
        }

        errmsg = e.what();
        anObjBuilder.append("code", code);
    }
    /*
    if (!ok) {
        _command->_commandsFailed.increment();
    }
    */
    //appendCommandStatus(anObjBuilder, ok, errmsg);
}

StatusWith<boost::optional<BSONObj>> ClusterClientCursorImpl::next() {
    // First return stashed results, if there are any.
    if (!_stash.empty()) {
        BSONObj front = std::move(_stash.front());
        _stash.pop();
        ++_numReturnedSoFar;
        return {front};
    }
    // std::cout<<"进入next"<<endl;
    if(_isRtree)
    {
        // std::cout<<"isRtree"<<endl;
        // std::cout<<_command->name<<"  ---zai next li"<<endl;
        int size = _command->rtreeDataMore(20,_stash);
        if(size!=0)
        {
            BSONObj front = std::move(_stash.front());
            _stash.pop();
            ++_numReturnedSoFar;
            return {front};
        }
        return BSONObj();    
    }
    else
    {
        auto next = _root->next();
        if (next.isOK() && next.getValue()) {
            ++_numReturnedSoFar;
        }
        return next;
    }
    
}

void ClusterClientCursorImpl::kill() {
    if(!_isRtree)
       _root->kill();
    else
       _command->freeCursor();
}

bool ClusterClientCursorImpl::isTailable() const {
    return _isTailable;
}

long long ClusterClientCursorImpl::getNumReturnedSoFar() const {
    return _numReturnedSoFar;
}

void ClusterClientCursorImpl::queueResult(const BSONObj& obj) {
    invariant(obj.isOwned());
    _stash.push(obj);
}
//for r-tree 
void ClusterClientCursorImpl::setExhausted(bool isExhausted)
{
    _isExhausted = isExhausted;
    // std::cout<<"is exhausted in cursor?  "<< isExhausted<<endl;
}

bool ClusterClientCursorImpl::remotesExhausted() {
    if(_isRtree)
       return _isExhausted;
    return _root->remotesExhausted();
}

Status ClusterClientCursorImpl::setAwaitDataTimeout(Milliseconds awaitDataTimeout) {  
    return _root->setAwaitDataTimeout(awaitDataTimeout);
}

std::unique_ptr<RouterExecStage> ClusterClientCursorImpl::buildMergerPlan(
    executor::TaskExecutor* executor, ClusterClientCursorParams&& params) {
    const auto skip = params.skip;
    const auto limit = params.limit;
    const bool hasSort = !params.sort.isEmpty();

    // The first stage is always the one which merges from the remotes.
    std::unique_ptr<RouterExecStage> root =
        stdx::make_unique<RouterStageMerge>(executor, std::move(params));

    if (skip) {
        root = stdx::make_unique<RouterStageSkip>(std::move(root), *skip);
    }

    if (limit) {
        root = stdx::make_unique<RouterStageLimit>(std::move(root), *limit);
    }

    if (hasSort) {
        root = stdx::make_unique<RouterStageRemoveSortKey>(std::move(root));
    }

    return root;
}

}  // namespace mongo

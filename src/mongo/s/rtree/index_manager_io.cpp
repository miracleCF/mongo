/**
*    Copyright (C) 2015 LIESMARS, Wuhan University.
*    Financially supported by Wuda Geoinfamatics Co. ,Ltd.
*    Author:  Xiang Longgang, Wang Dehao , Shao Xiaotian
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
*/

#include "index_manager_io.h"
#include "mongo/s/catalog/type_database.h"
#include "write_op.h"
#include "mongo/s/catalog/catalog_cache.h"

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kCommand
#include "mongo/util/log.h"

#include <iostream>

using namespace std;
using namespace rtree_index;
using namespace mongo;
using namespace geometry_parser;

namespace index_manager
{
	stdx::mutex _INDEXMANAGERIO_mu;

	
	
    MongoIndexManagerIO::MongoIndexManagerIO(DBClientBase * USER_CONN)
	{
		_conn = 0;
		//_conn = USER_CONN;
	}

	bool  MongoIndexManagerIO::connectMyself()
	{
		std::string errmsg;
		string url = "localhost:" + boost::lexical_cast<string>(serverGlobalParams.port);
		ConnectionString cs = ConnectionString::parse( url).getValue();
       // ConnectionString cs(ConnectionString::parse(connectionString));
        if (!cs.isValid()) {
           cout << "error parsing url: " << errmsg << endl;
           return false;
        }
		log() << "see what the connection string is: " << cs << endl;
		_conn= cs.connect(errmsg);
        if (!_conn) {
            cout << "couldn't connect: " << errmsg << endl;
			return false;
        }
		 
		return true;
	}

	bool  MongoIndexManagerIO::IsConnected()
	{
		if (_conn!=0&&this->_conn->isStillConnected())
			return true;
		else
			return false;
	}

	mongo::OID MongoIndexManagerIO::Basic_Generate_Key()
	{
		return mongo::OID::gen();
	}

	mongo::BSONObj MongoIndexManagerIO::Basic_Fetch_AtomData(string DB_NAME, string STORAGE_NAME, mongo::OID Data2Fetch)
	{
		/*findOne*/
		BSONObjBuilder bdr;
		bdr.append("_id",Data2Fetch);
		/*
		 * please note that this _conn for indexmanagerIO
		 * is shared by many threads. just like RTreeIO _conn
		 * so lock it 1st
		 */
		 BSONObj returnBSONOBJ;
		{
			stdx::lock_guard<stdx::mutex> CONN_lock(_INDEXMANAGERIO_mu);
			if(!IsConnected())
			{
				connectMyself();
			}
		    returnBSONOBJ =_conn->findOne(DB_NAME + "." + STORAGE_NAME, bdr.obj());
		}
		return returnBSONOBJ;
	}


	bool MongoIndexManagerIO::Basic_Store_One_Atom_Data(OperationContext* txn,string DB_NAME, string STORAGE_NAME, mongo::BSONObj AtomData, mongo::OID &AtomKey, BSONObjBuilder& result)
	{
		BSONObjBuilder bdr;
		AtomKey = AtomData["_id"].OID();
		//bdr.append("_id", AtomKey);
		bdr.appendElements(AtomData);
		/*insert*/
		bool ok;
		ok = RunWriteCommand(txn,DB_NAME, STORAGE_NAME, bdr.obj(), INSERT,result);
		//_conn->insert(DB_NAME + "." + STORAGE_NAME, AtomData);
		return 1;
	}

	bool MongoIndexManagerIO::Basic_Drop_Storage(OperationContext* txn,string DB_NAME, string STORAGE_NAME)
	{
		/*drop*/
		BSONObj empty;
		BSONObjBuilder bdr;
		BSONObjBuilder & bdrRef=bdr;
		return RunWriteCommand(txn,DB_NAME, STORAGE_NAME, empty, DROP,bdrRef);
	}

	int MongoIndexManagerIO::Basic_Delete_One_SpatialObj_Only(OperationContext* txn,string DB_NAME, string STORAGE_NAME, mongo::OID key2delete)
	{
		BSONObjBuilder bdr;
		bdr.append("_id",key2delete);
		/*remove*/
		//_conn->remove(_dbName + "." + STORAGE_NAME, bdr.obj());
		bool ok;
		
		BSONObjBuilder newBdr;
		BSONObjBuilder & newBdrRef=newBdr;
		ok = RunWriteCommand(txn,DB_NAME, STORAGE_NAME, bdr.obj(), REMOVE,newBdrRef);
		return 1;
	}

	int MongoIndexManagerIO::Basic_Get_Index_Type(OperationContext* txn,string DB_NAME,string STORAGE_NAME)
	{
		BSONObjBuilder bdr;
		bdr.append("NAMESPACE", DB_NAME+"."+STORAGE_NAME);

		/*findOne*/
		//auto status = grid.catalogCache()->getDatabase(txn, DB_NAME);
		auto status = grid.implicitCreateDb(txn, DB_NAME);
        uassertStatusOK(status.getStatus());
        shared_ptr<DBConfig> conf = status.getValue();
		//DBConfigPtr conf = grid.getDBConfig(DB_NAME, false);
		BSONObj oneGeoMeta = conf->getGeometry(txn,bdr.obj());

		if (oneGeoMeta.isEmpty())
		{
			return -1;
		}
		else
		{
			int index_type = oneGeoMeta["INDEX_TYPE"].Int();
			return index_type;
		}
		return -1;
		
	}

	bool MongoIndexManagerIO::Basic_Exist_Storage(string DB_NAME,string STORAGE_NAME)
	{
		/*EXIST*/
		stdx::lock_guard<stdx::mutex> CONN_lock(_INDEXMANAGERIO_mu);
		if(!IsConnected())
		{
				connectMyself();
		}
		if (_conn->exists(DB_NAME + "." + STORAGE_NAME))
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	int MongoIndexManagerIO::Basic_StorageOneGeoMeteData(OperationContext* txn,string DB_NAME, string STORAGE_NAME, string COLUMN_NAME, int INDEX_TYPE, MBR m, int GTYPE, int SRID, int CRS_TYPE, double TOLERANCE)
	{
		BSONObjBuilder bdr;
		bdr.append("NAMESPACE", DB_NAME+"."+STORAGE_NAME);
		bdr.append("COLUMN_NAME", COLUMN_NAME);
		bdr.append("INDEX_TYPE", INDEX_TYPE);
		OID tempOID;
		bdr.append("INDEX_INFO", tempOID);
		BSONArrayBuilder mbrbdr;
		mbrbdr.append(m.MinX);
		mbrbdr.append(m.MinY);
		mbrbdr.append(m.MaxX);
		mbrbdr.append(m.MaxY);
		bdr.append("MBR", mbrbdr.arr());
		bdr.append("SDO_GTYPE", GTYPE);
		bdr.append("SRID", SRID);
		bdr.append("CRS_TYPE", CRS_TYPE);
		bdr.append("TOLERANCE", TOLERANCE);
		/*insert*/
        //auto status = grid.catalogCache()->getDatabase(txn, DB_NAME);
		auto status = grid.implicitCreateDb(txn, DB_NAME);
        uassertStatusOK(status.getStatus());
        shared_ptr<DBConfig> conf = status.getValue();
		//DBConfigPtr conf = grid.getDBConfig(DB_NAME, false);
	    conf->registerGeometry(txn,bdr.obj());
		return 1;
	}

	int MongoIndexManagerIO::Basic_DeleteOneGeoMeteData(OperationContext* txn,string DB_NAME,string STORAGE_NAME)
	{
		BSONObjBuilder querybdr;
		querybdr.append("NAMESPACE", DB_NAME+"."+STORAGE_NAME);
		//auto status = grid.catalogCache()->getDatabase(txn, DB_NAME);
		auto status = grid.implicitCreateDb(txn, DB_NAME);
        uassertStatusOK(status.getStatus());
        shared_ptr<DBConfig> conf = status.getValue();
		//DBConfigPtr conf = grid.getDBConfig(DB_NAME, false);
		conf->deleteGeometry(txn,querybdr.obj());
		return 1;
	}

	int MongoIndexManagerIO::Basic_ModifyIndexType(OperationContext* txn,string DB_NAME, string STORAGE_NAME, int Type2Modify)
	{
		BSONObjBuilder condition;
		condition.append("NAMESPACE", DB_NAME+"."+STORAGE_NAME);
		BSONObjBuilder setBuilder;
		BSONObjBuilder setConditionBuilder;
		setConditionBuilder.append("INDEX_TYPE", Type2Modify);
		setBuilder.append("$set", setConditionBuilder.obj());

		//BSONObjBuilder cmdObj;
		//cmdObj.append("query", condition.obj());
		//cmdObj.append("update", setBuilder.obj());
		//bool ok;
		//ok = RunWriteCommand(DB_NAME, GeoMeteDataName, cmdObj.obj(), UPDATE);
		//auto status = grid.catalogCache()->getDatabase(txn, DB_NAME);
		auto status = grid.implicitCreateDb(txn, DB_NAME);
        uassertStatusOK(status.getStatus());
        shared_ptr<DBConfig> conf = status.getValue();
		conf->updateGeometry(txn,condition.obj(), setBuilder.obj());
		/*updata*/
		//_conn->update(_dbName + "." + GeoMeteDataName, Query(condition.obj()), setBuilder.obj());
		if (Type2Modify == 0)
		{
			BSONObjBuilder condition1;
			condition1.append("NAMESPACE", DB_NAME+"."+STORAGE_NAME);
			BSONObjBuilder setBuilder1;
			BSONObjBuilder setConditionBuilder1;
			mongo::OID nullOID;
			setConditionBuilder1.append("INDEX_INFO", nullOID);
			setBuilder1.append("$set", setConditionBuilder1.obj());

			/*updata*/
			conf->updateGeometry(txn,condition.obj(), setBuilder1.obj());
		}

		return 1;
	}

	int MongoIndexManagerIO::Basic_ModifyIndexMeteDataKey(OperationContext* txn,string DB_NAME, string STORAGE_NAME, mongo::OID Key2Modify)
	{
		
		BSONObjBuilder condition;
		condition.append("NAMESPACE", DB_NAME+"."+STORAGE_NAME);
		BSONObjBuilder setBuilder;
		BSONObjBuilder setConditionBuilder;
		setConditionBuilder.append("INDEX_INFO", Key2Modify);
		setBuilder.append("$set", setConditionBuilder.obj());
		//BSONObjBuilder cmdObj;
		//cmdObj.append("query", condition.obj());
		//cmdObj.append("update", setBuilder.obj());
		//bool ok;
		//ok = RunWriteCommand(DB_NAME, GeoMeteDataName, cmdObj.obj(), UPDATE);
		//auto status = grid.catalogCache()->getDatabase(txn, DB_NAME);
		auto status = grid.implicitCreateDb(txn, DB_NAME);
        uassertStatusOK(status.getStatus());
        shared_ptr<DBConfig> conf = status.getValue();
		conf->updateGeometry(txn,condition.obj(), setBuilder.obj());
		/*updata*/
		//_conn->update(_dbName + "." + GeoMeteDataName, Query(condition.obj()), setBuilder.obj());

		return 1;
	}

	bool MongoIndexManagerIO::Basic_Exist_Geo_MeteData(OperationContext* txn,string DB_NAME, string STORAGE_NAME)
	{
		BSONObjBuilder bdr;
		bdr.append("NAMESPACE",DB_NAME+"."+STORAGE_NAME);
		//log() << "checkgeo:" << DB_NAME + "." + STORAGE_NAME << endl;
		/*findOne*/
		//auto status = grid.catalogCache()->getDatabase(txn, DB_NAME);
       // uassertStatusOK(status.getStatus());
       // shared_ptr<DBConfig> conf = status.getValue();
		shared_ptr<DBConfig> conf =
                uassertStatusOK(grid.catalogCache()->getDatabase(txn, DB_NAME));
		bool is_exist = conf->checkGeoExist(txn,bdr.obj());
	
		return is_exist;
	}

	int MongoIndexManagerIO::RTree_StorageIndexMeteData(Transaction* t,string STORAGE_NAME, int MAX_NODE, int MAX_LEAF, mongo::OID RootKey)
	{
		BSONObjBuilder bdr;
		mongo::OID Index_INFO_OID = OID::gen();
		bdr.append("_id", Index_INFO_OID);
		bdr.append("MAX_NODE", MAX_NODE);
		bdr.append("MAX_LEAF", MAX_LEAF);
		bdr.append("INDEX_ROOT", RootKey);
        string DB_NAME = t->getDBName();
		OperationContext* txn = t->getOperationContext();
		auto status = grid.implicitCreateDb(txn, DB_NAME);
        uassertStatusOK(status.getStatus());
        shared_ptr<DBConfig> conf = status.getValue();
		conf->insertIndexMetadata(txn,bdr.obj());
		t->InsertDone(0,mongo::DatabaseType::IndexMetaDataNS,RTreeIndex::INSERT,"insertIndexMetadata");
		Basic_ModifyIndexMeteDataKey(txn,DB_NAME,STORAGE_NAME, Index_INFO_OID);
		t->UpdateDone(1,mongo::DatabaseType::IndexMetaDataNS,RTreeIndex::UPDATE,"Basic_ModifyIndexMeteDataKey");
		Basic_ModifyIndexType(txn,DB_NAME,STORAGE_NAME, 1);
		t->UpdateDone(2,mongo::DatabaseType::IndexMetaDataNS,RTreeIndex::UPDATE,"Basic_ModifyIndexType");
		return 1;
	}

	bool MongoIndexManagerIO::RTree_GetParms(OperationContext* txn,string DB_NAME,string STORAGE_NAME, mongo::OID &theRoot, int &MAX_NODE, int & MAX_LEAF, string &COLUMN_NAME)
	{
		BSONObjBuilder bdr;
		bdr.append("NAMESPACE", DB_NAME+"."+STORAGE_NAME);
		
		/*findOne*/
		//auto status = grid.catalogCache()->getDatabase(txn, DB_NAME);
		auto status = grid.implicitCreateDb(txn, DB_NAME);
		uassertStatusOK(status.getStatus());
        shared_ptr<DBConfig> conf = status.getValue();
		BSONObj Geo = conf->getGeometry(txn,bdr.obj());
		//BSONObj Geo = _conn->findOne(DB_NAME+"."+GeoMeteDataName,bdr.obj());
		//log() << "Geoooooo:" << Geo << endl;
		COLUMN_NAME = Geo["COLUMN_NAME"].String();

		if (Geo["INDEX_TYPE"].Int() == 1)
		{
			mongo::OID index_info_oid = Geo["INDEX_INFO"].OID();
			BSONObjBuilder indexquerybdr;
			indexquerybdr.append("_id", index_info_oid);

			/*findOne*/
			BSONObj index_info = conf->getIndexMetadata(txn,indexquerybdr.obj());

			theRoot = index_info["INDEX_ROOT"].OID();
			MAX_NODE = index_info["MAX_NODE"].Int();
			MAX_LEAF = index_info["MAX_LEAF"].Int();
		}
		return true;
	}

	int MongoIndexManagerIO::RTree_ModifyRootKey(OperationContext* txn,string DB_NAME, string STORAGE_NAME, mongo::OID newRootKey)
	{
		BSONObjBuilder bdr;
		bdr.append("NAMESPACE", DB_NAME+"."+STORAGE_NAME);

		/*findOne*/
		//BSONObj Geo = _conn->findOne(DB_NAME + "." + GeoMeteDataName, bdr.obj());
		//auto status = grid.catalogCache()->getDatabase(txn, DB_NAME);
		auto status = grid.implicitCreateDb(txn, DB_NAME);
        uassertStatusOK(status.getStatus());
        shared_ptr<DBConfig> conf = status.getValue();
		BSONObj Geo = conf->getGeometry(txn,bdr.obj());

		BSONObjBuilder condition;
		condition.append("_id", Geo["INDEX_INFO"].OID());
		BSONObjBuilder setBuilder;
		BSONObjBuilder setConditionBuilder;
		setConditionBuilder.append("INDEX_ROOT", newRootKey);
		setBuilder.append("$set", setConditionBuilder.obj());

	//	BSONObjBuilder cmdObj;
	//	cmdObj.append("query", condition.obj());
	//	cmdObj.append("update", setBuilder.obj());

		conf->updateIndexMetadata(txn,condition.obj(), setBuilder.obj());
		//bool ok;
		//ok = RunWriteCommand(DB_NAME, RTreeIndexMetaData, cmdObj.obj(), UPDATE);

		/*updata*/
		//_conn->update(_dbName + "." + RTreeIndexMetaData, Query(condition.obj()), setBuilder.obj());

		return 1;
	}

	bool MongoIndexManagerIO::RTree_DeleteIndex(OperationContext* txn,string DB_NAME, string STORAGE_NAME)
	{
		BSONObjBuilder bdr;
		bdr.append("NAMESPACE", DB_NAME+"."+STORAGE_NAME);

		/*findOne*/
		//BSONObj Geo = _conn->findOne(DB_NAME + "." + GeoMeteDataName, bdr.obj());
		//auto status = grid.catalogCache()->getDatabase(txn, DB_NAME);
		auto status = grid.implicitCreateDb(txn, DB_NAME);
        uassertStatusOK(status.getStatus());
        shared_ptr<DBConfig> conf = status.getValue();
		BSONObj Geo = conf->getGeometry(txn,bdr.obj());

		BSONObjBuilder condition;
		condition.append("_id", Geo["INDEX_INFO"].OID());
		/*remove*/
		//_conn->remove(_dbName + "." + RTreeIndexMetaData, condition.obj());
		//ok = RunWriteCommand(DB_NAME, RTreeIndexMetaData, condition.obj(), REMOVE);
		conf->deleteIndexMetadata(txn,condition.obj());

		/*Exist*/
		{
		stdx::lock_guard<stdx::mutex> CONN_lock(_INDEXMANAGERIO_mu);
		if(!IsConnected())
		{
			connectMyself();
		}
		if (_conn->exists(DB_NAME + "." + "rtree_" + STORAGE_NAME))
		{
			/*Drop*/
			_conn->dropCollection(DB_NAME + "." + "rtree_" + STORAGE_NAME);
		}
		}

		Basic_DeleteOneGeoMeteData(txn,DB_NAME, STORAGE_NAME);

		return 1;
	}

	bool MongoIndexManagerIO::RTree_ExistIndex(OperationContext* txn,string DB_NAME,string STORAGE_NAME)
	{
		BSONObjBuilder bdr;
		bdr.append("NAMESPACE", DB_NAME+"."+STORAGE_NAME);
		/*findOne*/
		//BSONObj Geo = _conn->findOne(DB_NAME + "." + GeoMeteDataName, bdr.obj());
		//auto status = grid.catalogCache()->getDatabase(txn, DB_NAME);
		auto status = grid.implicitCreateDb(txn, DB_NAME);
        uassertStatusOK(status.getStatus());
        shared_ptr<DBConfig> conf = status.getValue();
		bool is_exist = conf->checkRtreeExist(txn,bdr.obj());

		return is_exist;
	}

	bool MongoIndexManagerIO::RTree_GetDataMBR(string DB_NAME,string STORAGE_NAME, MBR &returnMBR, mongo::OID DataNodeKey, string COLUMN_NAME)
	{
		BSONObjBuilder bdr;
		bdr.append("_id",DataNodeKey);
		/*findOne*/
		BSONObj oneDoc;
		stdx::lock_guard<stdx::mutex> CONN_lock(_INDEXMANAGERIO_mu);
        {
			if(!IsConnected())
			{
				connectMyself();
			}
	    	oneDoc= _conn->findOne(DB_NAME+"."+STORAGE_NAME,bdr.obj());
		}		
		if (oneDoc.isEmpty())
		{
			return false;
		}

		BSONObj GeoObj = oneDoc[COLUMN_NAME].Obj();

		GeoJSONEngine::GeoJSONPaser::VerifyGeoBSONType(GeoObj, returnMBR);
		return true;
	}

	bool MongoIndexManagerIO::RTree_GetDataMBR(mongo::BSONObj AtomData, string COLUMN_NAME, MBR &returnMBR)
	{
		BSONObj GeoObj = AtomData[COLUMN_NAME].Obj();
		if (AtomData.hasField(COLUMN_NAME))
		{
			GeoJSONEngine::GeoJSONPaser::VerifyGeoBSONType(GeoObj, returnMBR);
			return true;
		}
		return false;
	}

	void MongoIndexManagerIO::Basic_Init_Storage_Traverse(OperationContext* txn,string DB_NAME,string STORAGE_NAME)
	{
		_Current_Storage = STORAGE_NAME;
		/*query*/
		{
			stdx::lock_guard<stdx::mutex> CONN_lock(_INDEXMANAGERIO_mu);
			if(!IsConnected())
			{
				connectMyself();
			}
			_Current_Cursor = _conn->query(DB_NAME+"."+_Current_Storage,BSONObj());
		}
		BSONObjBuilder bdr;
		bdr.append("NAMESPACE",DB_NAME+"."+STORAGE_NAME);
		/*findOne*/
		//BSONObj oneGeoMeteData = _conn->findOne(DB_NAME + "." + GeoMeteDataName, bdr.obj());
		//auto status = grid.catalogCache()->getDatabase(txn, DB_NAME);
		auto status = grid.implicitCreateDb(txn, DB_NAME);
        uassertStatusOK(status.getStatus());
        shared_ptr<DBConfig> conf = status.getValue();
		BSONObj oneGeoMeteData = conf->getGeometry(txn,bdr.obj());
		_COLUMN_NAME = oneGeoMeteData["COLUMN_NAME"].String();
	}

	/*
	  1: OK and Have Next;
	  -1: Not Ok ,error Data
	  0: Traverse Over
	*/
	int MongoIndexManagerIO::Basic_Storage_Traverse_Next(MBR &returnMBR, mongo::OID &returnKey)
	{
		/*more*/
		if (_Current_Cursor->more())
		{
			/*next*/
			BSONObj oneDoc = _Current_Cursor->next();
			returnKey = oneDoc["_id"].OID();
			
			BSONObj GeoJSONObj = oneDoc[_COLUMN_NAME].Obj();
			if (GeoJSONEngine::GeoJSONPaser::VerifyGeoBSONType(GeoJSONObj, returnMBR))
			{
				return 1;
			}
			return -1;
		}
		else
		{
			return 0;
		}
		return 0;
	}

	DBClientBase * MongoIndexManagerIO::Basic_Get_Connection()
	{
		return _conn;
	}

	bool MongoIndexManagerIO::Geo_Verify_Intersect(mongo::OID childOID, geos::geom::Geometry *searchGeometry, bool Conditions, string DB_NAME, string STORAGE_NAME, string COLUMN_NAME)
	{
		if (Conditions)
		{
			return true;
		}
		else
		{

			BSONObj atomdata = Basic_Fetch_AtomData(DB_NAME, STORAGE_NAME, childOID);
			BSONObj geoObj = atomdata[COLUMN_NAME].Obj();
			if (geoObj.isEmpty())return false;
			geom::Geometry *pGeometry = NULL;
			int parseSt = MGP.DataType2Geometry(geoObj, pGeometry);
			bool returnValue = false;
			if (parseSt == 1)//parseSuccess
			{
				if (pGeometry->intersects(searchGeometry))
				{
					returnValue = true;
				}
			}
			delete pGeometry;
			return returnValue;
		}

	}

	bool MongoIndexManagerIO::Geo_Verify_Contain(mongo::OID childOID, geos::geom::Geometry *searchGeometry, bool Conditions, string DB_NAME, string STORAGE_NAME, string COLUMN_NAME)
	{
		if (Conditions)
		{
			return true;
		}
		else
		{

			BSONObj atomdata = Basic_Fetch_AtomData(DB_NAME, STORAGE_NAME, childOID);
			BSONObj geoObj = atomdata[COLUMN_NAME].Obj();
			if (geoObj.isEmpty())return false;
			geom::Geometry *pGeometry = NULL;
			int parseSt = MGP.DataType2Geometry(geoObj, pGeometry);
			bool returnValue = false;
			if (parseSt == 1)//parseSuccess
			{
				if (searchGeometry->contains(pGeometry))
				{
					returnValue = true;
				}
			}
			delete pGeometry;
			return returnValue;
		}
	}


	bool MongoIndexManagerIO::Geo_Verify_Intersect(mongo::OID childOID, double ctx, double cty,double rMin,double rMax, bool Contidions, string DB_NAME, string STORAGE_NAME, string COLUMN_NAME,double &distance)
	{
		BSONObj atomdata = Basic_Fetch_AtomData(DB_NAME, STORAGE_NAME, childOID);
		BSONObj geoObj = atomdata[COLUMN_NAME].Obj();
		if (geoObj.isEmpty())return false;
		GeometryFactory factory;
		CoordinateArraySequenceFactory csf;
		geom::Point *pPoint = factory.createPoint(Coordinate(ctx, cty, 0));
		geom::Geometry *pGeometry = NULL;
		int parseSt = MGP.DataType2Geometry(geoObj, pGeometry);
		bool returnValue = false;
		if (parseSt == 1)//parseSuccess
		{
			distance = pGeometry->distance(pPoint);
			if (distance <= rMax&&distance >= rMin)
			{
				returnValue = true;
			}
			
		}
		delete pPoint;
		delete pGeometry;
		return returnValue;
	}

	bool MongoIndexManagerIO::ParseGeometry(mongo::BSONObj Geometry2Parser, geos::geom::Geometry *&parsedGeometry)
	{
		try
		{
			int parseSt = MGP.DataType2Geometry(Geometry2Parser, parsedGeometry);
			if (parseSt == 1)//parseSuccess
				return true;
			else
				return false;
		}
		catch (geos::util::GEOSException e)
		{
			cout << e.what() << endl;
			return false;
		}
	}
    
}
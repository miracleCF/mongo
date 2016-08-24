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

#pragma once

#include <iostream>
#include "rtree_core.h"
#include "geojson_engine.h"
#include "mongo_geometry_parser.h"
// #include "GeoRef.h"
#include <boost/scoped_ptr.hpp>
#include "mongo/s/catalog/type_database.h"
#include "mongo/s/rtree/transaction.h"

//header from s2 (geos ex)
#include "third_party/s2/Geometry.h"
#include "third_party/s2/GeometryFactory.h"
#include "third_party/s2/CoordinateArraySequenceFactory.h"
#include "third_party/s2/CoordinateSequence.h"
#include "third_party/s2/Polygon.h"
#include "third_party/s2/LineString.h"
#include "third_party/s2/Point.h"
#include "third_party/s2/GeometryCollection.h"
#include "third_party/s2/MultiLineString.h"
#include "third_party/s2/MultiPoint.h"
#include "third_party/s2/MultiPolygon.h"
#include "third_party/s2/GEOSException.h"
#include "third_party/s2/LinearRing.h"

#include "mongo/stdx/mutex.h"


using namespace std;
using namespace rtree_index;
using namespace mongo;
using namespace geometry_parser;

namespace index_manager
{
	//const string GeoMeteDataName = "geometry";
	//const string RTreeIndexMetaData = "geoindex";
    extern stdx::mutex _INDEXMANAGERIO_mu;
	/**
	 *Some operations in meta data are defined here
	 *By calling functions defined in DBConfig class
	 */


	class MongoIndexManagerIO 
	{
	public:
		MongoIndexManagerIO(DBClientBase * USER_CONN);
		mongo::OID Basic_Generate_Key();
		mongo::BSONObj Basic_Fetch_AtomData(string DB_NAME, string STORAGE_NAME, mongo::OID Data2Fetch);
		bool Basic_Store_One_Atom_Data(OperationContext* txn,string DB_NAME, string STORAGE_NAME, mongo::BSONObj AtomData, mongo::OID &AtomKey, BSONObjBuilder& result);
		bool Basic_Drop_Storage(OperationContext* txn,string DB_NAME, string STORAGE_NAME);
		int Basic_Delete_One_SpatialObj_Only(OperationContext* txn,string DB_NAME, string STORAGE_NAME, mongo::OID key2delete);
		int Basic_Get_Index_Type(OperationContext* txn,string DB_NAME, string STORAGE_NAME);
		bool Basic_Exist_Storage(string DB_NAME, string STORAGE_NAME);
		int Basic_StorageOneGeoMeteData(OperationContext* txn,string DB_NAME, string STORAGE_NAME, string COLUMN_NAME, int INDEX_TYPE, MBR m, int GTYPE, int SRID, int CRS_TYPE, double TOLERANCE);
		int Basic_DeleteOneGeoMeteData(OperationContext* txn,string DB_NAME, string STORAGE_NAME);
		int Basic_ModifyIndexType(OperationContext* txn,string DB_NAME, string STORAGE_NAME, int Type2Modify);
		bool Basic_Exist_Geo_MeteData(OperationContext* txn,string DB_NAME, string COLLECTION_NAME);
		int Basic_ModifyIndexMeteDataKey(OperationContext* txn,string DB_NAME, string STORAGE_NAME, mongo::OID Key2Modify);
		int RTree_StorageIndexMeteData(Transaction* t, string STORAGE_NAME, int MAX_NODE, int MAX_LEAF, mongo::OID RootKey);
		int RTree_ModifyRootKey(OperationContext* txn,string DB_NAME, string STORAGE_NAME, mongo::OID newRootKey);
		bool RTree_GetParms(OperationContext* txn,string DB_NAME, string STORAGE_NAME, mongo::OID &theRoot, int &MAX_NODE, int & MAX_LEAF, string &COLUMN_NAME);
		bool RTree_DeleteIndex(OperationContext* txn,string DB_NAME, string STORAGE_NAME);
		bool RTree_ExistIndex(OperationContext* txn,string DB_NAME, string STORAGE_NAME);
		bool RTree_GetDataMBR(string DB_NAME, string STORAGE_NAME, MBR &returnMBR, mongo::OID DataNodeKey, string COLUMN_NAME);
		bool RTree_GetDataMBR(mongo::BSONObj AtomData, string COLUMN_NAME, MBR &returnMBR);
		void Basic_Init_Storage_Traverse(OperationContext* txn,string DB_NAME, string STORAGE_NAME);
		int Basic_Storage_Traverse_Next(MBR &returnMBR, mongo::OID &returnKey);
		DBClientBase* Basic_Get_Connection();
		bool Geo_Verify_Intersect(mongo::OID childOID, geos::geom::Geometry *searchGeometry, bool Conditions, string DB_NAME, string STORAGE_NAME, string COLUMN_NAME);
		bool Geo_Verify_Contain(mongo::OID childOID, geos::geom::Geometry *searchGeometry, bool Conditions, string DB_NAME, string STORAGE_NAME, string COLUMN_NAME);
		bool Geo_Verify_Intersect(mongo::OID childOID, double ctx, double cty, double rMin,double rMax,bool Contidions, string DB_NAME, string STORAGE_NAME, string COLUMN_NAME,double &distance);
		bool ParseGeometry(mongo::BSONObj Geometry2Parser, geos::geom::Geometry *&parsedGeometry);
		bool connectMyself();
		bool IsConnected();



	private:
	    MongoGeometryParser MGP;
		string _connectionString;//place here rather in IndexManagerBase to be IO detached
		DBClientBase *_conn;
		string _Current_Storage;
		std::unique_ptr<DBClientCursor> _Current_Cursor;
		string _COLUMN_NAME;//used to accelerate the process if necessary
	};

	





}
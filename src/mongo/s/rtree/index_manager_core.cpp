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

#include "index_manager_core.h"
#include "mongo/db/operation_context_impl.h"
#include <iostream>
using namespace std;
namespace index_manager
{
	
	bool nearcompare(KeywithDis o1, KeywithDis o2)
	{
		if (o1.distance < o2.distance)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	

	IndexManagerBase::IndexManagerBase()
	{
		//RTree();
	}
	
	IndexManagerBase::IndexManagerBase(MongoIndexManagerIO *USER_INDEXMANAGER_IO, MongoIO *USER_RTREE_IO)
	{
		IO = USER_INDEXMANAGER_IO;
		_RIO = USER_RTREE_IO;
		_R.IO = _RIO;
	}
	
	
	int IndexManagerBase::RegisterGeometry(OperationContext* txn,string DB_NAME, string COLLECTION_NAME, string COLUMN_NAME, int SDO_GTYPE, int SDO_SRID, int CRS_TYPE, double SDO_TORRANCE)
	{
		
		if (IO->Basic_Exist_Geo_MeteData(txn,DB_NAME,COLLECTION_NAME))
		{
			return -1;
		}
		
		if (SDO_GTYPE < 0 || SDO_GTYPE>7)
		{
			return -2;
		}
		if (SDO_TORRANCE < 0)
		{
			return -3;
		}
		if (CRS_TYPE < 0 || CRS_TYPE>2)
		{
			return -4;
		}


		if (IO->Basic_StorageOneGeoMeteData(txn,DB_NAME,COLLECTION_NAME, COLUMN_NAME, 0, MBR(0, 0, 0, 0), SDO_GTYPE, SDO_SRID, CRS_TYPE, SDO_TORRANCE))
		{

			return 1;
		}
		return 0;
		
	}


	int IndexManagerBase::PrepareIndex(OperationContext* txn,string DB_NAME, string COLLECTION_NAME, string COLUMN_NAME, int INDEX_TYPE, int Max_Node, int Max_Leaf)
	{
		std::cout<<"NS in Operation context: (PrepareIndex)"<<txn->getNS()<<endl;
		
		//log() << "position0" << endl;
		if (INDEX_TYPE <= 0 || INDEX_TYPE > 2)
		{
			return -1;
		}
		if (!IO->Basic_Exist_Geo_MeteData(txn,DB_NAME,COLLECTION_NAME))
		{
			return -2;
		}
		else
		{	
			if (IO->RTree_ExistIndex(txn,DB_NAME,COLLECTION_NAME))
			{
				return -3;
			}
			if (INDEX_TYPE == 1)
			{
				Transaction* t = new CreateIndexTransaction(txn,DB_NAME);
				mongo::OID tempkey;
				IO->RTree_StorageIndexMeteData(t,COLLECTION_NAME, Max_Node, Max_Leaf, tempkey);
				mongo::OID nullKey;
				_R.ReConfigure(Max_Node, Max_Leaf, nullKey, DB_NAME, COLLECTION_NAME);

				mongo::OID Root;
				_R.InsertRoot(txn,Root);
				t->InsertDone(3,"rtree_"+COLLECTION_NAME,RTreeIndex::INSERT,"InsertRoot");

				IO->Basic_Init_Storage_Traverse(txn,DB_NAME,COLLECTION_NAME);
				t->UpdateDone(4,"rtree_"+COLLECTION_NAME,rtree_index::UPDATE,"begin building Rtree index on existing data");
				mongo::OID oneKey;
				MBR oneMBR;
				int count = 0;

				while (IO->Basic_Storage_Traverse_Next(oneMBR,oneKey))
				{
					Branch b;
					b.ChildKey=oneKey;
					b.HasData = true;
					b.mbr = oneMBR;
					_R.Insert(txn,Root, b, 0);
					if (count%100==0)
					cout << count << "  "<< Root<<"\r";
					count++;
				}
				t->UpdateDone(5,"rtree_"+COLLECTION_NAME,rtree_index::UPDATE,"finish building Rtree index on existing data");
				
				IO->RTree_ModifyRootKey(txn,DB_NAME,COLLECTION_NAME, Root);
				t->UpdateDone(6,mongo::DatabaseType::GeoMetaDataNS,rtree_index::UPDATE,"Update Root Key in meta_geom");
				delete t;
				return 1;
			}
			else
			{
				return -4;
			}
			return 0;
		}
		return false;
	}

	/*
	   1: OK
	   0: Something wrong
	*/
	
	int IndexManagerBase::ValidateGeometry(OperationContext* txn,string DB_NAME,string COLLECTION_NAME)
	{
		IO->Basic_Init_Storage_Traverse(txn,DB_NAME,COLLECTION_NAME);
		mongo::OID oneKey;
		MBR oneMBR;

		//Easy Verify,Robust Verify please use mongo:: GeoJSON parser
		while (true)
		{
			int flag = IO->Basic_Storage_Traverse_Next(oneMBR, oneKey);
			if (flag == 1)
			{
				continue;
			}
			if (flag == 0)
			{
				break;
			}
			if (flag == -1)
			{
				return 0;
			}
			
		}
		return 1;
	}

	
	std::unique_ptr<RTreeRangeQueryCursor>  IndexManagerBase::GeoSearchWithin(OperationContext* txn,string DB_NAME, string COLLECTION_NAME, mongo::BSONObj InputGeometry)
	{
		vector < mongo::OID >  ResultKeys;
		vector<bool> lazyIntersects;
		vector<mongo::OID> RefinedResultKeys;
		int index_type = IO->Basic_Get_Index_Type(txn,DB_NAME, COLLECTION_NAME);
		// log()<<"index_type:"<<index_type;
		if (index_type> 0)
		{
			if (index_type == 1)
			{
				mongo::OID RootKey;
				int Max_Node = 0;
				int Max_Leaf = 0;
				string cn;
				if (IO->RTree_GetParms(txn,DB_NAME, COLLECTION_NAME, RootKey, Max_Node, Max_Leaf, cn))
				{
					// log()<<Max_Node<<","<<Max_Leaf;
					_R.ReConfigure(Max_Node, Max_Leaf, RootKey, DB_NAME, COLLECTION_NAME);
                    //parseGeometry
					geos::geom::Geometry * pGeometry = NULL;
					if (IO->ParseGeometry(InputGeometry, pGeometry))//parseSuccess
					{
						Node RootNode = _RIO->Basic_Find_One_Node(RootKey);
						std::unique_ptr<RTreeRangeQueryCursor> returnCursor (new RTreeRangeQueryCursor(Max_Node, RootNode, _RIO, IO, pGeometry, DB_NAME, COLLECTION_NAME, cn, 0));
						returnCursor->InitCursor();
						return std::move(returnCursor);
					}
					delete pGeometry;//free the memory of geos geos geometry
				}
			}
		}
		std::unique_ptr<RTreeRangeQueryCursor> returnCursor;
		return returnCursor;
	}
	
	
	bool IndexManagerBase::GeoSearchWithinWithoutRefining(OperationContext* txn,string DB_NAME, string COLLECTION_NAME,mongo::BSONObj InputGeometry, vector<mongo::OID>& results)
	{
		vector<bool> lazyIntersects;
		int index_type = IO->Basic_Get_Index_Type(txn,DB_NAME, COLLECTION_NAME);
		// log()<<"IndexType:"<<index_type;
		// log()<<"filter:"<<InputGeometry;
		if (index_type> 0)
		{
			if (index_type == 1)
			{
				mongo::OID RootKey;
				int Max_Node = 0;
				int Max_Leaf = 0;
				string cn;
				if (IO->RTree_GetParms(txn,DB_NAME, COLLECTION_NAME, RootKey, Max_Node, Max_Leaf, cn))
				{
					// log()<<Max_Node<<","<<Max_Leaf;
					_R.ReConfigure(Max_Node, Max_Leaf, RootKey, DB_NAME, COLLECTION_NAME);
                    //parseGeometry
					geos::geom::Geometry * pGeometry = NULL;
					if (IO->ParseGeometry(InputGeometry, pGeometry))//parseSuccess
					{
						if (pGeometry->getGeometryType() == "MultiPolygon" || pGeometry->getGeometryType() == "Polygon")
						{
							_R.Search(pGeometry, results, lazyIntersects);
						}
						else
						{
							cout << "$GeoWithIn only supports \"Polygon\" and \"MutiPolygon\""<<endl;
							return false;
						}
					}
					delete pGeometry;//free the memory of geos geos geometry
				}
			}
		}
		//RTreeCursor<Key> returnCursor(RefinedResultKeys, DB_NAME, COLLECTION_NAME);
		return true;
	}

		
	std::unique_ptr<RTreeRangeQueryCursor> IndexManagerBase::GeoSearchIntersects(OperationContext* txn,string DB_NAME, string COLLECTION_NAME, mongo::BSONObj InputGeometry)
	{
		vector < mongo::OID >  ResultKeys;
		vector<bool> lazyIntersects;
		vector<mongo::OID> RefinedResultKeys;
		int index_type = IO->Basic_Get_Index_Type(txn,DB_NAME, COLLECTION_NAME);
		// log()<<"index_type:"<<index_type;
		if (index_type> 0)
		{
			if (index_type == 1)
			{
				mongo::OID RootKey;
				int Max_Node = 0;
				int Max_Leaf = 0;
				string cn;
				if (IO->RTree_GetParms(txn,DB_NAME, COLLECTION_NAME, RootKey, Max_Node, Max_Leaf, cn))
				{
					// log()<<Max_Node<<","<<Max_Leaf;
					_R.ReConfigure(Max_Node, Max_Leaf, RootKey, DB_NAME, COLLECTION_NAME);
                    //parseGeometry
					geos::geom::Geometry * pGeometry = NULL;
					if (IO->ParseGeometry(InputGeometry, pGeometry))//parseSuccess
					{
		                Node RootNode = _RIO->Basic_Find_One_Node(RootKey);
						std::unique_ptr<RTreeRangeQueryCursor> returnCursor (new RTreeRangeQueryCursor(Max_Node, RootNode, _RIO, IO, pGeometry, DB_NAME, COLLECTION_NAME, cn, 1));
						returnCursor->InitCursor();
						return std::move(returnCursor);
					}
                   /*
					*  Please Note We should delete pGeometry
					*  After finshing using the RTreeRangeQueryCursor
					*  use RTreeRangeQueryCursor->FreeCursor() when  cursor is exhausted
					*/
				}
			}
		}
		std::unique_ptr<RTreeRangeQueryCursor> returnCursor;
		return returnCursor;
	}
	
	
	std::unique_ptr<RTreeGeoNearCurosr> IndexManagerBase::GeoSearchNear(OperationContext* txn, string DB_NAME,string COLLECTION_NAME,double ctx,double cty,double rMin,double rMax)
	{
		vector < mongo::OID >  ResultKeys;
		vector<bool> lazyIntersects;
		vector<mongo::OID> RefinedResultKeys;
		vector<KeywithDis> toBeSort;
		int index_type = IO->Basic_Get_Index_Type(txn, DB_NAME, COLLECTION_NAME);
		if (index_type > 0)
		{
			if (index_type == 1)
			{
				mongo::OID RootKey;
				int Max_Node = 0;
				int Max_Leaf = 0;
				string cn;
				if (IO->RTree_GetParms(txn,DB_NAME, COLLECTION_NAME, RootKey, Max_Node, Max_Leaf, cn))
				{
					_RIO->Configure(DB_NAME,COLLECTION_NAME,Max_Node,Max_Leaf);
					Node rootNode = _RIO->Basic_Find_One_Node(RootKey);
					GeoNearSearchNode *RootN = new GeoNearSearchNode(0,9999999,111,rootNode);
					std::unique_ptr<RTreeGeoNearCurosr> TestCursor(new RTreeGeoNearCurosr(Max_Node, RootN, _RIO, IO, ctx, cty, rMin, rMax,DB_NAME,COLLECTION_NAME,cn));
					TestCursor->InitCursor();
					return std::move(TestCursor);
				}
			}
		}
		std::unique_ptr<RTreeGeoNearCurosr> returnCursor;
		return returnCursor;	
	}
	
	
	
	int IndexManagerBase::DeleteGeoObjByKey(OperationContext* txn,string DB_NAME,string COLLECTION_NAME, mongo::OID key2delete)
	{
		mongo::OID RootKey;
		int Max_Node, Max_Leaf;
		Max_Node = 0;
		Max_Leaf = 0;
		string cn;
		IO->RTree_GetParms(txn,DB_NAME,COLLECTION_NAME, RootKey, Max_Node, Max_Leaf,cn);
		_R.ReConfigure(Max_Node, Max_Leaf, RootKey, DB_NAME, COLLECTION_NAME);
		MBR m;
		IO->RTree_GetDataMBR(DB_NAME,COLLECTION_NAME, m, key2delete, cn);
		_R.DeleteNode(txn,RootKey, key2delete, m);
		IO->RTree_ModifyRootKey(txn,DB_NAME,COLLECTION_NAME, RootKey);
		IO->Basic_Delete_One_SpatialObj_Only(txn,DB_NAME,COLLECTION_NAME, key2delete);
		
		return 1;
	}

	
	int IndexManagerBase::DeleteIntersectedGeoObj(OperationContext* txn,string DB_NAME,string COLLECTION_NAME, mongo::BSONObj InputGeometry)
	{
		int index_type = IO->Basic_Get_Index_Type(txn,DB_NAME, COLLECTION_NAME);
		vector<mongo::OID> RefinedResultKeys;
		if (index_type > 0)
		{
			if (index_type == 1)
			{
				vector < mongo::OID >  ResultKeys;
				vector<bool> lazyIntersects;
				mongo::OID RootKey;
				string cn;
				int Max_Node = 0;
				int Max_Leaf = 0;
				if (IO->RTree_GetParms(txn,DB_NAME, COLLECTION_NAME, RootKey, Max_Node, Max_Leaf, cn))
				{
					 //parseGeometry
					geos::geom::Geometry * pGeometry = NULL;
					if (IO->ParseGeometry(InputGeometry, pGeometry))//parseSuccess
					{
					    _R.ReConfigure(Max_Node, Max_Leaf, RootKey, DB_NAME, COLLECTION_NAME);
					    _R.Search(pGeometry, ResultKeys, lazyIntersects);
		   			    for (unsigned int i = 0; i < ResultKeys.size(); i++)
					    {
						    if (IO->Geo_Verify_Intersect(ResultKeys[i], pGeometry, lazyIntersects[i], DB_NAME, COLLECTION_NAME, cn))
					    	{
							    RefinedResultKeys.push_back(ResultKeys[i]);
						    }
					    }
					    for (unsigned int i = 0; i < RefinedResultKeys.size(); i++)
					    {
						    MBR datambr;
					    	IO->RTree_GetDataMBR(DB_NAME, COLLECTION_NAME, datambr, RefinedResultKeys[i], cn);
					    	_R.DeleteNode(txn,RootKey, RefinedResultKeys[i], datambr);
						    IO->RTree_ModifyRootKey(txn,DB_NAME, COLLECTION_NAME, RootKey);
					    	IO->Basic_Delete_One_SpatialObj_Only(txn,DB_NAME, COLLECTION_NAME, RefinedResultKeys[i]);
					    }
					}
				}
			}
			return 1;
		}
		return 0;
	}
	
	
	
	int IndexManagerBase::DeleteContainedGeoObj(OperationContext* txn,string DB_NAME,string COLLECTION_NAME, mongo::BSONObj InputGeometry)
	{
		// log()<<"DeleteContainedGeoObj";
		int index_type = IO->Basic_Get_Index_Type(txn,DB_NAME, COLLECTION_NAME);
		// log()<<"index_type:"<<index_type;
		vector<mongo::OID> RefinedResultKeys;
		if (index_type > 0)
		{
			if (index_type == 1)
			{
				vector < mongo::OID >  ResultKeys;
				vector<bool> lazyIntersects;
				mongo::OID RootKey;
				string cn;
				int Max_Node = 0;
				int Max_Leaf = 0;
				if (IO->RTree_GetParms(txn,DB_NAME, COLLECTION_NAME, RootKey, Max_Node, Max_Leaf, cn))
				{
					 //parseGeometry
					geos::geom::Geometry * pGeometry = NULL;
					if (IO->ParseGeometry(InputGeometry, pGeometry) )//parseSuccess
					{
		              if (pGeometry->getGeometryType() == "MultiPolygon" || pGeometry->getGeometryType() == "Polygon")
					  {	  
					    _R.ReConfigure(Max_Node, Max_Leaf, RootKey, DB_NAME, COLLECTION_NAME);
					    _R.Search(pGeometry, ResultKeys, lazyIntersects);
		   			    for (unsigned int i = 0; i < ResultKeys.size(); i++)
					    {
						    if (IO->Geo_Verify_Contain(ResultKeys[i], pGeometry, lazyIntersects[i], DB_NAME, COLLECTION_NAME, cn))
					    	{
							    RefinedResultKeys.push_back(ResultKeys[i]);
						    }
					    }
					    for (unsigned int i = 0; i < RefinedResultKeys.size(); i++)
					    {
						    MBR datambr;
					    	IO->RTree_GetDataMBR(DB_NAME, COLLECTION_NAME, datambr, RefinedResultKeys[i], cn);
					    	_R.DeleteNode(txn,RootKey, RefinedResultKeys[i], datambr);
						    IO->RTree_ModifyRootKey(txn,DB_NAME, COLLECTION_NAME, RootKey);
					    	IO->Basic_Delete_One_SpatialObj_Only(txn,DB_NAME, COLLECTION_NAME, RefinedResultKeys[i]);
					    }
					  }
					}
				}
			}
			return 1;
		}
		return 0;
	}




	
	int IndexManagerBase::DropIndex(OperationContext* txn,string DB_NAME,string COLLECTIONNAME)
	{
		int index_type = IO->Basic_Get_Index_Type(txn,DB_NAME,COLLECTIONNAME);
		if (index_type <= 0)
			return 0;
		if (IO->RTree_DeleteIndex(txn,DB_NAME, COLLECTIONNAME))
			return 1;
		return 0;
	}

	/*
	    0: no index found, drop collection directly
		1: OK
		-1: unknown reason 
	*/
	
	int IndexManagerBase::DropCollection(OperationContext* txn,string DB_NAME,string COLLECTIONNAME)
	{
		int index_type = IO->Basic_Get_Index_Type(txn,DB_NAME,COLLECTIONNAME);
		if (index_type <= 0)
		{
			IO->Basic_Drop_Storage(txn,DB_NAME,COLLECTIONNAME);
			IO->Basic_DeleteOneGeoMeteData(txn,DB_NAME,COLLECTIONNAME);
			return 0;
		}
		else
		{
			IO->RTree_DeleteIndex(txn,DB_NAME,COLLECTIONNAME);
			IO->Basic_Drop_Storage(txn,DB_NAME,COLLECTIONNAME);
			IO->Basic_DeleteOneGeoMeteData(txn,DB_NAME, COLLECTIONNAME);
			return 1;
		}
		return -1;
	}

	
	int IndexManagerBase::RepairIndex(string DB_NAME,string COLLECTION_NAME)
	{

		return 1;
	}


	/*
	  -1:unsupported index type
	  0:faild because of meteData problem
	  1:success
	*/
	
	int IndexManagerBase::InsertIndexedDoc(OperationContext* txn,string DB_NAME, string COLLECTION_NAME, mongo::BSONObj AtomData, BSONObjBuilder& result)
	{
		int index_type = IO->Basic_Get_Index_Type(txn,DB_NAME,COLLECTION_NAME);
		if (index_type == 1)
		{
			mongo::OID RootKey;
			int Max_Node = 0;
			int Max_Leaf = 0;
			string cn;
			if (IO->RTree_GetParms(txn,DB_NAME,COLLECTION_NAME, RootKey, Max_Node, Max_Leaf, cn))
			{
				_R.ReConfigure(Max_Node,Max_Leaf,RootKey,DB_NAME,COLLECTION_NAME);
				
				MBR m;
				if (IO->RTree_GetDataMBR(AtomData, cn, m))
				{
					//store Data 1st because data is more important
					mongo::OID dataKeyAI;
					// log() << "Data2Insert:"<<AtomData << endl;
					IO->Basic_Store_One_Atom_Data(txn,DB_NAME, COLLECTION_NAME, AtomData, dataKeyAI, result);
					Branch b;
					b.ChildKey = dataKeyAI;
					b.HasData = true;
					b.mbr = m;
					_R.Insert(txn,RootKey, b, 0);

					IO->RTree_ModifyRootKey(txn,DB_NAME, COLLECTION_NAME, RootKey);
					return 1;
				}
			}
			return 0;
		}
		return -1;
	}
	

	
	bool IndexManagerBase::InitalizeManager(MongoIndexManagerIO *USER_INDEXMANAGER_IO, MongoIO *USER_RTREE_IO)
	{
		IO = USER_INDEXMANAGER_IO;
		_RIO = USER_RTREE_IO;
		_R.IO = _RIO;
		return true;
	}

	
}
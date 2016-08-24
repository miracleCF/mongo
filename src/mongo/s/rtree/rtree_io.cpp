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

#include "rtree_io.h"
// add log support for debug
#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kCommand
#include "mongo/util/log.h"
#include <iostream>
using namespace std; 
using namespace mongo;

namespace rtree_index
{
    stdx::mutex _connection_mu;

	
	
	
    rtree_index::MongoIO::MongoIO(DBClientBase *USER_CONN)
	{
		_conn = 0;
		//_conn = USER_CONN;
	}

	bool  rtree_index::MongoIO::connectMyself()
	{
		std::string errmsg;
		string url = "localhost:" + boost::lexical_cast<string>(serverGlobalParams.port);
		ConnectionString cs = ConnectionString::parse( url).getValue();
       // ConnectionString cs(ConnectionString::parse(connectionString));
        if (!cs.isValid()) {
           cout << "error parsing url: " << errmsg << endl;
           return false;
        }
		_conn= cs.connect(errmsg);
        if (!_conn) {
            cout << "couldn't connect: " << errmsg << endl;
			return false;
        }
		 
		return true;
	}

	bool rtree_index::MongoIO::IsConnected()
	{
		if (_conn != 0&&this->_conn->isStillConnected())
			return true;
		else
			return false;
	}

	Node rtree_index::MongoIO::Basic_Find_One_Node(mongo::OID k)
	{
#ifdef RTREECACHE
		//try to lock LRUCache_mu;
		{
			stdx::lock_guard<stdx::mutex> LRU_lock(LRUCache_mu);
			//get cache
			LRU2Cache * _Cache = LRU2Cache::getInstance();
			Node nodeInCache(_Max_Node);
			
			if (_Cache->Get(k, nodeInCache))
			{
				_Cache->Insert(nodeInCache);
				return nodeInCache;
			}
		}
#endif
		
		
		
		BSONObjBuilder bdr;
		bdr.append("_id", k);

		/*
		 * lock _conn first
		 * please note this _conn is shared by many querying threads
		 */
		BSONObj theNodeBSON;
	    { 
			stdx::lock_guard<stdx::mutex> CONN_lock(_connection_mu);
			if(!IsConnected())
			{
				connectMyself();
			}
		    theNodeBSON = _conn->findOne(_dbName + "." + "rtree_" + _Data_CollectionName, bdr.obj());
		}
	
		if (!theNodeBSON.isEmpty())
		{
			int Count = theNodeBSON["Count"].Int();
			int Level = theNodeBSON["Level"].Int();
			BSONObj AllBranches = theNodeBSON["Branchs"].Obj();
			vector<BSONElement> L;
			AllBranches.elems(L);
			
			rtree_index::Node theReturnNode(L.size());
			theReturnNode.Count = Count;
			theReturnNode.Level = Level;
			for (unsigned int i = 0; i < L.size(); i++)
			{
				BSONObj OneBranchBSON = L[i].Obj();
				theReturnNode.Branchs[i].HasData = OneBranchBSON["HasData"].boolean();
				theReturnNode.Branchs[i].ChildKey = OneBranchBSON["ChildID"].OID();
				BSONObj Values = OneBranchBSON["MBR"].Obj();
				vector<BSONElement> LL;
				Values.elems(LL);
				MBR m(LL[0].Double(), LL[1].Double(), LL[2].Double(), LL[3].Double());
				theReturnNode.Branchs[i].mbr = m;
			}
			
			
			//we get the node, we refresh the cache
#ifdef RTREECACHE
				//try to lock LRUCache_mu;
		 		{
					stdx::lock_guard<stdx::mutex> LRU_lock(LRUCache_mu);
					LRU2Cache * _Cache = LRU2Cache::getInstance();
					_Cache->Insert(theReturnNode);
				}
#endif
			
			return theReturnNode;
		}
		else
		{
			rtree_index::Node theReturnNode(_Max_Node>_Max_Leaf?_Max_Node:_Max_Leaf);
		}
		rtree_index::Node nullNode(4);
		return nullNode;

	}

	int rtree_index::MongoIO::Basic_Delect_Node_By_Key(OperationContext* txn,mongo::OID k,BSONObjBuilder& result)
	{
#ifdef RTREECACHE
		stdx::lock_guard<stdx::mutex> LRU_lock(LRUCache_mu);
		LRU2Cache * _Cache = LRU2Cache::getInstance();
		_Cache->Delete(k);
#endif
		
		
		BSONObjBuilder bdr;
		bdr.append("_id", k);
		bool ok;
		ok = RunWriteCommand(txn,_dbName, "rtree_" + _Data_CollectionName, bdr.obj(), REMOVE, result);
		/*remove*/
		//_conn->remove(_dbName + "." + _Data_CollectionName + "_RTreeIndex", bdr.obj());
		return 1;
	}

	mongo::OID rtree_index::MongoIO::Basic_Generate_Key()
	{
		/*gen*/
		return mongo::OID::gen();
	}

	int rtree_index::MongoIO::Basic_If_Exist_Collection(string StorageName)
	{
		/*exist*/
		if(!IsConnected())
		{
				connectMyself();
		}
		return _conn->exists(_dbName + "." + StorageName);
	}

	int rtree_index::MongoIO::Basic_Store_Node(OperationContext* txn,Node data)
	{
#ifdef RTREECACHE
		{
			stdx::lock_guard<stdx::mutex> LRU_lock(LRUCache_mu);
			LRU2Cache * _Cache = LRU2Cache::getInstance();
			_Cache->Insert(data);
			
		}
#endif
		
		
		BSONObjBuilder Nodebdr;
		Nodebdr.append("_id", data.NodeKey);
		Nodebdr.append("Level", data.Level);
		Nodebdr.append("Count", data.Count);

		BSONArrayBuilder arrbuilder;
		int MaxNum = data.Level > 0 ? _Max_Node : _Max_Leaf;
		for (int i = 0; i < MaxNum; i++)
		{
			BSONObjBuilder bb;
			bb.append("HasData", data.Branchs[i].HasData);
			BSONArrayBuilder arr;
			arr.append(data.Branchs[i].mbr.MinX);
			arr.append(data.Branchs[i].mbr.MinY);
			arr.append(data.Branchs[i].mbr.MaxX);
			arr.append(data.Branchs[i].mbr.MaxY);
			bb.append("MBR", arr.arr());
			bb.append("ChildID", data.Branchs[i].ChildKey);
			arrbuilder.append(bb.obj());//��ʵbb����һ��branch
		}
		Nodebdr.append("Branchs", arrbuilder.arr());
        
		/*insert*/
		bool ok;
		BSONObjBuilder bdr;
	    BSONObjBuilder & bdrRef=bdr;
		BSONObj insertObj=Nodebdr.obj();
		log()<<"starting to insert data into collection!";
		ok = RunWriteCommand(txn,_dbName, "rtree_" + _Data_CollectionName,insertObj, INSERT,bdrRef);
        
		return 1;
	}

	bool rtree_index::MongoIO::RTree_ModifyBranch(OperationContext* txn,mongo::OID targetOID, Branch branch2modify, int i)
	{
        bool IsFoundInLRUandHisList = false;
		Node possibleSnapShot = Node(0);
#ifdef RTREECACHE
		{
			stdx::lock_guard<stdx::mutex> LRU_lock(LRUCache_mu);
			LRU2Cache * _Cache = LRU2Cache::getInstance();
			IsFoundInLRUandHisList = _Cache->ModifyBranch(targetOID, branch2modify, i);
			if (IsFoundInLRUandHisList)
			{
				_Cache->Visit(targetOID, possibleSnapShot);
			}
		}
#endif
		
		
		BSONObjBuilder bb;
		bb.append("HasData", branch2modify.HasData);
		BSONArrayBuilder arr;
		arr.append(branch2modify.mbr.MinX);
		arr.append(branch2modify.mbr.MinY);
		arr.append(branch2modify.mbr.MaxX);
		arr.append(branch2modify.mbr.MaxY);
		bb.append("MBR", arr.obj());
		bb.append("ChildID", branch2modify.ChildKey);

		BSONObjBuilder condition;
		condition.append("_id", targetOID);
		BSONObjBuilder setBuilder;
		BSONObjBuilder setConditionBuilder;
		string s = "Branchs.";
		std::stringstream ss;
		std::string str;
		ss << i;
		ss >> str;
		s += str;
		setConditionBuilder.append(s, bb.obj());
		setBuilder.append("$set", setConditionBuilder.obj());

		BSONObjBuilder cmdObj;
		cmdObj.append("query", condition.obj());
		cmdObj.append("update", setBuilder.obj());
		/*��Ҫ�޸ĳ�����*/
		//_conn->update(_dbName + "." + _Data_CollectionName + "_RTreeIndex", Query(condition.obj()), setBuilder.obj());
		bool ok;
		
		BSONObjBuilder bdr;
	    BSONObjBuilder & bdrRef=bdr;
		ok = RunWriteCommand(txn,_dbName, "rtree_" + _Data_CollectionName, cmdObj.obj(), UPDATE,bdrRef);

		return ok;

	}

	bool rtree_index::MongoIO::RTree_ModifyCount(OperationContext* txn,mongo::OID targetOID, int count2modify)
	{
#ifdef RTREECACHE
		Node possibleSnapShot = Node(0);
		{
		    stdx::lock_guard<stdx::mutex> LRU_lock(LRUCache_mu);
			LRU2Cache * _Cache = LRU2Cache::getInstance();
			bool IsFoundInLRUandHisList = _Cache->ModifyCount(targetOID, count2modify);
			if (IsFoundInLRUandHisList)
			{
				_Cache->Visit(targetOID, possibleSnapShot);
			}
		}
#endif		

		BSONObjBuilder condition;
		condition.append("_id", targetOID);
		BSONObjBuilder setBuilder;
		BSONObjBuilder setConditionBuilder;
		setConditionBuilder.append("Count", count2modify);
		setBuilder.append("$set", setConditionBuilder.obj());

		BSONObjBuilder cmdObj;
		cmdObj.append("query", condition.obj());
		cmdObj.append("update", setBuilder.obj());
		/*��Ҫ�޸ĳ�����*/
		//_conn->update(_dbName + "." + _Data_CollectionName + "_RTreeIndex", Query(condition.obj()), setBuilder.obj());
		bool ok;
		BSONObjBuilder bdr;
		BSONObjBuilder & bdrRef=bdr;
		ok = RunWriteCommand(txn,_dbName, "rtree_" + _Data_CollectionName, cmdObj.obj(), UPDATE,bdrRef);

		return ok;
	}

	int rtree_index::MongoIO::RTree_Calculate_Data_Branch_Count(mongo::OID targetOID)
	{
int nodeInCacheBranchCount = 0;
#ifdef RTREECACHE
{
	        // cout<<"calculate count 1"<<endl;
		    stdx::lock_guard<stdx::mutex> LRU_lock(LRUCache_mu);
			Node nodeInCache = Node(0);
			LRU2Cache * _Cache = LRU2Cache::getInstance();
			if (_Cache->Get(targetOID, nodeInCache))
			{
				for (int i = 0; i < _Max_Node; i++)
				{
					if (nodeInCache.Branchs[i].HasData)
					{
						nodeInCacheBranchCount++;
					}
				}
				_Cache->Insert(nodeInCache);//flash一下缓存
				return nodeInCacheBranchCount;
			}	
	        // cout<<"calculate count 1 over"<<endl;
			
}
#endif
	
	
	
		BSONObjBuilder bdr;
		bdr.append("_id", targetOID);
        BSONObj theNodeBSON;
		/*findoOne*/
		{
	    	stdx::lock_guard<stdx::mutex> CONN_lock(_connection_mu);
			if(!IsConnected())
		    {
				connectMyself();
		    }
		    theNodeBSON = _conn->findOne(_dbName + "." + "rtree_" + _Data_CollectionName, bdr.obj());
		}

#ifdef RTREECACHE 
// add to cache
        if (!theNodeBSON.isEmpty())
		{
			int Count = theNodeBSON["Count"].Int();
			int Level = theNodeBSON["Level"].Int();
			BSONObj AllBranches = theNodeBSON["Branchs"].Obj();
			vector<BSONElement> L;
			AllBranches.elems(L);
			//�½�һ��Node
			RTreeIndex::Node theReturnNode(L.size());
			theReturnNode.Count = Count;
			theReturnNode.Level = Level;
			for (unsigned int i = 0; i < L.size(); i++)
			{
				BSONObj OneBranchBSON = L[i].Obj();
				theReturnNode.Branchs[i].HasData = OneBranchBSON["HasData"].boolean();
				theReturnNode.Branchs[i].ChildKey = OneBranchBSON["ChildID"].OID();
				BSONObj Values = OneBranchBSON["MBR"].Obj();
				vector<BSONElement> LL;
				Values.elems(LL);
				MBR m(LL[0].Double(), LL[1].Double(), LL[2].Double(), LL[3].Double());
				theReturnNode.Branchs[i].mbr = m;
			}
			//try to lock LRUCache_mu;
			{
	            // cout<<"calculate count 2"<<endl;
				stdx::lock_guard<stdx::mutex> LRU_lock(LRUCache_mu);
				LRU2Cache * _Cache = LRU2Cache::getInstance();
				_Cache->Insert(theReturnNode);
	            //  cout<<"calculate count 2 over"<<endl;
				
			}
		}		
#endif
		
		int Count = 0;
		BSONObj AllBranches = theNodeBSON["Branchs"].Obj();
		vector<BSONElement> L;
		AllBranches.elems(L);
		for (unsigned int i = 0; i < L.size(); i++)
		{
			if (L[i].Obj()["HasData"].boolean())
			{
				Count++;
			}
		}
		return Count;

	}

	bool rtree_index::MongoIO::RTree_ModifyAllBranch(OperationContext* txn,mongo::OID targetOID, vector<Branch> AllBranchs)
	{
bool IsFoundInLRUandHisList = false;
Node possibleSnapShot = Node(0);
#ifdef RTREECACHE
		{
			stdx::lock_guard<stdx::mutex> LRU_lock(LRUCache_mu);
			LRU2Cache * _Cache = LRU2Cache::getInstance();
			IsFoundInLRUandHisList = _Cache->ModifyAllBranch(targetOID, AllBranchs);
			if (IsFoundInLRUandHisList)
			{
				_Cache->Visit(targetOID, possibleSnapShot);
			}
		}
#endif
		BSONArrayBuilder Branchs;

		for (unsigned int i = 0; i < AllBranchs.size(); i++)
		{
			BSONObjBuilder branchbdr;
			branchbdr.append("HasData", AllBranchs[i].HasData);
			BSONArrayBuilder arr;
			arr.append(AllBranchs[i].mbr.MinX);
			arr.append(AllBranchs[i].mbr.MinY);
			arr.append(AllBranchs[i].mbr.MaxX);
			arr.append(AllBranchs[i].mbr.MaxY);
			branchbdr.append("MBR", arr.arr());
			branchbdr.append("ChildID", AllBranchs[i].ChildKey);
			Branchs.append(branchbdr.obj());
		}

		BSONObjBuilder condition;
		condition.append("_id", targetOID);
		BSONObjBuilder setBuilder;
		BSONObjBuilder setConditionBuilder;
		string s = "Branchs";
		setConditionBuilder.append(s, Branchs.arr());
		setBuilder.append("$set", setConditionBuilder.obj());

		BSONObjBuilder cmdObj;
		cmdObj.append("query", condition.obj());
		cmdObj.append("update", setBuilder.obj());
		/*������Ҫʹ������*/
		//_conn->update(_dbName + "." + _Data_CollectionName + "_RTreeIndex", Query(condition.obj()), setBuilder.obj());
		bool ok;
		
		BSONObjBuilder bdr;
		BSONObjBuilder & bdrRef=bdr;
		
		ok = RunWriteCommand(txn,_dbName, "rtree_" + _Data_CollectionName, cmdObj.obj(), UPDATE,bdrRef);

		return ok;
	}
    
	bool rtree_index::MongoIO::Configure(string DB_NAME, string STORAGE_NAME, int Max_Node, int Max_Leaf)
	{
		_dbName = DB_NAME;
		_Data_CollectionName = STORAGE_NAME;
		_Max_Node = Max_Node; 
		_Max_Leaf = Max_Leaf;
		return true;
	}
    
}
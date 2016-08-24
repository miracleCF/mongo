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
#include <vector>
#include "rtree_data_structure.h"
#if defined(_WIN32)
#include <winsock2.h>
#include <windows.h>
#endif
#include <boost/lexical_cast.hpp> 
#include "write_op.h"

#define RTREECACHE

#ifdef RTREECACHE
#include "rtree_cachelru2.h"
#endif


using namespace std; 
using namespace mongo;


namespace rtree_index
{
	extern stdx::mutex _connection_mu;

	/**
	 *Some operations in rtree are defined here
	 *
	 */
	class MongoIO 
	{
	public:
		MongoIO(DBClientBase *USER_CONN);
		Node Basic_Find_One_Node(mongo::OID k);
		int Basic_Delect_Node_By_Key(OperationContext* txn,mongo::OID k,BSONObjBuilder& result);
		mongo::OID Basic_Generate_Key();
		int Basic_If_Exist_Collection(string StorageName);
		int Basic_Store_Node(OperationContext* txn,Node data);
		bool RTree_ModifyBranch(OperationContext* txn,mongo::OID targetOID, Branch branch2modify, int i);
		bool RTree_ModifyCount(OperationContext* txn,mongo::OID targetOID, int count2modify);
		int RTree_Calculate_Data_Branch_Count(mongo::OID targetOID);
		bool RTree_ModifyAllBranch(OperationContext* txn,mongo::OID targetOID, vector<Branch> AllBranchs);
		bool Configure(string DB_NAME, string STORAGE_NAME, int Max_Node, int Max_Leaf);
		/**
		 *Connect to database
		 */
		bool connectMyself();
		bool IsConnected();
	private:
		string _dbName;
		string _Data_CollectionName;
		int _Max_Node;
		int _Max_Leaf;
		DBClientBase*   _conn;

	};

	
}
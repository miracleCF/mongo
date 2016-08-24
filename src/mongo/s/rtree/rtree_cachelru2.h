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

#include <list>
#include "rtree_data_structure.h"

//boost lib for mutex
// #include <boost/thread/thread.hpp>  
// #include <boost/thread/mutex.hpp>  
// #include <boost/bind.hpp>  

#include "mongo/stdx/mutex.h"
#include "write_op.h"
using namespace std;


using namespace mongo;


namespace rtree_index
{
     extern  stdx::mutex LRUCache_mu;
	 extern  stdx::mutex LRUCache_getInstance_mu;

	//LRU2Cache
	class LRU2Cache
	{
	public:
		static LRU2Cache* getInstance();
		void InitStatics();
		void Init(int MaxLRUSize, int MaxHistoricalSize);
		bool Get(mongo::OID TARGET_KEY, Node & returnNode);
		bool Insert(Node VisitedNode);
		//Please fill this code
		bool Visit(mongo::OID TARGET_KEY,Node &returnNode);
		bool Update(Node UpdatedNode);
		bool Delete(mongo::OID TARGET_KEY);

		bool ModifyCount(mongo::OID targetOID, int count2modify);
		bool ModifyBranch(mongo::OID targetOID, Branch branch2modify, int i);
		bool ModifyAllBranch(mongo::OID targetOID, vector<Branch> AllBranchs);
		//operator <<
		void PrintFunction();
	private:
		int _max_lru_size;
		int _max_historical_size;
		list<Node> LRUList;
		list<Node> HistoricalList;
		LRU2Cache();
		LRU2Cache(int MaxLRUSize, int MaxHistoricalSize);

		static LRU2Cache* _instance;

	};


}
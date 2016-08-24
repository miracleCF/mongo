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

#include "rtree_cachelru2.h"
using namespace mongo;
using namespace std;


namespace rtree_index
{
    
	
	LRU2Cache* LRU2Cache::_instance = 0;
    stdx::mutex LRUCache_mu;
	stdx::mutex LRUCache_getInstance_mu;
	//implement
	
	void rtree_index::LRU2Cache::PrintFunction()
	{
		cout << "******************************************LRU" << endl;
		list<Node>::iterator Itor;
		for (Itor = LRUList.begin(); Itor != LRUList.end(); Itor++)
		{
			cout << Itor->NodeKey<<endl;
		}
		cout << "******************************************HIS" << endl;
		for (Itor = HistoricalList.begin(); Itor != HistoricalList.end(); Itor++)
		{
			cout << Itor->NodeKey << endl;
		}
		cout << "******************************************END" << endl;
	}

	
	rtree_index::LRU2Cache::LRU2Cache()
	{
		_max_lru_size = 1000;
		_max_historical_size = 3000;
		LRUList.clear();
		HistoricalList.clear();
	}

	
	LRU2Cache* rtree_index::LRU2Cache::getInstance()
	{
		stdx::lock_guard<stdx::mutex> LRUGetInstance_lock(LRUCache_getInstance_mu);
		if (_instance == NULL)
		{
			_instance = new LRU2Cache();
		}
		return _instance;
	}


	
	rtree_index::LRU2Cache::LRU2Cache(int MaxLRUSize, int MaxHistoricalSize)
	{
		_max_lru_size = MaxLRUSize;
		_max_historical_size = MaxHistoricalSize;
		LRUList.clear();
		HistoricalList.clear();
	}

	
	void  rtree_index::LRU2Cache::Init(int MaxLRUSize, int MaxHistoricalSize)
	{
		_max_lru_size = MaxLRUSize;
		_max_historical_size = MaxHistoricalSize;
		LRUList.clear();
		HistoricalList.clear();
	}



	
	bool  rtree_index::LRU2Cache::Get(mongo::OID TARGET_KEY, Node & returnNode)
	{
		list<Node>::iterator Itor;
		for (Itor = LRUList.begin(); Itor != LRUList.end(); Itor++)
		{
			if (Itor->NodeKey == TARGET_KEY)
			{
				returnNode = *Itor;
				return true;
			}
		}
		return false;
	}

	//AKA visit
	
	bool rtree_index::LRU2Cache::Insert(Node VisitedNode)
	{
		list<Node>::iterator Itor1;
		for (Itor1 = LRUList.begin(); Itor1 != LRUList.end(); Itor1++)
		{
			if (Itor1->NodeKey == VisitedNode.NodeKey)
			{
				Node NewNode=*Itor1;//copy
				LRUList.erase(Itor1);
				LRUList.push_front(NewNode);
				return true;//must break to prevent list break
			}
		}
		//not in the LRUlist but in HistoricalList
		list<Node>::iterator Itor;
		for (Itor = HistoricalList.begin(); Itor != HistoricalList.end(); Itor++)
		{
			if (Itor->NodeKey == VisitedNode.NodeKey)
			{
				Node NewNode(*Itor);
				HistoricalList.erase(Itor);
				//add to LRUList;
				if (LRUList.size()  >= static_cast<unsigned int>(_max_lru_size))
				{
					LRUList.pop_back();
				}
				LRUList.push_front(NewNode);
				return true;
			}
		}
		//no historical record found
		if (HistoricalList.size() >= static_cast<unsigned int>(_max_historical_size))
		{
			HistoricalList.pop_back();
		}
		HistoricalList.push_front(VisitedNode);
		return true;
	}

	
	bool rtree_index::LRU2Cache::Visit(mongo::OID TARGET_KEY, Node &returnNode)
	{
		list<Node>::iterator Itor1;
		for (Itor1 = LRUList.begin(); Itor1 != LRUList.end(); Itor1++)
		{
			if (Itor1->NodeKey == TARGET_KEY)
			{
				Node NewNode(*Itor1);//copy
				returnNode = *Itor1;
				LRUList.erase(Itor1);
				LRUList.push_front(NewNode);
				return true;//must break to prevent list break
			}
		}
		//not in the LRUlist but in HistoricalList
		list<Node>::iterator Itor;
		for (Itor = HistoricalList.begin(); Itor != HistoricalList.end(); Itor++)
		{
			if (Itor->NodeKey == TARGET_KEY)
			{
				Node NewNode(*Itor);
				returnNode = *Itor;
				HistoricalList.erase(Itor);
				//add to LRUList;
				if (LRUList.size() >=static_cast<unsigned int>( _max_lru_size))
				{
					LRUList.pop_back();
				}
				LRUList.push_front(NewNode);
				return true;
			}
		}
		//no historical record found
		if (HistoricalList.size() >= static_cast<unsigned int> (_max_historical_size))
		{
			HistoricalList.pop_back();
		}
		return false;
	}


	
	bool rtree_index::LRU2Cache::Update(Node UpdatedNode)
	{
		list<Node>::iterator Itor1;
		for (Itor1 = LRUList.begin(); Itor1 != LRUList.end(); Itor1++)
		{
			if (Itor1->NodeKey == UpdatedNode.NodeKey)
			{
				*Itor1 = UpdatedNode;
				return true;
			}
		}
		//not in the LRUlist but in HistoricalList
		list<Node>::iterator Itor;
		for (Itor = HistoricalList.begin(); Itor != HistoricalList.end(); Itor++)
		{
			if (Itor->NodeKey == UpdatedNode.NodeKey)
			{
				*Itor = UpdatedNode;
				return true;
			}
		}
		return false;
	}

	
	bool rtree_index::LRU2Cache::Delete(mongo::OID TARGET_KEY)
	{
		list<Node>::iterator Itor1;
		for (Itor1 = LRUList.begin(); Itor1 != LRUList.end(); Itor1++)
		{
			if (Itor1->NodeKey == TARGET_KEY)
			{
				LRUList.erase(Itor1);
				return true;//must break to prevent list break
			}
		}
		//not in the LRUlist but in HistoricalList
		list<Node>::iterator Itor;
		for (Itor = HistoricalList.begin(); Itor != HistoricalList.end(); Itor++)
		{
			if (Itor->NodeKey == TARGET_KEY)
			{
				HistoricalList.erase(Itor);
				return true;
			}
		}
		return false;
	}

	
	bool rtree_index::LRU2Cache::ModifyCount(mongo::OID targetOID, int count2modify)
	{
		list<Node>::iterator Itor1;
		for (Itor1 = LRUList.begin(); Itor1 != LRUList.end(); Itor1++)
		{
			if (Itor1->NodeKey == targetOID)
			{
				Itor1->Count = count2modify;
				return true;
			}
		}
		//not in the LRUlist but in HistoricalList
		list<Node>::iterator Itor;
		for (Itor = HistoricalList.begin(); Itor != HistoricalList.end(); Itor++)
		{
			if (Itor->NodeKey == targetOID)
			{
				Itor->Count = count2modify;
				return true;
			}
		}
		return false;
	}

	
	bool rtree_index::LRU2Cache::ModifyBranch(mongo::OID targetOID, Branch branch2modify, int i)
	{
		list<Node>::iterator Itor1;
		for (Itor1 = LRUList.begin(); Itor1 != LRUList.end(); Itor1++)
		{
			if (Itor1->NodeKey == targetOID)
			{
				Itor1->Branchs[i] = branch2modify;
				return true;
			}
		}
		//not in the LRUlist but in HistoricalList
		list<Node>::iterator Itor;
		for (Itor = HistoricalList.begin(); Itor != HistoricalList.end(); Itor++)
		{
			if (Itor->NodeKey == targetOID)
			{
				Itor->Branchs[i] = branch2modify;
				return true;
			}
		}
		return false;
	}

	
	bool rtree_index::LRU2Cache::ModifyAllBranch(mongo::OID targetOID, vector<Branch> AllBranchs)
	{
		list<Node>::iterator Itor1;
		for (Itor1 = LRUList.begin(); Itor1 != LRUList.end(); Itor1++)
		{
			if (Itor1->NodeKey == targetOID)
			{
				Itor1->Branchs.swap(AllBranchs);
				return true;
			}
		}
		//not in the LRUlist but in HistoricalList
		list<Node>::iterator Itor;
		for (Itor = HistoricalList.begin(); Itor != HistoricalList.end(); Itor++)
		{
			if (Itor->NodeKey == targetOID)
			{
				Itor->Branchs.swap(AllBranchs);
				return true;
			}
		}
		return false;
	}
    
    
}
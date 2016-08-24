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

/**
 * Welcome to rtree_geonear_cursor.cpp
 */


#include "rtree_geonear_cursor.h"

using namespace rtree_index;
using namespace mongo;
using namespace index_manager;

namespace rtree_index
{
    // public functions for cursor
    bool RTreeGeoNearSearchNodeCompare(GeoNearSearchNode *node1, GeoNearSearchNode *node2)
	{
		if (node1->minDistance < node2->minDistance)
		{
			return true;
		}
		else
		{
			if (node1->minDistance == node2->minDistance)
			{
				if (node1->maxDistance < node2->maxDistance)
				{
					return true;
				}
			}

			return false;
		}
	}


	double _cDistance(double x1, double y1, double x2, double y2)
	{
		//  the E-distance between the 2 points, I'm thinking if I can remove the sqrt() function to 
		//  boost the performance
		double f1 = (x1 - x2);
		double f2 = f1 * f1;
		double f3 = (y1 - y2);
		double f4 = f3 * f3;
		return sqrt(f2 + f4);
	}

	double _MinDistanceFromMBR(double x, double y, MBR m)
	{
		//find the minDistance between the MBR and the search point(2d)
		//the plot should be something line this
		/*
				*                         *
		1		*           2             *      3
				*                         *
				*                         *
		*******************************************  v[3]
				*                         *
				*                         *
		4		*            0            *      5
				*                         *
		*******************************************  v[1]
				*                         *
		6		*            7            *      8
				*                         *
				*                         *
				v[0]                      v[2]


		*/

		//if the search point is contained by the MBR
		if (m.MinX <= x&&m.MaxX >= x&&m.MinY <= y&&m.MaxY >= y)
		{
			//0
			return 0;
		}
		if (x >= m.MinX && x <= m.MaxX)
		{
			//2 or 7
			if (y >= m.MaxY)
			{
				//2
				return (y - m.MaxY);
			}
			else
			{
				//7
				return (m.MinY - y);
			}
		}
		if (y >= m.MinY && y <= m.MaxY)
		{
			//4 or 5
			if (x <= m.MinX)
			{
				//4
				return (m.MinX - x);
			}
			else
			{
				return (x - m.MaxX);
			}
		}
		if (x < m.MinX)
		{
			//1 or 6
			if (y > m.MaxY)
			{
				//1
				return _cDistance(x, y, m.MinX, m.MaxY);
			}
			else
			{
				//6
				return _cDistance(x, y, m.MinX, m.MinY);
			}
		}
		if (x > m.MaxX)
		{
			//3 or 8
			if (y > m.MaxY)
			{
				//3 
				return _cDistance(x, y, m.MaxX, m.MaxY);
			}
			else
			{
				return _cDistance(x, y, m.MaxX, m.MinY);
			}
		}
		
		return 0;//this is not supposed to happen.


	}

	double _MaxDistanceFromMBR(double x, double y, MBR m)
	{
		// Find the maxDistance between MBR and point, the plot should make it easier
		// to read 
		/*
				*           |             *
		1		*           |             *      2
				*           |             *
				*           |             *
		********************|**********************  v[3]
				*           |             *
				*           |             *
		------------------------------------------------
				*           |             *
		********************|**********************  v[1]
				*           |             *
		3		*           |             *      4
				*           |             *
				*           |             *
				v[0]                      v[2]


		*/
		if (x < (m.MinX + m.MaxX) / 2)
		{
			//1 or 3
			if (y >(m.MaxY + m.MinY) / 2)
			{
				//1
				return _cDistance(x, y, m.MaxX, m.MinY);
			}
			else
			{
				return _cDistance(x, y, m.MaxX, m.MaxY);
			}
		}
		else
		{
			//2 or 4
			if (y > (m.MaxY + m.MinY) / 2)
			{
				//2
				return _cDistance(x, y, m.MinX, m.MinY);
			}
			else
			{
				//4
				return _cDistance(x, y, m.MinX, m.MaxY);
			}
		}
	}

    //don't use this constructurer;
	rtree_index::RTreeGeoNearCurosr::RTreeGeoNearCurosr()
	{
			_closestBarrel.clear();
			_max_node = 32;
			_root_node = NULL;
			_IO = NULL;
			_DataIO = NULL;
	}

	rtree_index::RTreeGeoNearCurosr::RTreeGeoNearCurosr(int Max_Node, GeoNearSearchNode *RootNode, MongoIO *IO, MongoIndexManagerIO *DataIO, double ctx, double cty, double minDistance, double maxDistance, string DB_NAME, string COLLECTION_NAME, string COLUMN_NAME)
	{
			_closestBarrel.clear();
			_minDistance = minDistance;
			_maxDistance = maxDistance;
			_max_node = Max_Node;
			_root_node = RootNode;
			_IO = IO;
			_DataIO = DataIO;
			_ctx = ctx;
			_cty = cty;
			_db_name = DB_NAME;
			_collection_name = COLLECTION_NAME;
			_column_name = COLUMN_NAME;
		}



	/*
	 *	NNC, AKA Nearest Neighbour Cluster, is the minimum R-tree node set to pin point the nearest
	 *	spatial object e.g. ↓
	 *	here are 4 R-tree nodes: A, B, C and D, their distances from the search point is shown in 
	 *	the plot ↓ 
	 *		
	 *												|-------|
	 *												|		|
	 *												|	D	|
	 *												|	×	|
	 *	D								|-------|	|		|
	 *	I								|		|	|-------|
	 *	S					|-------|	|	C	|
	 *	T					|		|	|	√	|
	 *	A	----|-------|---|---B---|---|-------|--------------------- MBR (D) can't meet the condition of B.MinDistance < A.MaxDistance
	 *	N		|		|	|	√	|	|-------|
	 *	C		|	A	|	|		|
	 *	E		|	√	|	|-------|
	 *		----|-------|--------------------------------------------- Search Start Distance: _CurrentMinDistnace
	 *	S
	 *	P
	 *	A
	 *	C                           P-L-O-T (1)
	 *	E
	 *	
	 *	the _CurrentMinDistance is shown above as --------------------------------------
	 *	We can not decide the spatial object with distance is in node A, because the distance we use is vague (just distance of MBR),
	 *	We added A,B,C (C or B.maxdistance<A.mindistance, but D is not) to the _closestBarrel[Level], the closest spatial object
	 *	must in the _closestBarrel[0],
	 *	in thsi circumstances
	 *	NNC ={A,B,C} 
	 *	because R-tree node has Level attribute, we have to calculate each level's NNC
	 *  form the _root_node level to the level-0w
	 *	
	 */
	vector<GeoNearSearchNode *> rtree_index::RTreeGeoNearCurosr::GetNNCInLevel(vector<GeoNearSearchNode *> NNCInUpperLevel, int Level)
	{

			vector<GeoNearSearchNode * > returnNNCInLevel;
			vector<GeoNearSearchNode * > cmpVector;

			for (unsigned int i = 0; i < NNCInUpperLevel.size(); i++)
			{
				if (NNCInUpperLevel[i]->active_type == -1)
				{
					ExpendSingleNode(NNCInUpperLevel[i]);
					Trim(NNCInUpperLevel[i]);
				}
			}
			for (unsigned int i = 0; i < NNCInUpperLevel.size(); i++)
			{
				for (unsigned int j = 0; j < NNCInUpperLevel[i]->ChildPointers.size(); j++)
				{
					if (NNCInUpperLevel[i]->ChildPointers[j] != NULL)
					{
						cmpVector.push_back(NNCInUpperLevel[i]->ChildPointers[j]);
					}
				}
			}
			
			if(cmpVector.size()==0)
			{
				return cmpVector;
			}

			sort(cmpVector.begin(), cmpVector.end(), RTreeGeoNearSearchNodeCompare);

			_cmpVectors[Level] = cmpVector;

			GeoNearSearchNode * NN = cmpVector[0];
			returnNNCInLevel.push_back(NN);
			if (cmpVector.size() > 1)
			{
				for (unsigned int i = 1; i < cmpVector.size(); i++)
				{
					if (cmpVector[i]->minDistance <= NN->maxDistance)
					{
						returnNNCInLevel.push_back(cmpVector[i]);
					}
					else
					{
						break;
					}
				}
			}
			return returnNNCInLevel;



	}

	
	bool rtree_index::RTreeGeoNearCurosr::ExpendSingleNode(GeoNearSearchNode *Node2Expend)
	{
			if (Node2Expend != NULL && Node2Expend->data.Level > 0)// It's a tree node rather then a data node
			{
				for (unsigned int i = 0; i < Node2Expend->data.Branchs.size(); i++)
				{
					if (Node2Expend->data.Branchs[i].HasData == true)
					{
						Node oneNode = _IO->Basic_Find_One_Node(Node2Expend->data.Branchs[i].ChildKey);
						GeoNearSearchNode *newNode = new GeoNearSearchNode();
						newNode->data = oneNode;
						CalculateDistances(newNode, Node2Expend->data.Branchs[i].mbr);
						newNode->active_type = -1;
						//set pointer
						newNode->parent = Node2Expend;
						newNode->ChildIDInParent = i;
						Node2Expend->ChildPointers.push_back(newNode);
					}
					else
					{
						Node2Expend->ChildPointers.push_back(NULL);
					}
				}
				Node2Expend->active_type = 0;//expended
				return true;
			}
			return false;
		}

	void rtree_index::RTreeGeoNearCurosr::CalculateDistances(GeoNearSearchNode * node2Calculate, MBR m)
	{
			node2Calculate->minDistance = _MinDistanceFromMBR(_ctx, _cty, m);
			node2Calculate->maxDistance = _MaxDistanceFromMBR(_ctx, _cty, m);
		}

	
	bool rtree_index::RTreeGeoNearCurosr::DeleteNode(GeoNearSearchNode * node2Delete)
	{
		GeoNearSearchNode *parentNode = node2Delete->parent;
		if (parentNode != NULL)
		{
			parentNode->ChildPointers[node2Delete->ChildIDInParent] = NULL;
			if (node2Delete->parent->active_type == 1)
			{
				node2Delete->parent->NumberOfNode--;
			}
		}
		delete(node2Delete);
		return true;
	}

	
	bool rtree_index::RTreeGeoNearCurosr::Trim(GeoNearSearchNode *Node2Trim)
	{
			if (Node2Trim->active_type == -1) //means this node is unexpended. It is impossible
			{
				return false;
			}
			int NumOfNN = 0;
			for (unsigned int i = 0; i < Node2Trim->ChildPointers.size(); i++)
			{
				if (Node2Trim->ChildPointers[i] != NULL)
				{
					//distance has been calculated during the Expend process;
					//just trim
					if (Node2Trim->ChildPointers[i]->minDistance>_maxDistance || Node2Trim->ChildPointers[i]->maxDistance < _minDistance)
					{
						DeleteNode(Node2Trim->ChildPointers[i]);
					}
					else
					{
						NumOfNN++;
					}
				}
			}
			Node2Trim->NumberOfNode = NumOfNN;
			//if the node is trimed, we set the active_type = 1
			Node2Trim->active_type = 1;
			return true;

	}

	
	bool rtree_index::RTreeGeoNearCurosr::InitCursor()
	{
			int Level = _root_node->data.Level;
			_max_level = Level;
			for ( int i = 0; i < Level; i++)
			{
				vector<GeoNearSearchNode *> oneLevelofNNC;
				_closestBarrel.push_back(oneLevelofNNC);
			}
			vector<GeoNearSearchNode  *> initiator;
			initiator.push_back(_root_node);
			_closestBarrel.push_back(initiator);

			int thisLevel = Level;
			for ( int i = 0; i <= Level; i++)
			{
				vector<GeoNearSearchNode *> temp;
				_cmpVectors.push_back(temp);
				_CurrentDistanceIDcmpVectors.push_back(0);
				_Need2ReformCmpVectors.push_back(false);
			}
			while (thisLevel > 0)
			{
				vector<GeoNearSearchNode *> oneLevelofNNC = GetNNCInLevel(_closestBarrel[thisLevel], thisLevel - 1);
				if(oneLevelofNNC.size()>0)
				{
	    			_closestBarrel[thisLevel - 1] = oneLevelofNNC;
    				thisLevel--;
				}
				else
				{
					break;
				}
			}
			return true;

	}

	
	
	mongo::BSONObj rtree_index::RTreeGeoNearCurosr::Next()
	{
		//if _closestBarrel[0].size()==0, it means there are no NNC found, no extra result was found.
		if (_closestBarrel[0].size() == 0)
		{
			return mongo::BSONObj();
		}
		else
		{
			/** we start at level 0, load the spatial data (entries) from database to memory.
			  * calculate the real distance between entiries and query point
			  * and rule out inrevelant nodes
			 */
			for (unsigned int i = 0; i < _closestBarrel[0].size(); i++)
			{
				if (_closestBarrel[0][i]->DistanceSortList.size() == 0 && _closestBarrel[0][i]->CurrentDistanceID!=-2)
				{
					FillTheDataChilds(_closestBarrel[0][i]);
				}
			}

			/**
			 * It is possible that the MBR of the Node (Level==0) satisfies and query condtion ($minDistance and $maxDistance)
			 * while none of their childs (spatial data entries) meets the distance requirement.
			 * so we have to remove this node from _closestBarrel[0] as well as in _cmpVectors[0]
			 */
			for (int i = 0; i < static_cast<int> (_closestBarrel[0].size()); i++)
			{
				if ( _closestBarrel[0][i]->CurrentDistanceID == -2)
				{
					GeoNearSearchNode *node2Delete = _closestBarrel[0][i];
					RemoveNodeInCmpVectorAndclosestBarrel(node2Delete,0);
					if (i < _CurrentDistanceIDcmpVectors[0])
					{
						_CurrentDistanceIDcmpVectors[0]--;
					}
					i--;
				}
			}
            /**
			 * It is also possible that all nodes in _closestBarrel[0] has been removed
			 * No result will be found, we just return an empty BSONObj.
			 */
            if(_closestBarrel[0].size()==0)
			{
				//we J-Just can't return, we need a hard refresh
				int pLevel = _max_level;
				while (pLevel >= 0)
				{
					RefreshNNCInLevel(pLevel);
					pLevel--;
				}
			   /* If hard refresh doesn't change the situation
				* we will return an empty BSONObj, which means
				* the exhausted of this cursor
				*/
				if(_closestBarrel[0].size()==0)
				{
				    return mongo::BSONObj();
				}
				else
				{
					return Next();
				}
			}

			// find the entries with minimum distance
			double pMinDistance = _maxDistance + 1;
			int pMinDistanceID = -1;
			for (unsigned int i = 0; i < _closestBarrel[0].size(); i++)
			{
				if (_closestBarrel[0][i]->CurrentDistanceID != -2 && _closestBarrel[0][i]->DistanceSortList[_closestBarrel[0][i]->CurrentDistanceID].distance < pMinDistance)
				{
					pMinDistance = _closestBarrel[0][i]->DistanceSortList[_closestBarrel[0][i]->CurrentDistanceID].distance;
					pMinDistanceID = i;
				}
			}
			mongo::OID returnKey = _closestBarrel[0][pMinDistanceID]->DistanceSortList[_closestBarrel[0][pMinDistanceID]->CurrentDistanceID].key;
			// change the CurrentDistanceID tagged in geoNearSearchNode 
			if (_closestBarrel[0][pMinDistanceID]->CurrentDistanceID <  static_cast<int> (_closestBarrel[0][pMinDistanceID]->DistanceSortList.size() - 1))
			{
				_closestBarrel[0][pMinDistanceID]->CurrentDistanceID++;
			}
			else
			{
				// this node in Level0 has exhausted, we have to remove it from the vectors
				GeoNearSearchNode *node2Delete = _closestBarrel[0][pMinDistanceID];
				RemoveNodeInCmpVectorAndclosestBarrel(node2Delete,0);
				if (pMinDistanceID < _CurrentDistanceIDcmpVectors[0])
				{
					_CurrentDistanceIDcmpVectors[0]--;
				}
				DeleteNode(node2Delete);
			}

			// check if there are exhausted nodes in upper level (Level>0)
			// if found, delete it. 
			int pLevel = 1;
			while (pLevel < _max_level)
			{
				for ( int i = 0; i <static_cast<int> (_closestBarrel[pLevel].size()); i++)
				{
					if (_closestBarrel[pLevel][i]->NumberOfNode == 0)
					{
						GeoNearSearchNode *node2Delete = _closestBarrel[pLevel][i];
						RemoveNodeInCmpVectorAndclosestBarrel(node2Delete,pLevel);
						if (i < _CurrentDistanceIDcmpVectors[pLevel])
						{
							_CurrentDistanceIDcmpVectors[pLevel]--;
						}
						DeleteNode(node2Delete);
						i--;
					}
				}
				pLevel++;
			}

            /* we need a hard refresh here*/

			// update _CurrentMinDistance and refresh the _cmpVectors and _closestBarrel of each level
			// starts form top to bottom of  r-tree
			_CurrentMinDistance = pMinDistance;
			pLevel = _max_level;
			while (pLevel >= 0)
			{
				RefreshNNCInLevel(pLevel);
				pLevel--;
			}

			// if the upper level changed (e.g new nodes added), the level below needs to be reformed
			for (unsigned int i = 0; i < _Need2ReformCmpVectors.size(); i++)
			{
				_Need2ReformCmpVectors[i] = false;
			}

			return _DataIO->Basic_Fetch_AtomData(_db_name, _collection_name, returnKey);
		}
	}


	 


	/**
	 * called by Next()
	 * refresh the _cmpVectors and _closestBarrel
	 * must be called from the top Level to bottom level
	 * please we use iteration rather than recursion to refresh _closestBarrel and _cmpVectors because it is faster.
	 */
	void rtree_index::RTreeGeoNearCurosr::RefreshNNCInLevel(int Level)
	{

			
			if (!_Need2ReformCmpVectors[Level])// the upper Level didn't change, no new node needed to be added into this level
			{
				if (_cmpVectors[Level].size() > static_cast<unsigned int> (_CurrentDistanceIDcmpVectors[Level]+1))
				{
					/**
					 * test if we have to reform NNC
					 * make it easier to understand, In P-L-O-T (1), we check if _CurrentMinDistance > Min(B)
					 * it is rare that the NNC (with only one MBR now) in lower level have no overlap with other MBR
					 * in distance space, so we have to re-suppily the NNC even when   
					 * (_CurrentMinDistance <= _cmpVectors[Level][_CurrentDistanceIDcmpVectors[Level] + 1]->minDistance)==false
					 */
					bool _needResuppily = false;
					if (_closestBarrel[0].size()==0)
					{
						_needResuppily=true;
					}
					if (_CurrentMinDistance <= _cmpVectors[Level][_CurrentDistanceIDcmpVectors[Level] + 1]->minDistance
					   && !_needResuppily)
					{
						// we don't have to reform NNC
						if (Level > 0)
						{
							_Need2ReformCmpVectors[Level - 1] = false;
						}
					}
					else// we have to reform NNC
					{
						vector<GeoNearSearchNode *>returnNNC;//to record the reformed NNC;
						bool isFound = false;
						int i;
						for (i = _CurrentDistanceIDcmpVectors[Level] + 1; i < static_cast<int>  (_cmpVectors[Level].size()); i++)
						{
							// we find the 1st Node is not in NNC, so we rule out this one add add all node before it into NNC
							// fix 201604271644  : insufficient pruning methord
							if (_cmpVectors[Level][i]->minDistance > _CurrentMinDistance)
							{
								isFound = true;
								_CurrentDistanceIDcmpVectors[Level] = i - 1;
								break;
							}
						}
						if (!isFound)//their are no more nodes fits the NNC
						{
							_CurrentDistanceIDcmpVectors[Level] = _cmpVectors[Level].size() - 1;
						}
						// calualte new maxthreshold of the mindistance of the node in NNC
						double MaxThreshold = -1;
				        for (int pp = 0; pp <= _CurrentDistanceIDcmpVectors[Level]; pp++)
				        {
			        		if (_cmpVectors[Level][pp]->maxDistance>=MaxThreshold)
				        	MaxThreshold = _cmpVectors[Level][pp]->maxDistance;
			        	}
						/**
						 * we just have to add the now node into the NNC rather than create NNC from the beginning
					     * we just have to add the nodes (pos>_closestBarrel[Level].size()) in _cmpVectors into _closestBarrel
						 */
						for (unsigned int j = _closestBarrel[Level].size(); j < _cmpVectors[Level].size(); j++)
						{
							if (_cmpVectors[Level][j]->minDistance < MaxThreshold)
							{
								if (Level > 0)//we expend the new node in NNC
								{
									ExpendSingleNode(_cmpVectors[Level][j]);
									Trim(_cmpVectors[Level][j]);
								}
								//add new node to NNC
								_closestBarrel[Level].push_back(_cmpVectors[Level][j]);
								//we push the child nodes of newly added node to the next level of _cmpVectors
								if (Level > 0)
								{
									for (unsigned int p = 0; p < _cmpVectors[Level][j]->ChildPointers.size(); p++)
									{
										if (_cmpVectors[Level][j]->ChildPointers[p] != NULL)
										{
											_cmpVectors[Level - 1].push_back(_cmpVectors[Level][j]->ChildPointers[p]);
										}
									}
								}
							}
							else//do nothing
							{
								break;
							}
						}
						// this level is reformed, so we have to reform the next level
						if (Level > 0)
						{
							_Need2ReformCmpVectors[Level - 1] = true;
						}
					}
				}
			}
			else// we have to reform _cmpVectors
			{
				// sort the _cmpVectors, please note the new nodes have been added in the last run.
				sort(_cmpVectors[Level].begin(), _cmpVectors[Level].end(), RTreeGeoNearSearchNodeCompare);
				int i;
				bool isFound = false;
				for (i = _CurrentDistanceIDcmpVectors[Level]; i < static_cast<int> (_cmpVectors[Level].size()); i++)
				{
					if (_cmpVectors[Level][i]->minDistance>_CurrentMinDistance)
					{
						isFound = true;
						_CurrentDistanceIDcmpVectors[Level] = i - 1;
						break;
					}
				}
				if (!isFound == false)
				{
				}
				
				double MaxThreshold = -1;
				for (int pp = 0; pp <= _CurrentDistanceIDcmpVectors[Level]; pp++)
				{
					if (_cmpVectors[Level][pp]->maxDistance>=MaxThreshold)
					MaxThreshold = _cmpVectors[Level][pp]->maxDistance;
				}
				
				
				vector<GeoNearSearchNode *> returnNNC;
				for (unsigned int i = 0; i < _cmpVectors[Level].size(); i++)
				{
					if (_cmpVectors[Level][i]->minDistance < MaxThreshold)
					{
						returnNNC.push_back(_cmpVectors[Level][i]);
					}
				}
				// if (Level > 0)
				// {
					_closestBarrel[Level] = returnNNC;
				// }
				// else
				// {
				// 	int SecondStartPos = 0;
				// 	for (int oldID = 0; oldID < _closestBarrel[0].size(); oldID++)
				// 	{
				// 		for (int newID = SecondStartPos; newID < returnNNC.size(); newID++)
				// 		{
				// 			if (_closestBarrel[0][oldID]->data.NodeKey == returnNNC[newID]->data.NodeKey)
				// 			{
				// 				SecondStartPos = newID;
				// 				returnNNC[newID]->DistanceSortList = _closestBarrel[0][oldID]->DistanceSortList;
				// 				returnNNC[newID]->CurrentDistanceID = _closestBarrel[0][oldID]->CurrentDistanceID;
				// 			}
				// 		}
				// 	}
				// 	_closestBarrel[Level] = returnNNC;
				// }
				bool needExpendNextLevel = false;
				if (Level > 0)
				{
					for (unsigned int i = 0; i < _closestBarrel[Level].size(); i++)
					{
						if (_closestBarrel[Level][i]->ChildPointers.size() == 0)
						{
							
							ExpendSingleNode(_cmpVectors[Level][i]);
							Trim(_cmpVectors[Level][i]);
							for (unsigned int p = 0; p < _cmpVectors[Level][i]->ChildPointers.size(); p++)
							{
								if (_cmpVectors[Level][i]->ChildPointers[p] != NULL)
								{
									needExpendNextLevel = true;
									_cmpVectors[Level - 1].push_back(_cmpVectors[Level][i]->ChildPointers[p]);
								}
							}
						}
					}
				}
				if (needExpendNextLevel)
				{
					if (Level > 0)
					{
						_Need2ReformCmpVectors[Level - 1] = true;
					}
				}
			}
		
	
	}

	
	
	
	bool rtree_index::RTreeGeoNearCurosr::FillTheDataChilds(GeoNearSearchNode *Node2Fill)
	{
		vector<KeywithDis> tobeSort;
		for (unsigned int i = 0; i < Node2Fill->data.Branchs.size(); i++)
		{
			if (Node2Fill->data.Branchs[i].HasData == true)
			{
				double mind = _MinDistanceFromMBR(_ctx, _cty, Node2Fill->data.Branchs[i].mbr);
				double maxd = _MaxDistanceFromMBR(_ctx, _cty, Node2Fill->data.Branchs[i].mbr);
				if (mind > _maxDistance || maxd < _minDistance)
				{
					//do nothing
				}
				else
				{
					double calculateAccurateDistance;
					_DataIO->Geo_Verify_Intersect(Node2Fill->data.Branchs[i].ChildKey, _ctx, _cty, _minDistance, _maxDistance, false, _db_name, _collection_name, _column_name, calculateAccurateDistance);
					if (calculateAccurateDistance <= _maxDistance &&calculateAccurateDistance >= _minDistance)
					{
						KeywithDis node2sort;
						node2sort.key = Node2Fill->data.Branchs[i].ChildKey;
						node2sort.distance = calculateAccurateDistance;
						tobeSort.push_back(node2sort);
					}
				}
			}
		}
		sort(tobeSort.begin(), tobeSort.end(), nearcompare);
		Node2Fill->DistanceSortList = tobeSort;
		if (tobeSort.size() == 0)
		{
			Node2Fill->CurrentDistanceID = -2;
		}
		else
		{
			Node2Fill->CurrentDistanceID = 0;
		}
		return true;
	}

	
	void rtree_index::RTreeGeoNearCurosr::FreeCursor()
	{
		if (_closestBarrel[_closestBarrel.size() - 1].size() == 0)
		{
			return;
		}
		FreeNode(_root_node);
	}

	
	void rtree_index::RTreeGeoNearCurosr::FreeNode(GeoNearSearchNode * node2Free)
	{
		if (node2Free == NULL)
		{
			return;
		}
		else
		{
			if (node2Free->ChildPointers.size() == 0)
			{
				delete(node2Free);
			}
			else
			{
				for (unsigned int i = 0; i < node2Free->ChildPointers.size(); i++)
				{
					FreeNode(node2Free->ChildPointers[i]);
				}
				delete(node2Free);
			}
		}
	}

	
	void rtree_index::RTreeGeoNearCurosr::PrintStatus()
	{
		cout << "CurrentDistance:  " << _CurrentMinDistance<<"  ";
		cout << "activity_type:  " << _closestBarrel[1][0]->active_type;
		cout << "_cmpVectors:  ";
		for (unsigned int i = 0; i < _cmpVectors.size(); i++)
		{
			cout << _cmpVectors[i].size() << "\t";
		}
		cout << "_closestBarrel:  ";
		for (unsigned int i = 0; i < _closestBarrel.size(); i++)
		{
			cout << _closestBarrel[i].size() << "\t";
		}
	}

	void rtree_index::RTreeGeoNearCurosr::RemoveNodeInCmpVectorAndclosestBarrel(GeoNearSearchNode * Node2Remove,  int DeleteLevel)
	{
		vector<GeoNearSearchNode *>::iterator it;
		for (it = _cmpVectors[DeleteLevel].begin(); it != _cmpVectors[DeleteLevel].end();)
		{
			if (*it == Node2Remove)
			{
				it = _cmpVectors[DeleteLevel].erase(it);  
				break;
			}
			else
			{
				++it;    //next
			}
		}
		for (it = _closestBarrel[DeleteLevel].begin(); it != _closestBarrel[DeleteLevel].end();)
		{
			if (*it == Node2Remove)
			{
				it = _closestBarrel[DeleteLevel].erase(it);       
				break;
			}
			else
			{
				++it;    //next
			}
		}
	}
    
}
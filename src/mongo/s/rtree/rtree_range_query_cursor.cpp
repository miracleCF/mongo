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

#include "rtree_range_query_cursor.h"

using namespace rtree_index;
using namespace mongo;
using namespace index_manager;

#define _QUERY_WITH_PRECISION

namespace rtree_index
{
    rtree_index::RTreeRangeQueryCursor::RTreeRangeQueryCursor()
	{
	
	}


	rtree_index::RTreeRangeQueryCursor::RTreeRangeQueryCursor(int Max_Node, Node RootNode, MongoIO *IO, MongoIndexManagerIO *DataIO, geos::geom::Geometry * SearchGeometry, string DB_NAME, string COLLECTION_NAME, string COLUMN_NAME, int QueryType) 
	{
		_max_node = Max_Node;
		_Root_Node = RootNode;
		_IO = IO;
		_DataIO = DataIO;
		_SearchGeometry = SearchGeometry;
		_DB_NAME = DB_NAME;
		_COLLECTION_NAME = COLLECTION_NAME;
		_COLUMN_NAME = COLUMN_NAME;
		_Max_Level = RootNode.Level;
		_queryType = QueryType;
	}

	void rtree_index::RTreeRangeQueryCursor::InitCursor()
	{
		
		
		
		_NodeStack.push(_Root_Node);
		_NodeIDStack.push(-1);// please note we init the value -1 for the root level,so ++ will be 0
		while (!_NodeStack.empty())
		{
			Node CurrentNode = _NodeStack.top();
			int CurrentID = _NodeIDStack.top();//fatch the current pos of this level
			if (CurrentID < _max_node - 1)//== will exceed the array boundary
			{
				_NodeIDStack.top()++;
				if (CurrentNode.Branchs[_NodeIDStack.top()].HasData)
				{
	
					MBR m = CurrentNode.Branchs[_NodeIDStack.top()].mbr;
					CoordinateSequence* cs = csf.create(5, 2);
					cs->setAt(Coordinate(m.MinX, m.MinY, 0), 0);
					cs->setAt(Coordinate(m.MaxX, m.MinY, 0), 1);
					cs->setAt(Coordinate(m.MaxX, m.MaxY, 0), 2);
					cs->setAt(Coordinate(m.MinX, m.MaxY, 0), 3);
					cs->setAt(Coordinate(m.MinX, m.MinY, 0), 4);
					geos::geom::LinearRing *pMBR = factory.createLinearRing(cs);
					geos::geom::Polygon *pPolygon = factory.createPolygon(pMBR, NULL);
					if (CurrentNode.Branchs[_NodeIDStack.top()].HasData)
					{
						bool satisfyCond = false;
						if (_queryType == 0)
						{
							if(CurrentNode.Level == 0)
							{
								if (_SearchGeometry->contains(pPolygon))
							    {
								   satisfyCond = true;
								}
							}
							else// >0
							{
								if (_SearchGeometry->intersects(pPolygon))
							    {
								   satisfyCond = true;
								}
							}
						}
						if (_queryType == 1)
						{
							if (_SearchGeometry->intersects(pPolygon))
							{
								satisfyCond = true;
							}
						}
						if (satisfyCond)
						{
							if (CurrentNode.Level == 0)
							{
								_Current_Target = CurrentNode.Branchs[_NodeIDStack.top()].ChildKey;
							
								delete pPolygon;
								break;
							}
							else//Level>0
							{
								Node tempNode = _IO->Basic_Find_One_Node(CurrentNode.Branchs[_NodeIDStack.top()].ChildKey);
							
								_NodeStack.push(tempNode);
								_NodeIDStack.push(-1);
							}
						}
					}
					delete pPolygon;//LinearRing will be delete by ~Polygon() automatically
				}
			}
			else
			{
				//pop this level and back to upper level.
				_NodeStack.pop();
				_NodeIDStack.pop();
			}
		}
		_isFirstTime=true;
	}


		mongo::BSONObj rtree_index::RTreeRangeQueryCursor::Next()
	{
		bool hasMore = false;
		while (!_NodeStack.empty())
		{
		    //please note that initCursor may have found a correct result.
            if(_isFirstTime)
			{
				_isFirstTime=false;
				return _DataIO->Basic_Fetch_AtomData(_DB_NAME, _COLLECTION_NAME, _Current_Target);
			}
			//
			
			Node CurrentNode = _NodeStack.top();
			int CurrentID = _NodeIDStack.top();
			if (CurrentID < _max_node - 1)
			{
		
				_NodeIDStack.top()++;
				if (CurrentNode.Branchs[_NodeIDStack.top()].HasData)
				{
				
					MBR m = CurrentNode.Branchs[_NodeIDStack.top()].mbr;
					CoordinateSequence* cs = csf.create(5, 2);
					cs->setAt(Coordinate(m.MinX, m.MinY, 0), 0);
					cs->setAt(Coordinate(m.MaxX, m.MinY, 0), 1);
					cs->setAt(Coordinate(m.MaxX, m.MaxY, 0), 2);
					cs->setAt(Coordinate(m.MinX, m.MaxY, 0), 3);
					cs->setAt(Coordinate(m.MinX, m.MinY, 0), 4);
					geos::geom::LinearRing *pMBR = factory.createLinearRing(cs);
					geos::geom::Polygon *pPolygon = factory.createPolygon(pMBR, NULL);
					if (CurrentNode.Branchs[_NodeIDStack.top()].HasData)
					{
						bool satisfyCond = false;
						if (_queryType == 0)
						{
							if(CurrentNode.Level == 0)
							{
								if (_SearchGeometry->contains(pPolygon))
							    {
								   satisfyCond = true;
								}
							}
							else// >0
							{
								if (_SearchGeometry->intersects(pPolygon))
							    {
								   satisfyCond = true;
								}
							}
						}
						if (_queryType == 1)
						{
							if (_SearchGeometry->intersects(pPolygon))
							{
								satisfyCond = true;
							}
						}
						if (satisfyCond)
						{
							if (CurrentNode.Level == 0)
							{
#ifdef _QUERY_WITH_PRECISION
                                mongo::OID id2Test = CurrentNode.Branchs[_NodeIDStack.top()].ChildKey;
								
								if(_queryType==0)//geoWithin
								{
									if(_DataIO->Geo_Verify_Contain(id2Test,_SearchGeometry,false,_DB_NAME,_COLLECTION_NAME,_COLUMN_NAME))
									{
										 _Current_Target = CurrentNode.Branchs[_NodeIDStack.top()].ChildKey;
								         hasMore = true;
								         delete pPolygon;
								         break;
									}
								}
								if(_queryType==1)
								{
									if(_DataIO->Geo_Verify_Intersect(id2Test,_SearchGeometry,false,_DB_NAME,_COLLECTION_NAME,_COLUMN_NAME))
									{
										 _Current_Target = CurrentNode.Branchs[_NodeIDStack.top()].ChildKey;
								         hasMore = true;
								         delete pPolygon;
								         break;
									}
								}
								
#else
                                _Current_Target = CurrentNode.Branchs[_NodeIDStack.top()].ChildKey;
								hasMore = true;
								delete pPolygon;
								break;
#endif
								
							}
							else//Level>0
							{
								Node tempNode = _IO->Basic_Find_One_Node(CurrentNode.Branchs[_NodeIDStack.top()].ChildKey);
							
								_NodeStack.push(tempNode);
								_NodeIDStack.push(-1);
							}
						}
					}
					delete pPolygon;//LinearRing will be delete by ~Polygon() automatically
				}

			}
			else
			{
		
				_NodeStack.pop();
				_NodeIDStack.pop();
			}
		}
		if (hasMore)
		{
		
			return _DataIO->Basic_Fetch_AtomData(_DB_NAME, _COLLECTION_NAME, _Current_Target);
		}
		else
		{
			return mongo::BSONObj();
		}
	}

	
	void rtree_index::RTreeRangeQueryCursor::FreeCursor()
	{
		if(_SearchGeometry!=NULL)
		{
		    delete _SearchGeometry;
		}
	}
}
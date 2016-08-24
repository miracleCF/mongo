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

#include "rtree_io.h"
#include "index_manager_io.h"

#include "rtree_geonear_search_node.h"


//
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


using namespace rtree_index;
using namespace mongo;
using namespace index_manager;


namespace rtree_index
{
	class RTreeRangeQueryCursor
	{
	public:
		RTreeRangeQueryCursor();
		RTreeRangeQueryCursor(int Max_Node, Node RootNode, MongoIO *IO, MongoIndexManagerIO *DataIO, geos::geom::Geometry * SearchGeometry, string DB_NAME, string COLLECTION_NAME, string COLUMN_NAME,int QueryType);
		void InitCursor();
		mongo::BSONObj Next();
		void FreeCursor();
	private:
	   /*
		* -1 undefined
		*  0 GeoWithIn
		*  1 GeoIntersects
		*/
		int _queryType;
		int _max_node;
		Node  _Root_Node=Node(0);
		MongoIO *_IO;
		MongoIndexManagerIO *_DataIO;
		geos::geom::Geometry * _SearchGeometry=NULL;
		string _DB_NAME;
		string _COLLECTION_NAME;
		string _COLUMN_NAME;
		int _Max_Level;

		std::stack<int> _NodeIDStack;
		std::stack<Node> _NodeStack;
		mongo::OID _Current_Target;
		
        bool _isFirstTime=true;

		geos::geom::GeometryFactory factory;
		geos::geom::CoordinateArraySequenceFactory csf;
	};



}

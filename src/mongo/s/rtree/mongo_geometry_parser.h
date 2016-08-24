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
#include <vector>

//for mongo includes
#include "write_op.h"

using namespace geos;
using namespace geom;
using namespace mongo;

/*   basic element of any geometry obj is Coordinate   */
typedef Coordinate PT;

#define GEOM_INVALID_NO_ENOUGH_POINT_IN_RING -1
#define GEOM_INVALID_SHAPE -2
#define GEOM_INVALID_GEOJSON_FORMAT -3;
#define GEOM_INVALID_NO_ENOUGH_POINT_IN_LINESTRING -4
#define GEOM_PARSE_SUCCESS 1

/**
 *Get the geometry from the BSONObj
 *BSONObj has a field:coordinates,it is a array of point
 *And another filed:type,it tells the type of geometry
 */
namespace geometry_parser
{
	class MongoGeometryParser 
	{
	public:
		int DataType2Polygon(BSONObj GeometryData, geom::Polygon* &returnPolygon);
		int DataType2LineString(BSONObj GeometryData, LineString* &returnLinerString);
		int DataType2Point(BSONObj GeometryData, Point * &returnPoint);
		int DataType2MutiLineString(BSONObj GeometryData, MultiLineString * &returnMutiLineString);
		int DataType2MutiPoint(BSONObj GeometryData, MultiPoint * &returnMutiPoint);
		int DataType2MutiPolygon(BSONObj GeometryData, MultiPolygon * &returnMutiPolygon);
		int DataType2GeometryCollection(BSONObj GeometryData, GeometryCollection * &returnGeometryCollection);
		int DataType2Geometry(BSONObj GeometryData, geom::Geometry *&returnGeometry);
	public:
	    GeometryFactory factory;
		CoordinateArraySequenceFactory csf;
	};

	

	
}
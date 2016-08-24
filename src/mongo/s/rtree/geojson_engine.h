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

#include <vector>
#include "rtree_data_structure.h"
#include "write_op.h"

using namespace std;
using namespace mongo;
using namespace  rtree_index;

namespace geojson_engine
{
	/**
	 *Some operations in geojson are defined here
	 *get the geometry type of the object(such as point,linestring...)
	 *get the MBR of the object
	 */
	class GeoJSONPaser
	{
	public:
		static int GetGeoBSONType(BSONObj obj);
		static int GetGeoJSONType(string JSONObj);
		static bool VerifyGeoBSONType(BSONObj obj, MBR &mbr);
		static bool VerifyGeoJSONType(string JSONObj, MBR &mbr);
		static bool VeryfyPoint(BSONObj obj, MBR &mbr);
		static bool VeryfyLineString(BSONObj obj, MBR &mbr);
		static bool VeryfyPolygon(BSONObj obj, MBR &mbr);
		static bool VeryfyMutiPoint(BSONObj obj, MBR &mbr);
		static bool VeryfyMutiLineString(BSONObj obj, MBR &mbr);
		static bool VeryfyMutiPolygon(BSONObj obj, MBR &mbr);
		static bool VeryfyGeometryCollection(BSONObj obj, MBR &mbr);
	};
}
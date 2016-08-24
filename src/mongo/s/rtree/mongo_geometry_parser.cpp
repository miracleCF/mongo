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

#include "mongo_geometry_parser.h"

namespace geometry_parser
{
    int geometry_parser::MongoGeometryParser::DataType2Polygon(BSONObj GeometryData, geom::Polygon* &returnPolygon)
	{
		LinearRing* shellRing = NULL;
		vector<Geometry *> *holes = new vector<Geometry *>();
		if (!GeometryData.hasField("coordinates"))
		{
			delete shellRing;
			delete holes;
			return GEOM_INVALID_GEOJSON_FORMAT;
		}
		BSONObj coordinateObj = GeometryData["coordinates"].Obj();
		vector<BSONElement>LL;
		coordinateObj.elems(LL);
		if (LL.size() < 1)
		{
			delete shellRing;
			delete holes;
			return GEOM_INVALID_GEOJSON_FORMAT;
		}
		for (unsigned int j = 0; j < LL.size(); j++)
		{
			BSONObj oneRing = LL[j].Obj();
			vector<BSONElement>L;
			oneRing.elems(L);
			if (L.size() < 4)
			{
				delete shellRing;
				delete holes;
				return GEOM_INVALID_NO_ENOUGH_POINT_IN_RING;
			}

			CoordinateSequence* cs = csf.create(L.size(), 2);

			for (unsigned int i = 0; i < L.size(); i++)
			{
				BSONObj arrobj = L[i].Obj();
				vector<BSONElement>coord;
				arrobj.elems(coord);
				if (coord.size() == 2 && coord[0].isNumber() && coord[1].isNumber())
				{
					double x = coord[0].numberDouble();
					double y = coord[1].numberDouble();
					cs->setAt(PT(x, y, 0), i);

				}
				else
				{
					delete shellRing;
					delete holes;
					return GEOM_INVALID_GEOJSON_FORMAT;
				}
			}
			if (j == 0)
			{
				shellRing = factory.createLinearRing(cs);
			}
			/*if exist holes*/
			else
			{
				Geometry * oneHole = factory.createLinearRing(cs);
				holes->push_back(oneHole);
			}
		}
		/*
		Pay attention to memory leak here!
		shellRing and hole will be deleted by ~Polygon()
		so please make sure that returePolygon is deleted
		after the finishing his job out of this functions
		*/
		returnPolygon = factory.createPolygon(shellRing, holes);
		return GEOM_PARSE_SUCCESS;
	}

	int geometry_parser::MongoGeometryParser::DataType2LineString(BSONObj GeometryData, LineString* &returnLinerString)
	{
		if (!GeometryData.hasField("coordinates"))
		{
			return GEOM_INVALID_GEOJSON_FORMAT;
		}
		BSONObj coordinateObj = GeometryData["coordinates"].Obj();
		vector<BSONElement>L;
		coordinateObj.elems(L);
		if (L.size() < 2)
		{
			return GEOM_INVALID_NO_ENOUGH_POINT_IN_LINESTRING;
		}
		/*new here*/
		CoordinateSequence* cs = csf.create(L.size(), 2);
		for (unsigned int i = 0; i < L.size(); i++)
		{
			BSONObj arrobj = L[i].Obj();
			vector<BSONElement>coord;
			arrobj.elems(coord);
			if (coord.size() == 2 && coord[0].isNumber() && coord[1].isNumber())
			{
				double x = coord[0].numberDouble();
				double y = coord[1].numberDouble();
				cs->setAt(PT(x, y, 0), i);

			}
			else
			{
				delete cs;
				return GEOM_INVALID_GEOJSON_FORMAT;
			}
		}
		/*
		Pay attention to memory leak here!
		cs will be deleted by ~LineString()
		so please make sure that returnLinerString is deleted
		after the finishing his job out of this functions
		*/
		returnLinerString = factory.createLineString(cs);
		return GEOM_PARSE_SUCCESS;
		return false;
	}

	int geometry_parser::MongoGeometryParser::DataType2Point(BSONObj GeometryData, Point * &returnPoint)
	{
		if (!GeometryData.hasField("coordinates"))
		{
			return GEOM_INVALID_GEOJSON_FORMAT;
		}
		BSONObj coordinates = GeometryData["coordinates"].Obj();
		vector<BSONElement>coord;
		coordinates.elems(coord);
		if (coord.size() == 2 && coord[0].isNumber() && coord[1].isNumber())
		{
			double x = coord[0].numberDouble();
			double y = coord[1].numberDouble();
			returnPoint = factory.createPoint(PT(x, y, 0));
			return GEOM_PARSE_SUCCESS;
		}
		return GEOM_INVALID_GEOJSON_FORMAT;
	}

	int  GeometryParser::MongoGeometryParser::DataType2MutiLineString(BSONObj GeometryData, MultiLineString * &returnMutiLineString)
	{
		if (!GeometryData.hasField("coordinates"))
		{
			return GEOM_INVALID_GEOJSON_FORMAT;
		}
		BSONObj coordinateObj = GeometryData["coordinates"].Obj();
		vector<BSONElement>LL;
		coordinateObj.elems(LL);
		if (LL.size() < 1)
		{
			return GEOM_INVALID_GEOJSON_FORMAT;
		}
		/*new here*/
		vector<Geometry *> *LineStrings = new vector<Geometry *>();
		for (unsigned int j = 0; j < LL.size(); j++)
		{
			BSONObj oneRing = LL[j].Obj();
			vector<BSONElement>L;
			oneRing.elems(L);
			if (L.size() < 2)
			{
				delete LineStrings;
				return GEOM_INVALID_NO_ENOUGH_POINT_IN_LINESTRING;
			}
			/*new here*/
			CoordinateSequence* cs = csf.create(L.size(), 2);
			for (unsigned int i = 0; i < L.size(); i++)
			{
				BSONObj arrobj = L[i].Obj();
				vector<BSONElement>coord;
				arrobj.elems(coord);
				if (coord.size() == 2 && coord[0].isNumber() && coord[1].isNumber())
				{
					double x = coord[0].numberDouble();
					double y = coord[1].numberDouble();
					cs->setAt(PT(x, y, 0), i);

				}
				else
				{
					delete cs;
					delete LineStrings;
					return GEOM_INVALID_GEOJSON_FORMAT;
				}
			}
			geom::Geometry *pOneLine = factory.createLineString(cs);
			LineStrings->push_back(pOneLine);
		}
		/*
		Pay attention to memory leak here!
		LineString will be deleted by ~MutiLineString()
		so please make sure that returePolygon is deleted
		after the finishing his job out of this functions
		*/
		returnMutiLineString = factory.createMultiLineString(LineStrings);
		return GEOM_PARSE_SUCCESS;

	}

	int geometry_parser::MongoGeometryParser::DataType2MutiPoint(BSONObj GeometryData, MultiPoint * &returnMutiPoint)
	{
		if (!GeometryData.hasField("coordinates"))
		{
			return GEOM_INVALID_GEOJSON_FORMAT;
		}
		BSONObj coordinateObj = GeometryData["coordinates"].Obj();
		vector<BSONElement>L;
		coordinateObj.elems(L);
		if (L.size() < 1)
		{
			return GEOM_INVALID_GEOJSON_FORMAT;
		}
		/*new here*/
		vector<Geometry *> *Points = new vector<Geometry *>();
		for (unsigned int i = 0; i < L.size(); i++)
		{
			BSONObj arrobj = L[i].Obj();
			vector<BSONElement>coord;
			arrobj.elems(coord);
			if (coord.size() == 2 && coord[0].isNumber() && coord[1].isNumber())
			{
				double x = coord[0].numberDouble();
				double y = coord[1].numberDouble();
				geom::Geometry *pPoint = factory.createPoint(PT(x, y, 0));
				Points->push_back(pPoint);
			}
			else
			{
				delete Points;
				return GEOM_INVALID_GEOJSON_FORMAT;
			}
		}
		/*
		Pay attention to memory leak here!
		cs will be deleted by ~MutiPoint()
		so please make sure that returnLinerString is deleted
		after the finishing his job out of this functions
		*/
		returnMutiPoint = factory.createMultiPoint(Points);
		return GEOM_PARSE_SUCCESS;
		return false;
	}

	int geometry_parser::MongoGeometryParser::DataType2MutiPolygon(BSONObj GeometryData, MultiPolygon * &returnMutiPolygon)
	{
		if (!GeometryData.hasField("coordinates"))
		{
			return GEOM_INVALID_GEOJSON_FORMAT;
		}
		BSONObj TotalData = GeometryData["coordinates"].Obj();
		vector<BSONElement>LLL;
		TotalData.elems(LLL);
		if (LLL.size() < 1)
		{
			return GEOM_INVALID_GEOJSON_FORMAT;
		}
		/*new here*/
		vector<Geometry *> *Polygons = new vector<Geometry *>();
		for (unsigned int k = 0; k < LLL.size(); k++)
		{
			BSONObj onePolygon = LLL[k].Obj();
			vector<BSONElement>LL;
			onePolygon.elems(LL);
			if (LL.size() < 1)
			{
				delete Polygons;
				return GEOM_INVALID_GEOJSON_FORMAT;
			}
			/*new here*/
			LinearRing* shellRing = NULL;
			/*new here*/
			vector<Geometry *> *holes = new vector<Geometry *>();
			for (unsigned int j = 0; j < LL.size(); j++)
			{
				BSONObj oneRing = LL[j].Obj();
				vector<BSONElement>L;
				oneRing.elems(L);
				if (L.size() < 4)
				{
					delete Polygons;
					delete shellRing;
					delete holes;
					return GEOM_INVALID_NO_ENOUGH_POINT_IN_RING;
				}

				CoordinateSequence* cs = csf.create(L.size(), 2);
				for (unsigned int i = 0; i < L.size(); i++)
				{
					BSONObj arrobj = L[i].Obj();
					vector<BSONElement>coord;
					arrobj.elems(coord);
					if (coord.size() == 2 && coord[0].isNumber() && coord[1].isNumber())
					{
						double x = coord[0].numberDouble();
						double y = coord[1].numberDouble();
						cs->setAt(PT(x, y, 0), i);

					}
					else
					{
						delete Polygons;
						delete shellRing;
						delete holes;
						return GEOM_INVALID_GEOJSON_FORMAT;
					}
				}
				if (j == 0)
				{
					shellRing = factory.createLinearRing(cs);
				}
				/*if exist holes*/
				else
				{
					Geometry * oneHole = factory.createLinearRing(cs);
					holes->push_back(oneHole);
				}
			}
			geom::Geometry * singlePolygon = factory.createPolygon(shellRing, holes);
			Polygons->push_back(singlePolygon);

		}
		/*
		Pay attention to memory leak here!
		Polygons will be deleted by ~MutiPolygon()
		so please make sure that returePolygon is deleted
		after the finishing his job out of this functions
		*/
		returnMutiPolygon = factory.createMultiPolygon(Polygons);
		return GEOM_PARSE_SUCCESS;


	}

	int geometry_parser::MongoGeometryParser::DataType2GeometryCollection(BSONObj GeometryData, GeometryCollection * &returnGeometryCollection)
	{
		if (!GeometryData.hasField("geometries"))
		{
			return GEOM_INVALID_GEOJSON_FORMAT;
		}
		vector<BSONElement> L;
		BSONObj TotalData = GeometryData["geometries"].Obj();
		TotalData.elems(L);
		if (L.size() < 1)
		{
			return GEOM_INVALID_GEOJSON_FORMAT;
		}
		/*new here*/
		vector<Geometry *> *Geometrys = new vector<Geometry *>();
		for (unsigned int i = 0; i < L.size(); i++)
		{
			BSONObj oneGeoObj = L[i].Obj();
			string typeString = oneGeoObj["type"].String();
			if (typeString == "Point")
			{
				/*new here*/
				geom::Point *pPoint = NULL;
				if (DataType2Point(oneGeoObj, pPoint) == GEOM_PARSE_SUCCESS)
				{
					geom::Geometry *pGeometry = pPoint;
					Geometrys->push_back(pGeometry);
				}
				else
				{
					delete pPoint;
					delete Geometrys;
					return GEOM_INVALID_GEOJSON_FORMAT;
				}
				continue;
			}
			if (typeString == "LineString")
			{
				/*new here*/
				geom::LineString *pLineString = NULL;
				if (DataType2LineString(oneGeoObj, pLineString) == GEOM_PARSE_SUCCESS)
				{
					geom::Geometry *pGeometry = pLineString;
					Geometrys->push_back(pGeometry);
				}
				else
				{
					delete pLineString;
					delete Geometrys;
					return GEOM_INVALID_GEOJSON_FORMAT;
				}
				continue;
			}
			if (typeString == "Polygon")
			{
				/*new here*/
				geom::Polygon *pPolygon = NULL;
				if (DataType2Polygon(oneGeoObj, pPolygon) == GEOM_PARSE_SUCCESS)
				{
					geom::Geometry *pGeometry = pPolygon;
					Geometrys->push_back(pGeometry);
				}
				else
				{
					delete pPolygon;
					delete Geometrys;
					return GEOM_INVALID_GEOJSON_FORMAT;
				}
				continue;
			}
			if (typeString == "MultiPoint")
			{
				/*new here*/
				geom::MultiPoint *pMutiPoint = NULL;
				if (DataType2MutiPoint(oneGeoObj, pMutiPoint) == GEOM_PARSE_SUCCESS)
				{
					geom::Geometry *pGeometry = pMutiPoint;
					Geometrys->push_back(pGeometry);
				}
				else
				{
					delete pMutiPoint;
					delete Geometrys;
					return GEOM_INVALID_GEOJSON_FORMAT;
				}
				continue;
			}
			if (typeString == "MultiLineString")
			{
				/*new here*/
				geom::MultiLineString *pMultiLineString = NULL;
				if (DataType2MutiLineString(oneGeoObj, pMultiLineString) == GEOM_PARSE_SUCCESS)
				{
					geom::Geometry *pGeometry = pMultiLineString;
					Geometrys->push_back(pGeometry);
				}
				else
				{
					delete pMultiLineString;
					delete Geometrys;
					return GEOM_INVALID_GEOJSON_FORMAT;
				}
				continue;
			}
			if (typeString == "MultiPolygon")
			{
				/*new here*/
				geom::MultiPolygon *pMultiPolygon = NULL;
				if (DataType2MutiPolygon(oneGeoObj, pMultiPolygon) == GEOM_PARSE_SUCCESS)
				{
					geom::Geometry *pGeometry = pMultiPolygon;
					Geometrys->push_back(pGeometry);
				}
				else
				{
					delete pMultiPolygon;
					delete Geometrys;
					return GEOM_INVALID_GEOJSON_FORMAT;
				}
				continue;
			}
		}
		returnGeometryCollection = factory.createGeometryCollection(Geometrys);
		return GEOM_PARSE_SUCCESS;




	}

	int geometry_parser::MongoGeometryParser::DataType2Geometry(BSONObj GeometryData, geom::Geometry *&returnGeometry)
	{
		if (!GeometryData.hasField("type"))
		{
			return GEOM_INVALID_GEOJSON_FORMAT;
		}
		string typeString = GeometryData["type"].String();
		if (typeString == "Point")
		{
			geom::Point *pgeo = NULL;
			int returnValue = DataType2Point(GeometryData, pgeo);
			returnGeometry = pgeo;
			return returnValue;

		}
		if (typeString == "LineString")
		{
			geom::LineString *pgeo = NULL;
			int returnValue = DataType2LineString(GeometryData, pgeo);
			returnGeometry = pgeo;
			return returnValue;
		}
		if (typeString == "Polygon")
		{
			geom::Polygon *pgeo = NULL;
			int returnValue = DataType2Polygon(GeometryData, pgeo);
			returnGeometry = pgeo;
			return returnValue;
		}
		if (typeString == "GeometryCollection")
		{
			geom::GeometryCollection *pgeo = NULL;
			int returnValue = DataType2GeometryCollection(GeometryData, pgeo);
			returnGeometry = pgeo;
			return returnValue;
		}
		if (typeString == "MultiPoint")
		{
			geom::MultiPoint *pgeo = NULL;
			int returnValue = DataType2MutiPoint(GeometryData, pgeo);
			returnGeometry = pgeo;
			return returnValue;
		}
		if (typeString == "MultiLineString")
		{
			geom::MultiLineString *pgeo = NULL;
			int returnValue = DataType2MutiLineString(GeometryData, pgeo);
			returnGeometry = pgeo;
			return returnValue;
		}
		if (typeString == "MultiPolygon")
		{
			geom::MultiPolygon *pgeo = NULL;
			int returnValue = DataType2MutiPolygon(GeometryData, pgeo);
			returnGeometry = pgeo;
			return returnValue;
		}
		return GEOM_INVALID_GEOJSON_FORMAT;
	}
}
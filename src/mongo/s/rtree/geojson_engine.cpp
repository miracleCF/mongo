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

#include "geojson_engine.h"

using namespace std;
using namespace mongo;


namespace geojson_engine
{
	int GeoJSONPaser::GetGeoBSONType(BSONObj obj)
	{
		int typeID = -1;
		//
		try
		{
			string typeString = obj["type"].String();
			if (typeString == "Point")
			{
				typeID = 1;
				return typeID;
			}
			if (typeString == "LineString")
			{
				typeID = 2;
				return typeID;
			}
			if (typeString == "Polygon")
			{
				typeID = 3;
				return typeID;
			}
			if (typeString == "GeometryCollection")
			{
				typeID = 4;
				return typeID;
			}
			if (typeString == "MultiPoint")
			{
				typeID = 5;
				return typeID;
			}
			if (typeString == "MultiLineString")
			{
				typeID = 6;
				return typeID;
			}
			if (typeString == "MultiPolygon")
			{
				typeID = 7;
				return typeID;
			}
			cout << ""<< typeString << endl;
			return 0;
		}
		catch (exception& e)
		{
			cout << ">>Standard exception: " << e.what() << endl;
			typeID = 0;
			cout << "" << endl;
		}
		return typeID;
	}

	int GeoJSONPaser::GetGeoJSONType(string JSONObj)
	{
		BSONObj obj;
		try
		{
			obj = fromjson(JSONObj);
		}
		catch (exception& e)
		{
			cout << e.what() << endl;
			cout << "invalid JSOM format" << endl;
			return 0;
		}
		int typeID;
		// 
		typeID = GetGeoBSONType(obj);
		return typeID;
	}

	bool GeoJSONPaser::VerifyGeoBSONType(BSONObj obj, MBR &mbr)
	{
		int typeID = GetGeoBSONType(obj);
		if (typeID > 0)
		{
			if (typeID == 1)
			{
				return VeryfyPoint(obj, mbr);
			}
			if (typeID == 2)
			{
				return VeryfyLineString(obj, mbr);
			}
			if (typeID == 3)
			{
				return VeryfyPolygon(obj, mbr);
			}
			if (typeID == 4)
			{
				return VeryfyGeometryCollection(obj, mbr);
			}
			if (typeID == 5)
			{
				return VeryfyMutiPoint(obj, mbr);
			}
			if (typeID == 6)
			{
				return VeryfyMutiLineString(obj, mbr);
			}
			if (typeID == 7)
			{
				return VeryfyMutiPolygon(obj, mbr);
			}
		}
		cout << "unknown geo type" << endl;
		return false;
	}

	bool GeoJSONPaser::VeryfyPoint(BSONObj obj, MBR &mbr)
	{
		BSONObj arrobj;
		try
		{
			arrobj = obj["coordinates"].Obj();
			double x = arrobj[0].numberDouble();
			double y = arrobj[1].numberDouble();
			vector<double>v;
			v.push_back(x);
			v.push_back(y);
			v.push_back(x);
			v.push_back(y);
			 MBR m(v[0], v[1], v[2], v[3]);
			mbr = m;
			return true;
		}
		catch (exception& e)
		{
			cout << ">>Standard exception: " << e.what() << endl;
			cout << "invalid point type"<< endl;
			return false;
		}

	}

	bool GeoJSONPaser::VeryfyLineString(BSONObj obj, MBR &mbr)
	{
		double maxx, maxy, minx, miny;
		maxx = maxy = -999999999;
		minx = miny = 999999999;
		BSONObj TotalData;
		try
		{
			TotalData = obj["coordinates"].Obj();
			vector<BSONElement>L;
			TotalData.elems(L);
			for (unsigned int i = 0; i < L.size(); i++)
			{
				BSONObj arrobj = L[i].Obj();
				double x = arrobj[0].numberDouble();
				double y = arrobj[1].numberDouble();
				if (x > maxx)maxx = x;
				if (y > maxy)maxy = y;
				if (x < minx)minx = x;
				if (y < miny)miny = y;

			}
			//
			vector<double> v;
			v.push_back(minx);
			v.push_back(miny);
			v.push_back(maxx);
			v.push_back(maxy);
			 MBR m(v[0], v[1], v[2], v[3]);
			mbr = m;
			return true;

		}
		catch (exception& e)
		{
			cout << ">>Standard exception: " << e.what() << endl;
			cout <<""<< endl;
			return false;
		}

	}

	bool GeoJSONPaser::VeryfyPolygon(BSONObj obj, MBR &mbr)
	{
		double maxx, maxy, minx, miny;
		maxx = maxy = -999999999;
		minx = miny = 999999999;
		BSONObj TotalData;
		try
		{
			TotalData = obj["coordinates"].Obj();
			vector<BSONElement>LL;
			TotalData.elems(LL);
			for (unsigned int j = 0; j < LL.size(); j++)
			{
				BSONObj oneRing = LL[j].Obj();
				vector<BSONElement>L;
				oneRing.elems(L);
				for (unsigned int i = 0; i < L.size(); i++)
				{
					BSONObj arrobj = L[i].Obj();
					double x = arrobj[0].numberDouble();
					double y = arrobj[1].numberDouble();
					if (x > maxx)maxx = x;
					if (y > maxy)maxy = y;
					if (x < minx)minx = x;
					if (y < miny)miny = y;
				}
			}
			vector<double> v;
			v.push_back(minx);
			v.push_back(miny);
			v.push_back(maxx);
			v.push_back(maxy);
			 MBR m(v[0], v[1], v[2], v[3]);
			mbr = m;
			return true;

		}
		catch (exception& e)
		{
			cout << ">>Standard exception: " << e.what() << endl;
			cout << "" << endl;
			return false;
		}
	}

	bool GeoJSONPaser::VeryfyMutiPoint(BSONObj obj, MBR &mbr)
	{
		return VeryfyLineString(obj, mbr);
	}

	bool GeoJSONPaser::VeryfyMutiLineString(BSONObj obj, MBR &mbr)
	{
		return VeryfyPolygon(obj, mbr);
	}

	bool GeoJSONPaser::VeryfyMutiPolygon(BSONObj obj, MBR &mbr)
	{
		double maxx, maxy, minx, miny;
		maxx = maxy = -999999999;
		minx = miny = 999999999;
		BSONObj TotalData;
		try
		{
			TotalData = obj["coordinates"].Obj();
			vector<BSONElement>LLL;
			TotalData.elems(LLL);
			for (unsigned int k = 0; k < LLL.size(); k++)
			{
				BSONObj onePolygon = LLL[k].Obj();
				vector<BSONElement>LL;
				onePolygon.elems(LL);
				for (unsigned int j = 0; j < LL.size(); j++)
				{
					BSONObj oneRing = LL[j].Obj();
					vector<BSONElement>L;
					oneRing.elems(L);
					for (unsigned int i = 0; i < L.size(); i++)
					{
						BSONObj arrobj = L[i].Obj();
						double x = arrobj[0].numberDouble();
						double y = arrobj[1].numberDouble();
						if (x > maxx)maxx = x;
						if (y > maxy)maxy = y;
						if (x < minx)minx = x;
						if (y < miny)miny = y;
					}
				}
			}
			vector<double> v;
			v.push_back(minx);
			v.push_back(miny);
			v.push_back(maxx);
			v.push_back(maxy);
			 MBR m(v[0], v[1], v[2], v[3]);
			mbr = m;
			return true;

		}
		catch (exception& e)
		{
			cout << ">>Standard exception: " << e.what() << endl;
			cout << ""<< endl;
			return false;
		}
	}

	bool GeoJSONPaser::VeryfyGeometryCollection(BSONObj obj, MBR &mbr)
	{
		double maxx, maxy, minx, miny;
		maxx = maxy = -999999999;
		minx = miny = 999999999;
		try
		{
			vector<BSONElement> L;
			BSONObj TotalData = obj["geometries"].Obj();
			TotalData.elems(L);
			for (unsigned int i = 0; i < L.size(); i++)
			{
				 MBR m;
				BSONObj oneGeo = L[i].Obj();
				int typeID = GetGeoBSONType(oneGeo);
				if (typeID == 1)
				{
					VeryfyPoint(oneGeo, m);
				}
				if (typeID == 2)
				{
					VeryfyLineString(oneGeo, m);
				}
				if (typeID == 3)
				{
					VeryfyPolygon(oneGeo, m);
				}

				if (typeID == 5)
				{
					VeryfyMutiPoint(oneGeo, m);
				}
				if (typeID == 6)
				{
					VeryfyMutiLineString(oneGeo, m);
				}
				if (typeID == 7)
				{
					VeryfyMutiPolygon(oneGeo, m);
				}
				if (m.MinX < minx)minx = m.MinX;
				if (m.MinY < miny)miny = m.MinY;
				if (m.MaxX > maxx)maxx = m.MaxX;
				if (m.MaxY > maxy)maxy = m.MaxY;



			}

			vector<double> v;
			v.push_back(minx);
			v.push_back(miny);
			v.push_back(maxx);
			v.push_back(maxy);
			 MBR mm(v[0], v[1], v[2], v[3]);
			mbr = mm;

			return true;
		}
		catch (exception& e)
		{
			cout << ">>Standard exception: " << e.what() << endl;
			cout << ""<< endl;
			return false;
		}
	}

	bool GeoJSONPaser::VerifyGeoJSONType(string JSONObj, MBR &mbr)
	{
		BSONObj obj = fromjson(JSONObj);
		VerifyGeoBSONType(obj, mbr);
		return true;
	}
}
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

//#include "stdafx.h"
#include "rtree_data_structure.h"

namespace rtree_index{

	rtree_index::MBR::MBR()
	{
		MinX = 0;
		MinY = 0;
		MaxY = 0;
		MaxX = 0;
	}
	rtree_index::MBR::MBR(double minx, double miny, double maxx, double maxy)
	{
		MinX = minx;
		MinY = miny;
		MaxX = maxx;
		MaxY = maxy;
	}
	
	
	rtree_index::Branch::Branch() 
	{
		HasData = false;
		//mbr = MBR(0, 0, 0, 0);
	}

	rtree_index::Branch::Branch(double minx, double miny, double maxx, double maxy, mongo::OID key):mbr (maxx, maxy, minx, miny)
	{
		HasData = true;
		ChildKey = key;
	}

	rtree_index::Node::Node(int Max_Branch_Size)
	{
		for (int i = 0; i < Max_Branch_Size; i++)
		{
			Branch b;
			Branchs.push_back(b);
		}
		Count = 0;
		Level = 0;
	}
}
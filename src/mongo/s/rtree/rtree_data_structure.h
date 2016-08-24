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

#include<memory>
#include <vector>
#include "write_op.h"


using namespace std;
using namespace mongo;

/**
 *The data structure of rtree is defined here
 */
namespace rtree_index
{
	class MBR
	{
	public:
		MBR();
		MBR(double minx, double miny, double maxx, double maxy);
		double MinX;
		double MinY;
		double MaxX;
		double MaxY;
	};

	class Branch
	{
	public:
		bool HasData;
		MBR mbr;
		mongo::OID ChildKey;
		Branch();
		Branch(double minx, double miny, double maxx, double maxy, mongo::OID key);
	};

	class Node
	{
	public:
		mongo::OID NodeKey;
		int Count = 0;
		int Level = 0;
		vector<Branch> Branchs;
		Node(int Max_Branch_Size);
	};




}
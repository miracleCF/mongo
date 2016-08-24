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

#include "rtree_data_structure.h"
#include "write_op.h"

using namespace rtree_index;
using namespace mongo;


namespace rtree_index
{
	
    /*this is the data structure for distance sort of the spatial objects*/
	struct KeywithDis
	{
		double distance;
		mongo::OID key;
	};

    /* compare help function of the KeywithDis class */
	bool nearcompare(KeywithDis o1, KeywithDis o2);



	class GeoNearSearchNode
	{
	public:
		GeoNearSearchNode();
		GeoNearSearchNode(double min_Dis, double max_Dis, double minmax_Dis, Node nodedata);
	public:
		double minDistance;
		double maxDistance;
		double minMaxDistance;

		int NumberOfNode;//number of nodes
		
		/*
		-1:Unexpended
		0:Expended
		1:visited
		*/
		int active_type;
		//for storing node data in memory
		Node data;
		GeoNearSearchNode * parent;
		int ChildIDInParent;
		vector<GeoNearSearchNode *> ChildPointers;


		//this is for the level0 data entry 
		vector<KeywithDis> DistanceSortList;
		int CurrentDistanceID;
	};



	


}
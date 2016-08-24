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

#include "rtree_geonear_search_node.h"

using namespace rtree_index;
using namespace mongo;

namespace rtree_index
{
    	
	bool nearcompare(KeywithDis o1, KeywithDis o2)
	{
		if (o1.distance < o2.distance)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
    
    rtree_index::GeoNearSearchNode::GeoNearSearchNode():data(0)
	{
		ChildPointers.clear();
		maxDistance=0;
		minDistance=0;
		active_type=-1;
	}

	rtree_index::GeoNearSearchNode::GeoNearSearchNode(double min_Dis, double max_Dis, double minmax_Dis, Node nodedata) : data(nodedata)
	{
		active_type = -1;
		minDistance = min_Dis;
		maxDistance = max_Dis;
		minMaxDistance = minmax_Dis;
		ChildPointers.clear();
		DistanceSortList.clear();
		CurrentDistanceID = -1;
	}
    
    
}
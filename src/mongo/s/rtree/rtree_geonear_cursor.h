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
//#include "fstream"

using namespace rtree_index;
using namespace mongo;
using namespace index_manager;



namespace rtree_index
{
	bool RTreeGeoNearSearchNodeCompare(GeoNearSearchNode *node1, GeoNearSearchNode *node2);
	double _cDistance(double x1, double y1, double x2, double y2);
	double _MinDistanceFromMBR(double x, double y, MBR m);
	double _MaxDistanceFromMBR(double x, double y, MBR m);
	


	class RTreeGeoNearCurosr
	{
	public:
		RTreeGeoNearCurosr();
		RTreeGeoNearCurosr(int Max_Node, GeoNearSearchNode *RootNode, MongoIO *IO, MongoIndexManagerIO *DataIO, double ctx, double cty, double minDistance, double maxDistance, string DB_NAME, string COLLECTION_NAME, string COLUMN_NAME);
		vector<GeoNearSearchNode *> GetNNCInLevel(vector<GeoNearSearchNode *> NNCInUpperLevel, int Level);
		bool InitCursor();
		mongo::BSONObj Next();
		void FreeCursor();
		void PrintStatus();

	private:
		bool ExpendSingleNode(GeoNearSearchNode *Node2Expend);
		void CalculateDistances(GeoNearSearchNode * node2Calculate, MBR m);
		bool DeleteNode(GeoNearSearchNode * node2Delete);
		bool Trim(GeoNearSearchNode *Node2Trim);
		void RefreshNNCInLevel(int Level);
		bool FillTheDataChilds(GeoNearSearchNode *Node2Fill);
		void FreeNode(GeoNearSearchNode * node2Free);
		void RemoveNodeInCmpVectorAndclosestBarrel(GeoNearSearchNode * Node2Remove, int DeleteLevel);
		
		
		
		
		int _max_level;
		double _CurrentMinDistance;
		double _LastKey;
		bool _Continue2Expend = true;
		int _max_node;
		double _ctx;
		double _cty;
		double _minDistance;
		double _maxDistance;
		string _db_name;
		string _collection_name;
		string _column_name;

		vector<bool> _Need2ReformCmpVectors;
		vector<int> _CurrentDistanceIDcmpVectors;
		vector<vector<GeoNearSearchNode *>> _cmpVectors;
		vector<vector<GeoNearSearchNode *>> _closestBarrel;

		GeoNearSearchNode * _root_node;
		MongoIO *_IO;
		MongoIndexManagerIO *_DataIO;


	};

	
}
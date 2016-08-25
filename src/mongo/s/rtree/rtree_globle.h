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

#include "mongo/s/rtree/rtree_data_structure.h"
#include "mongo/s/rtree/index_manager_core.h"
#include "mongo/s/rtree/geojson_engine.h"

/**
 *Some global objects are defined here
 */
namespace mongo 
{
	extern DBClientBase* myconn;
	extern index_manager::MongoIndexManagerIO* pIndexManagerIO;
	extern rtree_index::MongoIO* pRTreeIO;
	extern index_manager::IndexManagerBase IM;
}

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

#include "rtree_globle.h"

namespace mongo
{
    DBClientBase* myconn;
    index_manager::MongoIndexManagerIO* pIndexManagerIO = new index_manager::MongoIndexManagerIO(myconn);
	rtree_index::MongoIO* pRTreeIO = new MongoIO(myconn);
	index_manager::IndexManagerBase IM(pIndexManagerIO, pRTreeIO);
}
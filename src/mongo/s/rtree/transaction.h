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

//#include "mongo/s/rtree/rtree_io.h"
#include "write_op.h"


using namespace mongo;

namespace index_manager{
	class Transaction
	{
	public:
		Transaction(std::string name,OperationContext* txn,std::string dbname);		
		 bool InsertDone(unsigned int sequence,std::string target_collection, rtree_index::writeOpt operation_type,std::string info);
	     bool UpdateDone(unsigned int sequence,std::string target_collection, rtree_index::writeOpt operation_type,std::string info);	
		 bool DeleteDone(unsigned int sequence,std::string target_collection, rtree_index::writeOpt operation_type,std::string info);
		OperationContext* getOperationContext();
		std::string getDBName();
	protected:
	    bool Insert(BSONObj obj);
		bool Update(BSONObj obj);
		bool Delete(BSONObj obj);
		
		std::string _transaction_name;
		mongo::OID _id;
		OperationContext* _txn;
		std::string _dbname;
		std::string _transaction_collection_name;
	};
	
	class CreateIndexTransaction:public Transaction
	{
	public:
	   CreateIndexTransaction(OperationContext* txn,std::string dbname);
	   CreateIndexTransaction(std::string name,OperationContext* txn,std::string dbname);
	protected:
	   bool InsertDone(unsigned int sequence,std::string target_collection, rtree_index::writeOpt operation_type,std::string info);
	   bool UpdateDone(unsigned int sequence,std::string target_collection, rtree_index::writeOpt operation_type,std::string info);	
	};
}

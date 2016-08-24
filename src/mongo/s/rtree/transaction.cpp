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

#include "transaction.h"
#include<time.h>
#include<iostream>

namespace index_manager{
	
Transaction::Transaction(std::string name,OperationContext* txn,std::string dbname)
{
	this->_transaction_name = name;
	this->_txn = txn;
	this->_dbname = dbname;
	this->_id = mongo::OID::gen();
	_transaction_collection_name="transaction";
}

 bool Transaction::InsertDone(unsigned int sequence,std::string target_collection, rtree_index::writeOpt operation_type,std::string info) {return false;}

 bool Transaction::UpdateDone(unsigned int sequence,std::string target_collection, rtree_index::writeOpt operation_type,std::string info) {return false;}	

 bool Transaction::DeleteDone(unsigned int sequence,std::string target_collection, rtree_index::writeOpt operation_type,std::string info) {return false;}

OperationContext* Transaction::getOperationContext() {return this->_txn;}

std::string Transaction::getDBName() 
{ 
	return this->_dbname; 
}

bool Transaction::Insert(BSONObj obj)
{
	BSONObjBuilder bdr;
	BSONObjBuilder & bdrRef = bdr;
	return rtree_index::RunWriteCommand(_txn,_dbname, _transaction_collection_name, obj, rtree_index::INSERT,bdrRef);
}

bool Transaction::Update(BSONObj obj)
{
	BSONObjBuilder bdr;
	BSONObjBuilder & bdrRef = bdr;
	return rtree_index::RunWriteCommand(_txn,_dbname, _transaction_collection_name, obj, rtree_index::UPDATE, bdrRef);
}

bool Transaction::Delete(BSONObj obj )
{
	BSONObjBuilder bdr;
	BSONObjBuilder & bdrRef = bdr;
	return rtree_index::RunWriteCommand(_txn,_dbname, _transaction_collection_name, obj, rtree_index::REMOVE, bdrRef);
}


CreateIndexTransaction::CreateIndexTransaction(OperationContext* txn,std::string dbname):Transaction("Transaction_CreateIndex",txn,dbname){}

CreateIndexTransaction::CreateIndexTransaction(std::string name,OperationContext* txn,std::string dbname):Transaction(name,txn,dbname){}

bool CreateIndexTransaction::InsertDone(unsigned int sequence,std::string target_collection, rtree_index::writeOpt operation_type,std::string info)
{
	BSONObjBuilder objbuilder;
	objbuilder.append("_id",_id);
	objbuilder.append("transactionName",this->_transaction_name);
    objbuilder.append("targetCollection",target_collection);
	objbuilder.append("sequenceNumber",sequence);
	std::string operation;
	
	if(operation_type == rtree_index::INSERT)
	    operation = "INSERT";
	else if(operation_type == rtree_index::UPDATE)
	    operation = "UPDATE";
	else if(operation_type == rtree_index::REMOVE)
	    operation = "REMOVE";
	else
	    operation ="UNKNOWN";
		
	objbuilder.append("operationType",operation);
	objbuilder.append("operationInfo",info);
	time_t timep; 
	time (&timep);
	objbuilder.append("lastModified", std::ctime(&timep));
	
	return Insert(objbuilder.obj());
}

bool CreateIndexTransaction::UpdateDone(unsigned int sequence,std::string target_collection, rtree_index::writeOpt operation_type,std::string info)
{
	BSONObjBuilder objbuilder;
    objbuilder.append("targetCollection",target_collection);
	objbuilder.append("sequenceNumber",sequence);
	objbuilder.append("operationInfo",info);
	std::string operation;
	if(operation_type == rtree_index::INSERT)
	    operation = "INSERT";
	else if(operation_type == rtree_index::UPDATE)
	    operation = "UPDATE";
	else if(operation_type == rtree_index::REMOVE)
	    operation = "REMOVE";
	else
	    operation ="UNKNOWN";
	cout<<"in transaction:"<<operation_type;
	cout<<"in transaction:"<<operation;
	objbuilder.append("operationType",operation);
	time_t timep; 
	time (&timep);
	objbuilder.append("lastModified",std::ctime(&timep));
	
	BSONObjBuilder condition;
	condition.append("_id",_id);
	
	BSONObjBuilder cmdObj;
	cmdObj.append("query", condition.obj());
	cmdObj.append("update", objbuilder.obj());
	
	return Update(cmdObj.obj());
}






}//end of namespace
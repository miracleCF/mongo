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

#include "rtree_core.h"





namespace rtree_index
{
    RTreeIndex::RTree::RTree(int Max_Node, int Max_Leaf, mongo::OID Root, MongoIO* UserIO)
	{
		_Max_Node = Max_Node;
		_Max_Leaf = Max_Leaf;
		_Root = Root;
		IO = UserIO;
	}

	RTreeIndex::RTree::RTree()
	{
		//Nothing is done here 
	}


	void RTreeIndex::RTree::SetRoot(mongo::OID Root)
	{
		_Root = Root;
	}

	bool RTreeIndex::RTree::InsertRoot(OperationContext* txn,mongo::OID &Root)
	{
		Node RootNode(_Max_Leaf);
		Root = IO->Basic_Generate_Key();
		RootNode.NodeKey = Root;
		IO->Basic_Store_Node(txn,RootNode);
		return true;
	}

	bool RTreeIndex::RTree::Insert(OperationContext* txn,mongo::OID &Root, Branch branch2insert, int Level)
	{
		mongo::OID newNode;
		if (Insert2(txn,Root, newNode, branch2insert, Level))
		{

			Node oldRoot=IO->Basic_Find_One_Node(Root);

			mongo::OID newRootOID = IO->Basic_Generate_Key();
			Node newRoot = NewNode(oldRoot.Level + 1, newRootOID);

			IO->Basic_Store_Node(txn,newRoot);

			Branch oldBranch = createBranch(true, createCover(Root), Root);
			Branch newBranch = createBranch(true, createCover(newNode), newNode);

			mongo::OID whatever;
			InsertBranch(txn,newRootOID, oldBranch, whatever);
			InsertBranch(txn,newRootOID, newBranch, whatever);
			Root = newRootOID;
			return true;
		}
		return false;
	}

	bool RTreeIndex::RTree::Insert2(OperationContext* txn,mongo::OID NodeOID, mongo::OID &newNode, Branch branch2insert, int Level)
	{
		Node CurrentNode=IO->Basic_Find_One_Node(NodeOID);

		MBR mbr2insert = branch2insert.mbr;
		int thisLevel = CurrentNode.Level;
		if (thisLevel > Level)
		{
			int i = SelectBestBranch(CurrentNode, mbr2insert);

			mongo::OID targetOID = CurrentNode.Branchs[i].ChildKey;
			if (!Insert2(txn,targetOID, newNode, branch2insert, Level))
			{

				MBR old_MBR = CurrentNode.Branchs[i].mbr;
				MBR m = comBineMBR(mbr2insert, old_MBR);




				bool hasChanged = false;
				if (old_MBR.MinX != m.MinX)hasChanged = true;
				if (old_MBR.MinY != m.MinY)hasChanged = true;
				if (old_MBR.MaxX != m.MaxX)hasChanged = true;
				if (old_MBR.MaxY != m.MaxY)hasChanged = true;
				if (hasChanged)
				{
					Branch updatedBranch = createBranch(true, m, targetOID);
					IO->RTree_ModifyBranch(txn,NodeOID, updatedBranch, i);
				}
				return 0;
			}
			else
			{

				MBR bestMBR = createCover(targetOID);
				Branch updatedBestBranch = createBranch(true, bestMBR, targetOID);
				IO->RTree_ModifyBranch(txn,NodeOID, updatedBestBranch, i);
				Branch newBranch = createBranch(true, createCover(newNode), newNode);
				return InsertBranch(txn,NodeOID, newBranch, newNode);
			}
		}
		else
		{
			return InsertBranch(txn,NodeOID, branch2insert, newNode);
		}
	}
	
		
	bool RTreeIndex::RTree::Search(geos::geom::Geometry  *SearchGeometry , vector<mongo::OID> &Results, vector<bool> &lazyIntersect)
	{
		Search2(_Root, SearchGeometry, 0, Results, lazyIntersect);
		if (Results.size() == 0)
		{
			return false;
		}
		return true;
	}


	
	bool RTreeIndex::RTree::Search2(mongo::OID nodeKey, geos::geom::Geometry * SearchGeometry, int Level, vector<mongo::OID> &OIDList, vector<bool> &lazyIntersect)
	{
		
		Node   currentNode = IO->Basic_Find_One_Node(nodeKey);
		int thisLevel = currentNode.Level;
		int MaxSize =( Level == 0 ? _Max_Leaf : _Max_Node);
		if (thisLevel > Level)
		{
			for (int i = 0; i <MaxSize; i++)
			{
				MBR m = currentNode.Branchs[i].mbr;
				//create a geos linering of node mbr to intersect the input geometry
				//be careful with the memory leak
				//MBR may be a point so fix it.
				geos::geom::CoordinateSequence* cs = csf.create(5, 2);
				cs->setAt(geos::geom::Coordinate(m.MinX, m.MinY, 0), 0);
				cs->setAt(geos::geom::Coordinate(m.MaxX, m.MinY, 0), 1);
				cs->setAt(geos::geom::Coordinate(m.MaxX, m.MaxY, 0), 2);
				cs->setAt(geos::geom::Coordinate(m.MinX, m.MaxY, 0), 3);
				cs->setAt(geos::geom::Coordinate(m.MinX, m.MinY, 0), 4);
				geos::geom::LinearRing *pMBR = factory.createLinearRing(cs);
				geos::geom::Polygon *pPolygon = factory.createPolygon(pMBR, NULL);

				if ( currentNode.Branchs[i].HasData  &&  SearchGeometry->intersects(pPolygon))
				{
					mongo::OID childOID = currentNode.Branchs[i].ChildKey;
					Search2(childOID, SearchGeometry, Level, OIDList, lazyIntersect);

				}
				delete pPolygon;//LinearRing will be delete by ~Polygon() automatically
			}
		}
		else
		{
			for (int i = 0; i < MaxSize; i++)
			{
				MBR m = currentNode.Branchs[i].mbr;
				//create a geos linering of node mbr to intersect the input geometry
				//be careful with the memory leak
				//MBR may be a point so fix it.
				geos::geom::CoordinateSequence* cs = csf.create(5, 2);
				cs->setAt(geos::geom::Coordinate(m.MinX, m.MinY, 0), 0);
				cs->setAt(geos::geom::Coordinate(m.MaxX, m.MinY, 0), 1);
				cs->setAt(geos::geom::Coordinate(m.MaxX, m.MaxY, 0), 2);
				cs->setAt(geos::geom::Coordinate(m.MinX, m.MaxY, 0), 3);
				cs->setAt(geos::geom::Coordinate(m.MinX, m.MinY, 0), 4);
				geos::geom::LinearRing *pMBR = factory.createLinearRing(cs);
				geos::geom::Polygon *pPolygon = factory.createPolygon(pMBR, NULL);
				if (currentNode.Branchs[i].HasData)
				{
					//if geos intersects
					bool Conditions = SearchGeometry->intersects(pPolygon);
					if (Conditions)
					{
						//获取childoid
						mongo::OID childOID = currentNode.Branchs[i].ChildKey;
						if (Level == 0)
						{
							OIDList.push_back(childOID);
							if (SearchGeometry->contains(pPolygon))//contains must be intersect, there is no need to check the intersect condition of this geom again
							{
								lazyIntersect.push_back(true);
							}
							else
							{
								lazyIntersect.push_back(false);
							}
						}
						if (Level != 0)
						{
							OIDList.push_back(childOID);

						}
					}
				}
				delete pPolygon;//LinearRing will be delete by ~Polygon() automatically

			}
		}
		return true;
	}

	
	Branch  RTreeIndex::RTree::createBranch(bool HasData, MBR m, mongo::OID childKey)
	{
		Branch theBranch2create;
		theBranch2create.HasData = HasData;
		theBranch2create.mbr = m;
		theBranch2create.ChildKey = childKey;
		return theBranch2create;
	}

	
	MBR RTreeIndex::RTree::comBineMBR(MBR m1, MBR m2)
	{
		double minx, miny, maxx, maxy;
		minx = m1.MinX < m2.MinX ? m1.MinX : m2.MinX;
		miny = m1.MinY < m2.MinY ? m1.MinY : m2.MinY;
		maxx = m1.MaxX > m2.MaxX ? m1.MaxX : m2.MaxX;
		maxy = m1.MaxY > m2.MaxY ? m1.MaxY : m2.MaxY;
		return MBR(minx, miny, maxx, maxy);
	}

	
	int RTreeIndex::RTree::SelectBestBranch(Node CurrentNode, MBR m)
	{

		int best=0;
		double bestIncrease;

		int Countf = 0;
		int MaxSize = (CurrentNode.Level == 0 ? _Max_Leaf : _Max_Node);

		for (int i = 0; i < MaxSize; i++)
		{
			if (CurrentNode.Branchs[i].HasData)
			{

				MBR currentMBRi = CurrentNode.Branchs[i].mbr;
				if (Countf == 0)
				{
					best = i;
					double area = RTreeMBRSphericalVolume(currentMBRi);
					MBR cm = comBineMBR(m, currentMBRi);
					bestIncrease = RTreeMBRSphericalVolume(cm) - area;
					Countf++;
				}
				else
				{
					double area = RTreeMBRSphericalVolume(currentMBRi);
					MBR cm = comBineMBR(m, currentMBRi);
					double increase;
					increase = RTreeMBRSphericalVolume(cm) - area;
					if (increase < bestIncrease)
					{
						bestIncrease = increase;
						best = i;
					}
				}
			}
		}

		return best;
	}

	
	double RTreeIndex::RTree::RTreeMBRSphericalVolume(MBR m)
	{

		//register int i;
		register double sum_of_squares = 0, radius;
		double half_extend;
		half_extend = (m.MaxX - m.MinX) / 2;
		sum_of_squares += (half_extend*half_extend);
		half_extend = (m.MaxY - m.MinY) / 2;
		sum_of_squares += (half_extend*half_extend);

		radius = sqrt(sum_of_squares);
		return (double)(pow(radius, 2) * 3.141593);
	}

	
	bool RTreeIndex::RTree::InsertBranch(OperationContext* txn,mongo::OID terget, Branch branch2insert, mongo::OID &newNode)
	{

		Node CurrentNode=IO->Basic_Find_One_Node(terget);

		int Level = CurrentNode.Level;
		int Count = CurrentNode.Count;
		int MaxN = Level > 0 ? _Max_Node : _Max_Leaf;
		if (Count < MaxN)
		{

			for (int i = 0; i < MaxN; i++)
			{
				if (!CurrentNode.Branchs[i].HasData)
				{
			
					IO->RTree_ModifyBranch(txn,terget, branch2insert, i);
				
					IO->RTree_ModifyCount(txn,terget, Count + 1);
					break;
				}
			}
			return false;
		}
		else
		{

			newNode = IO->Basic_Generate_Key();
	
			splitNode(txn,terget, branch2insert, newNode);
	
			IO->RTree_ModifyCount(txn,terget, IO->RTree_Calculate_Data_Branch_Count(terget));
			IO->RTree_ModifyCount(txn,newNode, IO->RTree_Calculate_Data_Branch_Count(newNode));

			return true;
		}
		return false;
	}

	
	bool RTreeIndex::RTree::splitNode(OperationContext* txn,mongo::OID currentOID, Branch Branch2Insert, mongo::OID &newNode)
	{
		Node CurrentNode=IO->Basic_Find_One_Node(currentOID);

		vector<Branch> Alldata;
		vector<MBR> AllMBRCatch;

		int Max_Size = (CurrentNode.Level == 0 ? _Max_Leaf : _Max_Node);
		for (int i = 0; i < Max_Size; i++)
		{
			Alldata.push_back(CurrentNode.Branchs[i]);
			AllMBRCatch.push_back(CurrentNode.Branchs[i].mbr);
		}
		Alldata.push_back(Branch2Insert);
		AllMBRCatch.push_back(Branch2Insert.mbr);



		//findSeed
		int SeedA; int SeedB;


		double MaxArea = 0;
		vector<bool> Visited;
		for (unsigned int i = 0; i < Alldata.size(); i++)
		{
			Visited.push_back(false);
			for (unsigned int j = 0; j < Alldata.size(); j++)
			{
				if (i != j)
				{
					MBR m1 = AllMBRCatch[i];
					MBR m2 = AllMBRCatch[j];
					MBR m3 = comBineMBR(m1, m2);
					double carea = RTreeMBRSphericalVolume(m3);
					if (carea > MaxArea)
					{
						SeedA = i; SeedB = j;
						MaxArea = carea;
					}
				}
			}
		}
		//cout << SeedA << endl << SeedB << endl;
		Visited[SeedA] = true; Visited[SeedB] = true;
		vector<int> ClassA; vector<int>ClassB;
		ClassA.push_back(SeedA); ClassB.push_back(SeedB);
		MBR mbrA = AllMBRCatch[SeedA];
		MBR mbrB = AllMBRCatch[SeedB];
		int MaxN = CurrentNode.Level > 0 ? _Max_Node : _Max_Leaf;

		while (ClassA.size() + ClassB.size() < static_cast<unsigned int>( MaxN + 1))
		{
		
			double maxIncressDif = 0;
			double bestCandidate = -1;
			for (unsigned int i = 0; i < Visited.size(); i++)
			{
				if (Visited[i] == false)
				{
					double increaseA;
					double increaseB;
					if (bestCandidate == -1)
					{
						bestCandidate = i;
						increaseA = RTreeMBRSphericalVolume(comBineMBR(mbrA, AllMBRCatch[i])) - RTreeMBRSphericalVolume(mbrA);
						increaseB = RTreeMBRSphericalVolume(comBineMBR(mbrB, AllMBRCatch[i])) - RTreeMBRSphericalVolume(mbrB);
						maxIncressDif = increaseA - increaseB;


					}
					else
					{
						increaseA = RTreeMBRSphericalVolume(comBineMBR(mbrA, AllMBRCatch[i])) - RTreeMBRSphericalVolume(mbrA);
						increaseB = RTreeMBRSphericalVolume(comBineMBR(mbrB, AllMBRCatch[i])) - RTreeMBRSphericalVolume(mbrB);
						if (abs(increaseA - increaseB) > abs(maxIncressDif))
						{
							bestCandidate = i;
							maxIncressDif = increaseA - increaseB;
						}
					}
				}
			}
		
			if (ClassA.size() + Alldata.size() - ClassA.size() - ClassB.size() > static_cast<unsigned int>(MaxN / 2) && ClassB.size() + Alldata.size() - ClassA.size() - ClassB.size() > static_cast<unsigned int> (MaxN / 2))
			{
		
				if (maxIncressDif > 0)
				{
					ClassB.push_back(bestCandidate);
					mbrB = comBineMBR(mbrB, AllMBRCatch[bestCandidate]);
				}
				else
				{
					ClassA.push_back(bestCandidate);
					mbrA = comBineMBR(mbrA, AllMBRCatch[bestCandidate]);
				}
				Visited[bestCandidate] = true;
				continue;
			}
			if (ClassA.size() + Alldata.size() - ClassA.size() - ClassB.size() <=static_cast<unsigned int> (MaxN / 2))
			{
				for (unsigned int i = 0; i < Alldata.size(); i++)
				{
					if (Visited[i] == false)
					{
						Visited[i] = true;
						ClassA.push_back(i);
						mbrA = comBineMBR(mbrA, AllMBRCatch[i]);
					}
				}
				continue;
			}
			if (ClassB.size() + Alldata.size() - ClassA.size() - ClassB.size() <= static_cast<unsigned int>( MaxN / 2))
			{
				for (unsigned int i = 0; i < Alldata.size(); i++)
				{
					if (Visited[i] == false)
					{
						Visited[i] = true;
						ClassB.push_back(i);
						mbrB = comBineMBR(mbrB, AllMBRCatch[i]);
					}
				}
				continue;
			}

		}//end while

		//assert(ClassA.size() >= MaxN / 2 && ClassB.size() >= MaxN / 2);
		vector<Branch> AllBranches2Modify;
		for (int i = 0; i < MaxN; i++)
		{

		


			if (static_cast<unsigned int> (i) < ClassA.size())
			{
				AllBranches2Modify.push_back(Alldata[ClassA[i]]);
			}
			else
			{
				MBR m;
				Branch emptyBranch = Branch();
				AllBranches2Modify.push_back(emptyBranch);
			}
		}
		IO->RTree_ModifyAllBranch(txn,currentOID, AllBranches2Modify);


	
		newNode = IO->Basic_Generate_Key();
		Node theNewNode = NewNode(CurrentNode.Level, newNode);

		IO->Basic_Store_Node(txn,theNewNode);
	
		//Need Improvement
		for (int i = 0; i < MaxN; i++)
		{
			if (static_cast<unsigned int> (i) < ClassB.size())
			{
				IO->RTree_ModifyBranch(txn,newNode, Alldata[ClassB[i]], i);
			}
			else
			{
				break;
			}
		}
		return true;

	}

	
	MBR RTreeIndex::RTree::createCover(mongo::OID keyofcreate)
	{
		Node target=IO->Basic_Find_One_Node(keyofcreate);
		int Max_Size = (target.Level == 0 ? _Max_Leaf : _Max_Node);
		int Countf = 0;
		MBR m;
		for (int i = 0; i < Max_Size; i++)
		{
			if (target.Branchs[i].HasData)
			{
				//parse MBR
				if (Countf == 0)
				{
					m = target.Branchs[i].mbr;
					Countf++;
				}
				else
				{
					m = comBineMBR(m, target.Branchs[i].mbr);
				}
			}
		}
		return m;
	}

	
	Node RTreeIndex::RTree::NewNode(int Level, mongo::OID NodeKey)
	{
		if (Level == 0)
		{
			Node node(_Max_Node);
			node.NodeKey = NodeKey;
			node.Level = Level;
			return node;
		}
		else
		{
			Node node(_Max_Leaf);
			node.NodeKey = NodeKey;
			node.Level = Level;
			return node;
		}
		
	}

	
	int RTreeIndex::RTree::Intersect(MBR m1, MBR m2)
	{
		if (m1.MinX > m2.MaxX || m2.MinX > m1.MaxX || m1.MinY > m2.MaxY || m2.MinY > m1.MaxY)
		{
			return 0;
		}


		if (m1.MinX >= m2.MinX&&m1.MaxX <= m2.MaxX&&m1.MinY >= m2.MinY&&m1.MaxY <= m2.MaxY)
		{
			return 1;
		}
		return -1;


	}


	bool RTreeIndex::RTree::DeleteNode(OperationContext* txn,mongo::OID &RootKey, mongo::OID KeyNode2Delete, MBR mbrOfDeleteNode)
	{
		vector<mongo::OID> reInsertList;
		if (DeleteNode2(txn,RootKey, KeyNode2Delete, mbrOfDeleteNode, reInsertList))
		{
			while (reInsertList.size() > 0)
			{
				mongo::OID theOID = reInsertList.back();
				Node oldNode = IO->Basic_Find_One_Node(theOID);
				int level2ReInsert = oldNode.Level;


				int MaxSize = level2ReInsert > 0 ? _Max_Node : _Max_Leaf;
				for (int i = 0; i <MaxSize; i++)
				{
					if (oldNode.Branchs[i].HasData)
					{
						MBR mm = oldNode.Branchs[i].mbr;
						Branch b2ri;
						b2ri.HasData = true;
						b2ri.mbr = mm;
						b2ri.ChildKey = oldNode.Branchs[i].ChildKey;
						Insert(txn,RootKey, b2ri, level2ReInsert);
					}
				}
			
				BSONObjBuilder result;
				IO->Basic_Delect_Node_By_Key(txn,theOID,result);
				reInsertList.pop_back();
			}
		
			Node rootNode=IO->Basic_Find_One_Node(RootKey);
			int countr = rootNode.Count;
			int levelr = rootNode.Level;
			if (countr == 1 && levelr>0)
			{
				int Max_Size = levelr > 0 ? _Max_Node : _Max_Leaf;
				for (int i = 0; i < Max_Size; i++)
				{
					if (rootNode.Branchs[i].HasData)
					{
						mongo::OID oldRootOID = RootKey;
						RootKey = rootNode.Branchs[i].ChildKey;
						//delete the old root from db
						BSONObjBuilder result;
						IO->Basic_Delect_Node_By_Key(txn,oldRootOID,result);
						//modify collection:   _RTreeIndex 's root info
					
						break;
					}
				}
			}
		}
		return true;
	}

	
	bool RTreeIndex::RTree::DeleteNode2(OperationContext* txn,mongo::OID NodeKey, mongo::OID Key2Delete, MBR mbrOFDeletingNode, vector<mongo::OID> &L)
	{
		Node currentNode = IO->Basic_Find_One_Node(NodeKey);
		int theLevel = currentNode.Level;
		int currentCount = currentNode.Count;
		int Max_Size = theLevel > 0 ? _Max_Node : _Max_Leaf;

		if (theLevel > 0)
		{
			for (int i = 0; i < Max_Size; i++)
			{
				if (currentNode.Branchs[i].HasData && Intersect(mbrOFDeletingNode, currentNode.Branchs[i].mbr))
				{
					if (DeleteNode2(txn,currentNode.Branchs[i].ChildKey, Key2Delete, mbrOFDeletingNode, L))
					{
						Node Child_I = IO->Basic_Find_One_Node(currentNode.Branchs[i].ChildKey);
						int NodesNumOfChild = Child_I.Count;
						if (NodesNumOfChild >= _Max_Node/2)
						{
							MBR m = createCover(currentNode.Branchs[i].ChildKey);
							Branch modifiedBranch;
							modifiedBranch.HasData = true;
							modifiedBranch.mbr = m;
							modifiedBranch.ChildKey = currentNode.Branchs[i].ChildKey;
							IO->RTree_ModifyBranch(txn,NodeKey,modifiedBranch,i);
						}
						else
						{
							L.push_back(currentNode.Branchs[i].ChildKey);
							//DisconnectBranch
							MBR emptyM;
							Branch disConnectedBranch;
							disConnectedBranch.HasData = false;
							disConnectedBranch.mbr = emptyM;
							disConnectedBranch.ChildKey = NodeKey;
							IO->RTree_ModifyBranch(txn,NodeKey, disConnectedBranch, i);
							IO->RTree_ModifyCount(txn,NodeKey,currentCount-1);
							//endDisconnect
						}
						return true;
					}
				}

			}
		}
		else
		{
			for (int i = 0; i < Max_Size; i++)
			{
				if (currentNode.Branchs[i].ChildKey == Key2Delete)
				{
					MBR emptyM;
					Branch disConnectedBranch;
					disConnectedBranch.HasData = false;
					disConnectedBranch.mbr = emptyM;
					disConnectedBranch.ChildKey = NodeKey;
					IO->RTree_ModifyBranch(txn,NodeKey, disConnectedBranch, i);
					IO->RTree_ModifyCount(txn,NodeKey, currentCount - 1);
					return true;
				}
			}
		}
		return false;//this is not supposed to happen
	}


	
	bool RTreeIndex::RTree::ReConfigure(int Max_Node, int Max_Leaf, mongo::OID Root, string DB_NAME, string STORAGE_NAME)
	{
		_Max_Node = Max_Node;
		_Max_Leaf = Max_Leaf;
		_Root = Root;
		IO->Configure(DB_NAME, STORAGE_NAME, _Max_Node, _Max_Leaf);
		return true;
	}
	
	


	


	



	
	
	


	

    
}
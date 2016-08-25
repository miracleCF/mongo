#include "snapshot.h"

MBR::MBR()
{
    MinX = 0;
    MinY = 0;
    MaxY = 0;
    MaxX = 0;
}
MBR::MBR(double minx, double miny, double maxx, double maxy)
{
    MinX = minx;
    MinY = miny;
    MaxX = maxx;
    MaxY = maxy;
}


Branch::Branch()
{
    HasData = false;
}

Branch::Branch(double minx, double miny, double maxx, double maxy, mongo::OID key):mbr (maxx, maxy, minx, miny)
{
    HasData = true;
    ChildKey = key;
}

Node::Node(int Max_Branch_Size)
{
    for (int i = 0; i < Max_Branch_Size; i++)
    {
        Branch b;
        Branchs.push_back(b);
    }
    Count = 0;
    Level = 0;
}


//public function here
Node BSONObj2Node(BSONObj theNodeBSON)
{
    int Count = theNodeBSON["Count"].Int();
    int Level = theNodeBSON["Level"].Int();
    BSONObj AllBranches = theNodeBSON["Branchs"].Obj();
    vector<BSONElement> L;
    AllBranches.elems(L);
    Node theReturnNode(L.size());
    theReturnNode.Count = Count;
    theReturnNode.Level = Level;
    for (unsigned int i = 0; i < L.size(); i++)
    {
        BSONObj OneBranchBSON = L[i].Obj();
        theReturnNode.Branchs[i].HasData = OneBranchBSON["HasData"].boolean();
        theReturnNode.Branchs[i].ChildKey = OneBranchBSON["ChildID"].OID();
        BSONObj Values = OneBranchBSON["MBR"].Obj();
        vector<BSONElement> LL;
        Values.elems(LL);
        MBR m(LL[0].Double(), LL[1].Double(), LL[2].Double(), LL[3].Double());
        theReturnNode.Branchs[i].mbr = m;
    }
    return theReturnNode;
}

bool Intersects(MBR m1,MBR m2)
{
    if(m1.MinX>m2.MaxX || m1.MinY>m2.MaxY ||m2.MinX>m1.MaxX ||m2.MinY>m1.MaxY)
    {
        return false;
    }
    return true;

}


SnapShot::SnapShot(DBClientBase *conn, string dbName, string collectionName)
{
    _conn=conn;
    _dbName=dbName;
    _collectionName=collectionName;
}

vector<MBR> SnapShot::getSnapShot(MBR viewMBR)
{
    Level2Find=-1;
    //getRTreeRoot;
    vector<MBR> returnSnapShot;
    BSONObjBuilder geometadatacondbdr;
    string ns=_dbName+"."+_collectionName;
    geometadatacondbdr.append("NAMESPACE",ns);
    BSONObj geometadataObj=_conn->findOne("config.meta_geom",geometadatacondbdr.obj());
    if(geometadataObj.isEmpty())
    {
        return returnSnapShot;
    }
    else
    {
        mongo::OID IndexOID=geometadataObj["INDEX_INFO"].OID();
        BSONObjBuilder indexmetacondbdr;
        indexmetacondbdr.append("_id",IndexOID);
        BSONObj indexmetaObj=_conn->findOne("config.meta_rtree",indexmetacondbdr.obj());
        if(indexmetaObj.isEmpty())
        {
            return returnSnapShot;
        }
        else
        {
            string rtreens=_dbName+"."+"rtree_"+_collectionName;
            mongo::OID rootOID=indexmetaObj["INDEX_ROOT"].OID();
            BSONObjBuilder rootcondbdr;
            rootcondbdr.append("_id",rootOID);
            BSONObj rootNode=_conn->findOne(rtreens,rootcondbdr.obj());
            if(rootNode.isEmpty())
            {
                return returnSnapShot;
            }
            else
            {
                int maxLevel=rootNode["Level"].Int();
                //start get MBR snapShot, the num of snapshoted MBR must not exceed 500.
                vector<Node> NodeSnapShot;
                int pLevel=maxLevel;
                Node root=BSONObj2Node(rootNode);
                int Max_Node=root.Branchs.size();
                NodeSnapShot.push_back(root);
                while(pLevel>=0)
                {
                   Level2Find=pLevel;
                   vector<mongo::OID> childOIDs;
                   returnSnapShot.clear();
                   for(int i=0;i<NodeSnapShot.size();i++)
                   {
                       Node tempNode=NodeSnapShot[i];
                       for(int j=0;j<tempNode.Branchs.size();j++)
                       {
                           if(tempNode.Branchs[j].HasData && Intersects(tempNode.Branchs[j].mbr,viewMBR) )
                           {
                               childOIDs.push_back(tempNode.Branchs[j].ChildKey);
                               returnSnapShot.push_back(tempNode.Branchs[j].mbr);
                           }
                       }
                   }
                   if(pLevel==0 || childOIDs.size()*Max_Node/2>_maxMBRInView )
                   {
                      break;
                      //exit while, this is the end, more mbr will cause draw effiency problems
                      //current BMR is enough for the run
                   }
                   else
                   {
                       if(pLevel>0)
                       {
                        //to the next level;
                        NodeSnapShot.clear();
                        returnSnapShot.clear();
                        //to vector
                        for(int i=0;i<childOIDs.size();i++)
                        {
                            mongo::OID oneOID=childOIDs[i];
                            BSONObjBuilder bdr;
                            bdr.append("_id",oneOID);
                            BSONObj oneNodeObj=_conn->findOne(_dbName+".rtree_"+_collectionName,bdr.obj());
                            Node pNode=BSONObj2Node(oneNodeObj);
                            NodeSnapShot.push_back(pNode);
                        }
                       }

                   }
                    --pLevel;
                }
            }
        }
    }
    //get MBR of the data
    return returnSnapShot;

}

MBR SnapShot::getEnvelope()
{
    //getRTreeRoot;
    BSONObjBuilder geometadatacondbdr;
    string ns=_dbName+"."+_collectionName;
    geometadatacondbdr.append("NAMESPACE",ns);
    BSONObj geometadataObj=_conn->findOne("config.meta_geom",geometadatacondbdr.obj());
    if(geometadataObj.isEmpty())
    {
        return MBR();
    }
    else
    {
        mongo::OID IndexOID=geometadataObj["INDEX_INFO"].OID();
        BSONObjBuilder indexmetacondbdr;
        indexmetacondbdr.append("_id",IndexOID);
        BSONObj indexmetaObj=_conn->findOne("config.meta_rtree",indexmetacondbdr.obj());
        if(indexmetaObj.isEmpty())
        {
            return MBR();
        }
        else
        {
            string rtreens=_dbName+"."+"rtree_"+_collectionName;
            mongo::OID rootOID=indexmetaObj["INDEX_ROOT"].OID();
            BSONObjBuilder rootcondbdr;
            rootcondbdr.append("_id",rootOID);
            BSONObj rootNode=_conn->findOne(rtreens,rootcondbdr.obj());
            if(rootNode.isEmpty())
            {
                return MBR();
            }
            else
            {
                Node root=BSONObj2Node(rootNode);
                MBR m;
                bool isFirst=true;
                for(int i=0;i<root.Branchs.size();i++)
                {
                    if(root.Branchs[i].HasData)
                    {
                        if(isFirst)
                        {
                            m=root.Branchs[i].mbr;
                            isFirst=false;
                        }
                        else
                        {
                            Branch b=root.Branchs[i];
                            double minx= (m.MinX<b.mbr.MinX?m.MinX:b.mbr.MinX);
                            double miny= (m.MinY<b.mbr.MinY?m.MinY:b.mbr.MinY);
                            double maxx=(m.MaxX>b.mbr.MaxX? m.MaxX:b.mbr.MaxX);
                            double maxy=(m.MaxY>b.mbr.MaxY? m.MaxY:b.mbr.MaxY);
                            m=MBR(minx,miny,maxx,maxy);
                        }

                    }

                }
                return m;

            }
        }
    }
    return MBR();

}



string SnapShot::getGeometryField()
{
    //getRTreeRoot;
    BSONObjBuilder geometadatacondbdr;
    string ns=_dbName+"."+_collectionName;
    geometadatacondbdr.append("NAMESPACE",ns);
    BSONObj geometadataObj=_conn->findOne("config.meta_geom",geometadatacondbdr.obj());
    if(geometadataObj.isEmpty())
    {
        return "";
    }
    else
    {
        string geofield=geometadataObj["COLUMN_NAME"].str();
        return geofield;
    }
    return "";


}




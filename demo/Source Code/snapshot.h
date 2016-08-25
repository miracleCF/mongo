#ifndef SNAPSHOT_H
#define SNAPSHOT_H


#if defined(_WIN32)
#include <winsock2.h>
#include <windows.h>
#endif
#include "mongo/client/dbclient.h"
#include "string"
#include <vector>
using namespace std;
using namespace mongo;

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

class SnapShot
{
public:
    SnapShot(DBClientBase * conn, string dbName,string collectionName);
    vector<MBR> getSnapShot(MBR viewMBR);
    MBR getEnvelope();

    string getGeometryField();
    int Level2Find=-1;
private:
    int _maxMBRInView=1000;
    DBClientBase * _conn;
    string _dbName="";
    string _collectionName="";
};



#endif // SNAPSHOT_H

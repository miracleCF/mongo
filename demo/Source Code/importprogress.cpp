#include "importprogress.h"
#include "ui_importprogress.h"
#include <thread>

#include <QMessageBox>
#include "gdal.h"
#include "ogr_api.h"
#include "ogrsf_frmts.h"

int MaxSize=0;
int Countf=0;
bool importShapefile(DBClientBase * _conn,string dbName, string collectionName, string path)
{

            GDALAllRegister();
            OGRRegisterAll();
            Countf=0;
            cout << "DBNAME:" << dbName << endl;
            cout << "COLLECTION_NAME:" << collectionName << endl;

#ifdef _MSC_VER
            OGRSFDriver* poDriver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName("ESRI Shapefile");
            std::string FileCompletePath = path;
            const char* c_s = FileCompletePath.c_str();
            OGRDataSource* poDS = poDriver->Open(c_s, NULL);
#else
            GDALDataset *poDS;
            const char* c_s = path.c_str();
            poDS = (GDALDataset*) GDALOpenEx( c_s, GDAL_OF_VECTOR, NULL, NULL, NULL );
            if( poDS == NULL )
            {
                printf( "Open failed.\n" );
                return false;
            }
#endif


            OGRLayer* poLayer = poDS->GetLayer(0);
            poLayer->ResetReading();
            MaxSize=poLayer->GetFeatureCount();
            OGRFeature *pFeature;
            while (true)
            {
                pFeature = poLayer->GetNextFeature();
                if (pFeature == NULL)
                {
                    break;
                }
                OGRGeometry* pGeometry = pFeature->GetGeometryRef();
                if (pGeometry == NULL)
                {
                    cout<<"Geometry get failed."<<endl;
                    return FALSE;
                }

                OGRwkbGeometryType geoType = pGeometry->getGeometryType();
                if (wkbPoint == geoType)
                {
                    OGRPoint* pPoint = (OGRPoint *)pGeometry;
                    BSONObjBuilder pGeometrybdr;
                    BSONObjBuilder mbrPointBuilder;
                    mbrPointBuilder.append("minx", pPoint->getX());
                    mbrPointBuilder.append("miny", pPoint->getY());
                    mbrPointBuilder.append("maxx", pPoint->getX());
                    mbrPointBuilder.append("maxy", pPoint->getY());
                    pGeometrybdr.append("MBR", mbrPointBuilder.obj());
                    BSONObj GeoJSON = fromjson(pPoint->exportToJson());
                    pGeometrybdr.append("GeoJSON", GeoJSON);
                    _conn->insert(dbName + "." + collectionName, pGeometrybdr.obj());

                }
                else if (wkbLineString == geoType)
                {
                    OGRLineString* pLineGeo = (OGRLineString*)pGeometry;
                    BSONObjBuilder pGeometrybdr;
                    OGREnvelope ev;
                    pLineGeo->getEnvelope(&ev);
                    BSONObjBuilder mbrPointBuilder;
                    mbrPointBuilder.append("minx", ev.MinX);
                    mbrPointBuilder.append("miny", ev.MinY);
                    mbrPointBuilder.append("maxx", ev.MaxX);
                    mbrPointBuilder.append("maxy", ev.MaxY);
                    pGeometrybdr.append("MBR", mbrPointBuilder.obj());
                    BSONObj GeoJSON = fromjson(pLineGeo->exportToJson());
                    pGeometrybdr.append("GeoJSON", GeoJSON);
                    _conn->insert(dbName + "." + collectionName, pGeometrybdr.obj());
                }
                else if (wkbPolygon==geoType)
                {
                    BSONObjBuilder pGeometrybdr;
                    //获取BMR
                    OGREnvelope ev;
                    pGeometry->getEnvelope(&ev);
                    BSONObjBuilder mbrPointBuilder;
                    mbrPointBuilder.append("minx", ev.MinX);
                    mbrPointBuilder.append("miny", ev.MinY);
                    mbrPointBuilder.append("maxx", ev.MaxX);
                    mbrPointBuilder.append("maxy", ev.MaxY);
                    pGeometrybdr.append("MBR", mbrPointBuilder.obj());
                    /*char * c = pGeometry->exportToJson();
                    BSONObj GeoJSON = fromjson(c);
                    delete []c;*/
                    BSONObj GeoJSON = fromjson(pGeometry->exportToJson());
                    pGeometrybdr.append("GeoJSON", GeoJSON);
                    //if (pGeometry->IsValid())
                    //{

                        _conn->insert(dbName + "." + collectionName, pGeometrybdr.obj());
                    //}
                    //else
                    //{
                      //  cout <<endl<< "invalid data" << endl;
                    //}

                }
                Countf++;
                OGRFeature::DestroyFeature( pFeature );
            }



}


ImportProgress::ImportProgress(string connS,string dbName,string collectionName,string path,QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ImportProgress)
{
    ui->setupUi(this);
    _db=dbName;
    _coll=collectionName;
    _path=path;

    string errmsg;
    ConnectionString cs = ConnectionString::parse(connS, errmsg);
    if (!cs.isValid()) {
        this->close();
    }
    newconn=cs.connect(errmsg);
    if (!newconn) {
        string stdconnErr="Can not connect to :"+connS;
        QMessageBox::information(NULL, "Error", QString::fromStdString(stdconnErr));
        return;
    }
    std::thread thd1(std::bind(&importShapefile, newconn,dbName,collectionName,path));
    thd1.detach();



    timer=new QTimer(this);
    connect(timer,SIGNAL(timeout()),this,SLOT(timerUpDate()));
    timer->start(100);
    this->setAttribute(Qt::WA_DeleteOnClose,true);
}

ImportProgress::~ImportProgress()
{
    delete ui;
    delete timer;
}


void ImportProgress::timerUpDate()

{
    if(isFirstTime)
    {
        if(MaxSize>0)
        {
            ui->progressBar->setRange(0,MaxSize);
            isFirstTime=false;
        }
    }
    else
    {
        ui->progressBar->setValue(Countf);
        string displayStr="";
        displayStr+="dbName:\t";
        displayStr+=_db;
        displayStr+="\t";
        displayStr+="collectionName:\t";
        displayStr+=_coll;
        displayStr+="\n";
        displayStr+="Current Progress:\t";
        stringstream ss;
        ss<<Countf<<"/"<<MaxSize;
        displayStr+= ss.str();
        ui->label_2->setText(QString::fromStdString(displayStr));
        if(Countf==MaxSize)
        {
            newconn->~DBClientBase();
            this->close();
        }
    }


}

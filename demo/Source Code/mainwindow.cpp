#include "mainwindow.h"
#include "ui_mainwindow.h"



using namespace std;
using namespace mongo;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    map1=new MapWidget(this);
    ui->gridLayout->addWidget(map1);





}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionConnect_to_MongoDB_triggered()
{
    //open connectionString dialog
     ConnectionDialog *connDialog=new ConnectionDialog(this);
     string nameSpaceStringFromDialog="";
     if(connDialog->exec()==QDialog::Accepted)
     {
         nameSpaceStringFromDialog=connDialog->connectionString;
         std::cout<<">>> Tring to create Connection"<<std::endl;
         std::string errmsg;
         ConnectionString cs = ConnectionString::parse(nameSpaceStringFromDialog, errmsg);
         if (!cs.isValid()) {
             std::cout << "Error parsing connection string " << nameSpaceStringFromDialog << ": " << errmsg << std::endl;
         }
         std::cout<<">>> Trying to test conn"<<std::endl;
         _conn=cs.connect(errmsg);
         if (!_conn) {
             cout << "couldn't connect : " << errmsg << endl;
             string stdconnErr="Can not connect to :"+nameSpaceStringFromDialog;
             QMessageBox::information(NULL, "Error", QString::fromStdString(stdconnErr));
             return;
         }
         //connect success
         _currentConnectionString=nameSpaceStringFromDialog;
         ui->actionConnect_to_MongoDB->setEnabled(false);
         ui->actionImport_shapefile_shp->setEnabled(true);
         ui->actionRefresh_layer->setEnabled(true);
         ui->actionZoom_in->setEnabled(true);
         ui->actionZoom_out->setEnabled(true);
         ui->actionZoom_to_full_extend->setEnabled(true);
         ui->action_geoIntersects->setEnabled(true);
         ui->action_geoNear->setEnabled(true);
         ui->action_geoWithin->setEnabled(true);
         ui->actionMove->setEnabled(true);

         goodsModel = new QStandardItemModel();
         //start loading ns of the collections
         refreshTreeView();

     }
     delete connDialog;


}

void MainWindow::refreshTreeView()
{
    goodsModel->clear();
    ui->treeView->setModel(goodsModel);
    list<string> dbList=_conn->getDatabaseNames();
    QIcon dbIcon("icons/dbConnect.png");
    QIcon collIcon("icons/collection.png");
    QIcon indexIcon("icons/element.png");
    list<string>::iterator it;

    for(it=dbList.begin();it!=dbList.end();++it)
    {
        if(*it!="config")
        {
        QList<QStandardItem *> items;//List容器
        QString s=QString::fromStdString(*it);
        QStandardItem *item = new QStandardItem(s);
        item->setIcon(dbIcon);
        item->setEditable(false);

        items.append(item);
        goodsModel->appendRow(items);

         QList<QStandardItem *> childItems;
         list<string> collList=_conn->getCollectionNames(*it,BSONObj());
         list<string>::iterator iit;
         for(iit=collList.begin();iit!=collList.end();++iit)
         {
             QString ss=QString::fromStdString(*iit);
             string nsToClassify=*iit;
             string head=nsToClassify.substr(0,6);
             if(head!="rtree_")
             {
                 //querying config to check out if it exist
                 BSONObjBuilder geometadatacondbdr;
                 string ns=*it+"."+*iit;
                 geometadatacondbdr.append("NAMESPACE",ns);
                 BSONObj geometadataObj=_conn->findOne("config.meta_geom",geometadatacondbdr.obj());
                 if(!geometadataObj.isEmpty())
                 {
                     QStandardItem *childitem = new QStandardItem(ss);

                     childitem->setIcon(collIcon);

                     childitem->setEditable(false);
                     childItems.append(childitem);
                 }
             }
         }
         items.at(0)->appendRows(childItems);
        }
    }
}

void MainWindow::on_actionRefresh_layer_triggered()
{
    refreshTreeView();
}

void MainWindow::on_treeView_clicked(const QModelIndex &index)
{

    if(index.parent().isValid())
    {

          _currentDB=index.parent().data().toString().toStdString();
          _currentColl=index.data().toString().toStdString();
          string ns=_currentDB+"."+_currentColl;
          map1->Direct2NS(_conn,_currentDB,_currentColl);
          this->setWindowTitle(QString::fromStdString(ns));

    }
    else
    {
        this->setWindowTitle(index.data().toString());
        _currentDB=index.data().toString().toStdString();
        _currentColl="";
    }
}

void MainWindow::on_actionImport_shapefile_shp_triggered()
{
     ImportShapefileDialog *importshpdialog=new ImportShapefileDialog(_currentDB,_currentColl,this);
     bool isGO=false;
     if(importshpdialog->exec()==QDialog::Accepted)
     {
         isGO=true;
     }
     string dbName=importshpdialog->_nameOfSelectedDB;
     string shpPath=importshpdialog->_filePath;
     string collectionName=importshpdialog->_collName;
     delete importshpdialog;

     //query the config if the collection exists
     SnapShot snapshot=SnapShot(_conn,dbName,collectionName);
     if(snapshot.getGeometryField()=="")
     {
         //need registerGeometry
         registerGeometry *rg=new registerGeometry(this);
         if(rg->exec()==QDialog::Accepted)
         {
             //registering
             BSONObjBuilder bdr;
             bdr.append("registerGeometry",1.0);
             bdr.append("collectionName",collectionName);
             bdr.append("field","GeoJSON");// if you use our app, the geoField will be GeoJSON
             bdr.append("gtype",rg->_geoType);
             bdr.append("torrance",0.00001);
             bdr.append("srid",4326.0);
             bdr.append("crstype",0.0);
             //createIndex
             BSONObj cmdObj=bdr.done();
             BSONObj info;
             _conn->runCommand(dbName,cmdObj,info);
             _conn->createIndex(dbName+"."+collectionName, fromjson("{GeoJSON:\"rtree\"}"));
         }
         else
         {
             isGO=false;
         }
         delete rg;
     }

     if(isGO)
     {
        ImportProgress *progress=new ImportProgress(_currentConnectionString,dbName,collectionName,shpPath);
        progress->exec();
        refreshTreeView();
        //delete(progress);
        //don't be surprised when you see no delete here
        //because we set the Attribute WA_DeleteOnClose =ture
        //in the construtor of the ImportPrograss
     }
}



void MainWindow::on_actionExit_triggered()
{
    this->close();
}

void MainWindow::on_actionMove_triggered()
{
    map1->setCurrentTool(1);
}

void MainWindow::on_actionZoom_to_full_extend_triggered()
{
    map1->Direct2NS(_conn,_currentDB,_currentColl);
}

void MainWindow::on_action_geoNear_triggered()
{
    map1->setCurrentTool(4);
}

void MainWindow::on_action_geoIntersects_triggered()
{
    map1->setCurrentTool(3);
}

void MainWindow::on_action_geoWithin_triggered()
{
    map1->setCurrentTool(2);
}

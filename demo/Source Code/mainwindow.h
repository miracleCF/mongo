#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#if defined(_WIN32)
#include <winsock2.h>
#include <windows.h>
#endif


#include <QMainWindow>
#include <QStandardItemModel>
#include <string>

#include "mapwidget.h"
#include "importshapefiledialog.h"
#include<iostream>
#include "connectiondialog.h"

#include "importprogress.h"
#include "registergeometry.h"
using namespace mongo;
using namespace std;
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_actionConnect_to_MongoDB_triggered();

    void on_actionRefresh_layer_triggered();

    void on_treeView_clicked(const QModelIndex &index);

    void on_actionImport_shapefile_shp_triggered();

    
    void on_actionExit_triggered();

    void on_actionMove_triggered();

    void on_actionZoom_to_full_extend_triggered();

    void on_action_geoNear_triggered();

    void on_action_geoIntersects_triggered();

    void on_action_geoWithin_triggered();

private:
    Ui::MainWindow *ui;
    bool _need2InitDriverInstance=true;
    mongo::DBClientBase * _conn;
    QStandardItemModel * goodsModel;
    void refreshTreeView();

    string _currentDB="";
    string _currentColl="";
    string _currentConnectionString="";
    MapWidget *map1;







};

#endif // MAINWINDOW_H

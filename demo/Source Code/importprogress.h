#ifndef IMPORTPROGRESS_H
#define IMPORTPROGRESS_H

#include <QDialog>
#include <string>
#include <QtCore>

#if defined(_WIN32)
#include <winsock2.h>
#include <windows.h>
#endif
#include "mongo/client/dbclient.h"

using namespace std;
using namespace mongo;
namespace Ui {
class ImportProgress;
}







class ImportProgress : public QDialog
{
    Q_OBJECT

public:
    explicit ImportProgress(string connS,string dbName,string collectionName,string path,QWidget *parent = 0);
    ~ImportProgress();

private:
    Ui::ImportProgress *ui;
    DBClientBase * newconn=NULL;


    QTimer *timer;
    bool isFirstTime=true;
    string _db;
    string _coll;
    string _path;

private slots:
    void timerUpDate();
};

#endif // IMPORTPROGRESS_H

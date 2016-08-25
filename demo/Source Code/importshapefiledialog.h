#ifndef IMPORTSHAPEFILEDIALOG_H
#define IMPORTSHAPEFILEDIALOG_H
#include <string>

#include <QDialog>
using namespace std;

namespace Ui {
class ImportShapefileDialog;
}

class ImportShapefileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImportShapefileDialog(string dbname,string collectionName,QWidget *parent = 0);
    ~ImportShapefileDialog();

private:
    Ui::ImportShapefileDialog *ui;
    
public:
    string _nameOfSelectedDB="";
    string _filePath="";
    string _collName="";
private slots:
    void on_pushButton_clicked();
    void on_buttonBox_accepted();
};

#endif // IMPORTSHAPEFILEDIALOG_H

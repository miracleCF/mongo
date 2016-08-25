#include "importshapefiledialog.h"
#include "ui_importshapefiledialog.h"
#include <QFileDialog>
ImportShapefileDialog::ImportShapefileDialog(string dbname,string collectionName,QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ImportShapefileDialog)
{
    ui->setupUi(this);
    _nameOfSelectedDB=dbname;
    ui->lineEdit_2->setText(QString::fromStdString(dbname));
    ui->lineEdit_3->setText(QString::fromStdString(collectionName));
    
}

ImportShapefileDialog::~ImportShapefileDialog()
{
    delete ui;
}

void ImportShapefileDialog::on_pushButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("open file"), " ",  tr("shapefile(*.shp)"));
    ui->lineEdit->setText(fileName);

}

void ImportShapefileDialog::on_buttonBox_accepted()
{
    _filePath=ui->lineEdit->text().toStdString();
    _collName=ui->lineEdit_3->text().toStdString();
    _nameOfSelectedDB=ui->lineEdit_2->text().toStdString();
}

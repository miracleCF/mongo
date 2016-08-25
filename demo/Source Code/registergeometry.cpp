#include "registergeometry.h"
#include "ui_registergeometry.h"

registerGeometry::registerGeometry(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::registerGeometry)
{
    ui->setupUi(this);
}

registerGeometry::~registerGeometry()
{
    delete ui;
}

void registerGeometry::on_radioButton_clicked()
{
    _geoType=1;
    ui->buttonBox->setEnabled(true);
}

void registerGeometry::on_radioButton_2_clicked()
{
    _geoType=2;
    ui->buttonBox->setEnabled(true);
}

void registerGeometry::on_radioButton_3_clicked()
{
    _geoType=3;
    ui->buttonBox->setEnabled(true);
}

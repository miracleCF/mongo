#ifndef REGISTERGEOMETRY_H
#define REGISTERGEOMETRY_H

#include <QDialog>

namespace Ui {
class registerGeometry;
}

class registerGeometry : public QDialog
{
    Q_OBJECT

public:
    explicit registerGeometry(QWidget *parent = 0);
    int _geoType=-1;
    ~registerGeometry();

private slots:
    void on_radioButton_clicked();

    void on_radioButton_2_clicked();

    void on_radioButton_3_clicked();

private:
    Ui::registerGeometry *ui;


};

#endif // REGISTERGEOMETRY_H

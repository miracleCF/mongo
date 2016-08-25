#ifndef CONNECTIONDIALOG_H
#define CONNECTIONDIALOG_H

#include <QDialog>
#include <string>

using namespace std;
namespace Ui {
class ConnectionDialog;
}

class ConnectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConnectionDialog(QWidget *parent = 0);
    ~ConnectionDialog();

private slots:
    void on_buttonBox_accepted();

private:
    Ui::ConnectionDialog *ui;

public:
    string connectionString;

};

#endif // CONNECTIONDIALOG_H

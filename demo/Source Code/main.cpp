#include "mainwindow.h"
#include <QApplication>

#if defined(_WIN32)
#include <winsock2.h>
#include <windows.h>
#endif

using namespace mongo;
using namespace std;
int main(int argc, char *argv[])
{
    std::cout<<">>> here running MongoDB CXX Driver Test Program In Ubuntu 16.04"<<std::endl;
    std::cout<<">>> Trying to Init MongoDB Global instance"<<std::endl;
    mongo::client::GlobalInstance instance;
    if (!instance.initialized()) {
         std::cout << "failed to initialize the client driver: " << instance.status() << std::endl;
    }

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}

#-------------------------------------------------
#
# Project created by QtCreator 2016-05-30T08:36:31
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = mongoDB MapViewer
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    mapwidget.cpp \
    connectiondialog.cpp \
    importshapefiledialog.cpp \
    importprogress.cpp \
    snapshot.cpp \
    registergeometry.cpp

HEADERS  += mainwindow.h \
    mapwidget.h \
    connectiondialog.h \
    importshapefiledialog.h \
    importprogress.h \
    snapshot.h \
    registergeometry.h

FORMS    += mainwindow.ui \
    connectiondialog.ui \
    importshapefiledialog.ui \
    importprogress.ui \
    registergeometry.ui









win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../gdal/release-1800-x64-gdal-1-11-1-mapserver-6-4-1-libs/lib/ -lgdal_i
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../gdal/release-1800-x64-gdal-1-11-1-mapserver-6-4-1-libs/lib/ -lgdal_i
else:unix: LIBS += -L$$PWD/../../gdal/release-1800-x64-gdal-1-11-1-mapserver-6-4-1-libs/lib/ -lgdal_i

INCLUDEPATH += $$PWD/../../gdal/release-1800-x64-gdal-1-11-1-mapserver-6-4-1-libs/include
DEPENDPATH += $$PWD/../../gdal/release-1800-x64-gdal-1-11-1-mapserver-6-4-1-libs/include

win32: LIBS += -L$$PWD/../../mongo-cxx-driver-legacy3.2/build/install/lib/ -llibmongoclient-gd

INCLUDEPATH += $$PWD/../../mongo-cxx-driver-legacy3.2/build/install/include
DEPENDPATH += $$PWD/../../mongo-cxx-driver-legacy3.2/build/install/include




win32: LIBS += -LC:/local/boost_1_58_0/lib64-msvc-12.0/ -llibboost_thread-vc120-mt-gd-1_58

INCLUDEPATH += C:/local/boost_1_58_0/lib64-msvc-12.0
DEPENDPATH += C:/local/boost_1_58_0/lib64-msvc-12.0

win32:!win32-g++: PRE_TARGETDEPS += C:/local/boost_1_58_0/lib64-msvc-12.0/libboost_thread-vc120-mt-gd-1_58.lib
else:win32-g++: PRE_TARGETDEPS += C:/local/boost_1_58_0/lib64-msvc-12.0/liblibboost_thread-vc120-mt-gd-1_58.a



win32: LIBS += -LC:/local/boost_1_58_0/lib64-msvc-12.0/ -llibboost_system-vc120-mt-gd-1_58

INCLUDEPATH += C:/local/boost_1_58_0/lib64-msvc-12.0
DEPENDPATH += C:/local/boost_1_58_0/lib64-msvc-12.0

win32:!win32-g++: PRE_TARGETDEPS += C:/local/boost_1_58_0/lib64-msvc-12.0/libboost_system-vc120-mt-gd-1_58.lib
else:win32-g++: PRE_TARGETDEPS += C:/local/boost_1_58_0/lib64-msvc-12.0/liblibboost_system-vc120-mt-gd-1_58.a


win32: LIBS += -LC:/local/boost_1_58_0/lib64-msvc-12.0/ -llibboost_filesystem-vc120-mt-gd-1_58

INCLUDEPATH += C:/local/boost_1_58_0/lib64-msvc-12.0
DEPENDPATH += C:/local/boost_1_58_0/lib64-msvc-12.0

win32:!win32-g++: PRE_TARGETDEPS += C:/local/boost_1_58_0/lib64-msvc-12.0/libboost_filesystem-vc120-mt-gd-1_58.lib
else:win32-g++: PRE_TARGETDEPS += C:/local/boost_1_58_0/lib64-msvc-12.0/liblibboost_filesystem-vc120-mt-gd-1_58.a



win32: LIBS += -LC:/local/boost_1_58_0/lib64-msvc-12.0/ -lboost_regex-vc120-mt-gd-1_58

INCLUDEPATH += C:/local/boost_1_58_0/lib64-msvc-12.0
DEPENDPATH += C:/local/boost_1_58_0/lib64-msvc-12.0

win32:!win32-g++: PRE_TARGETDEPS += C:/local/boost_1_58_0/lib64-msvc-12.0/boost_regex-vc120-mt-gd-1_58.lib
else:win32-g++: PRE_TARGETDEPS += C:/local/boost_1_58_0/lib64-msvc-12.0/libboost_regex-vc120-mt-gd-1_58.a

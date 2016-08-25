#ifndef MAPWIDGET_H
#define MAPWIDGET_H
#if defined(_WIN32)
#include <winsock2.h>
#include <windows.h>
#endif
#include <QtWidgets>
#include <string>
#include "snapshot.h"

using namespace mongo;
using namespace std;



class MapWidget : public QWidget
{
        Q_OBJECT

public:
        MapWidget(QWidget *parent = 0);
        void Direct2Null();
        void Direct2NS(DBClientBase * conn,string dbName,string collectionName);
        void setCurrentTool(int toolType);




protected:
        void paintEvent(QPaintEvent *event);
        void mouseMoveEvent(QMouseEvent * e);
        void wheelEvent(QWheelEvent *e);
        void mousePressEvent(QMouseEvent *e);
        void mouseReleaseEvent(QMouseEvent *e);
        void mouseDoubleClickEvent(QMouseEvent *e);
        void resizeEvent(QResizeEvent *e);

private slots:
    void timerUpDate();

private:
        void geoNearQuery(double ctx,double cty,double r);
        void geoRangeQUery(vector<QPointF> polygon, int type);
        void parseGeometry(BSONObj &obj, string geoField);

        void DrawFrameWork(QPainter& painter);
        void DrawData(QPainter & painter);
        void DrawData2(QPainter &painter);
        void DrawData3(QPainter &painter);
        void DrawGeometry(QPainter &painter);
        void DrawQueryRange(QPainter &painter);

        DBClientBase * _conn=NULL;// for connect MongoDB
        string _dbName;
        string _collectionName;
        MBR mapMBR;
        MBR dataspaceMBR;

        vector<MBR> historyBMRs;
        int HistoryLevel=0;
        QTimer *timer;
        int timeElps=0;


        double _ratio=-1;
        double _ctx=0;
        double _cty=0;
        bool _isFirst=true;

        double _startX=0;
        double _startY=0;
        double _endX=0;
        double _endY=0;
        double _dx=0;
        double _dy=0;
        bool _isPressed=false;

        /*
         * 0: null query
         * 2: geoWithin
         * 3: geoIntersects
         * 4: geoNear
         *
         */
        int _currentQuery=0;
        vector<QPointF> _polygon;
        vector<vector<QPointF>> _queriedPolygons;
        vector<vector<QPointF>> _queriedPolylines;
        vector<QPointF> _queriedPoints;

        //for storing the query range of the GoeNear queries
        double nearR=0;
        double nearX=0;
        double nearY=0;
        bool _hasData=false;


        //for storing the query range of geoIntersects and geoWithin
        vector<QPointF> rangePolygon;
        bool _isRangeBegin=false;
        int lastX=0;
        int lastY=0;

        QPixmap pix;

        /*
         * -1: undefined
         *  1: Move and navigate
         *  2: geoWithin
         *  3: geoIntersects
         *  4: geoWithin
         */
        double currentTool=1;
        double lastTool=0;
        bool _needRedraw=true;



};




#endif // MAPWIDGET_H

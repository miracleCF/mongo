#include "mapwidget.h"
#include <QPainter>

//#include "mainwindow.h"
#include <string>
#include <iostream>


using namespace std;

MapWidget::MapWidget(QWidget *parent)
        : QWidget(parent)
{
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        this->setMouseTracking(true);


        timer=new QTimer(this);
        connect(timer,SIGNAL(timeout()),this,SLOT(timerUpDate()));//connect the timeout with SLOT


}

void MapWidget::paintEvent(QPaintEvent *event)
{
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setBrush(Qt::white);
        QSize sizeOfMapViewer=this->size();
        painter.drawRect(0,0,sizeOfMapViewer.width(),sizeOfMapViewer.height());
        DrawFrameWork(painter);
        if(_conn!=NULL)
        {
            if(_isFirst)
            {
                DrawData(painter);
                _isFirst=false;
            }
            else
            {
                if(currentTool==1)
                {
                    DrawData2(painter);
                    DrawGeometry(painter);
                    DrawQueryRange(painter);
                }
                if(currentTool==4 ||currentTool==2 ||currentTool==3)
                {
                    DrawData3(painter);
                }
            }

        }
}

void MapWidget::timerUpDate()
{
    if(timeElps==5)
    {
        this->repaint();
        timeElps=0;//clear
        //now we want to refresh the data
        timer->stop();//stop, next time will be triggered by MouseWheel event

    }
    timeElps++;
}

void MapWidget::mouseMoveEvent(QMouseEvent *e)
{
    if(_isPressed)
    {
        if(currentTool==1)
        {
            _dx=e->x()-_startX;
            _dy=e->y()-_startY;
            _dx=-_dx;
            _dy=-_dy;

            timeElps=0;
            timer->start(100);

        }
        if(currentTool==4)//near
        {
            nearR=sqrt((e->x()-_startX)*(e->x()-_startX)+(e->y()-_startY)*(e->y()-_startY))*_ratio;
        }


        this->repaint();
    }


    if(_isRangeBegin)
    {
        if(currentTool==2 ||currentTool==3)
        {
            QPointF p1;
            p1.setX(dataspaceMBR.MinX+e->x()*_ratio);
            p1.setY(dataspaceMBR.MinY+e->y()*_ratio);
            rangePolygon[rangePolygon.size()-1]=p1;
        }
        this->repaint();
    }

}

void MapWidget::DrawFrameWork(QPainter &painter)
{
    QSize sizeOfMapViewer=this->size();
    QPen pen;
    pen.setColor(QColor::fromRgb(200,200,200));
    pen.setStyle(Qt::PenStyle::DashLine);
    painter.setPen(pen);
    for(int i=0;i<=sizeOfMapViewer.width();i+=20)
    {

        painter.drawLine(i,0,i,5);
        painter.drawLine(i,sizeOfMapViewer.height(),i,sizeOfMapViewer.height()- 5);
    }
    for(int i=0;i<=sizeOfMapViewer.height();i+=20)
    {
        painter.drawLine(0,i,5,i);
        painter.drawLine(sizeOfMapViewer.width(),i,sizeOfMapViewer.width()-5,i);
    }

    for(int i=0;i<=sizeOfMapViewer.width();i+=60)
    {
        painter.drawLine(i,0,i,sizeOfMapViewer.width());
    }
    for(int i=0;i<=sizeOfMapViewer.height();i+=60)
    {
        painter.drawLine(sizeOfMapViewer.width(),i,0,i);
    }
    QRect r1(0,0,140,20);
    QRect r2(sizeOfMapViewer.width()-140,0,140,20);
    QRect r3(0,sizeOfMapViewer.height()-20,140,20);
    QRect r4(sizeOfMapViewer.width()-140,sizeOfMapViewer.height()-20,140,20);
    if(_conn==NULL)
    {
        painter.drawText(r1,"NULL");
        painter.drawText(r2,"NULL");
        painter.drawText(r3,"NULL");
        painter.drawText(r4,"NULL");
    }
    else
    {
        painter.setPen(QColor::fromRgb(0,0,0));
        stringstream ss1;
        stringstream ss2;
        stringstream ss3;
        stringstream ss4;
        ss1<<dataspaceMBR.MinX<<","<<dataspaceMBR.MinY;
        ss2<<dataspaceMBR.MaxX<<","<<dataspaceMBR.MinY;
        ss3<<dataspaceMBR.MaxX<<","<<dataspaceMBR.MaxY;
        ss4<<dataspaceMBR.MinX<<","<<dataspaceMBR.MaxY;

        painter.drawText(r1,QString::fromStdString(ss4.str()));
        painter.drawText(r2,QString::fromStdString(ss3.str()));
        painter.drawText(r3,QString::fromStdString(ss1.str()));
        painter.drawText(r4,QString::fromStdString(ss2.str()));
    }

}

void MapWidget::Direct2Null()
{
    _conn=NULL;
    _dbName="";
    _collectionName="";
    this->repaint();
}

void MapWidget::Direct2NS(DBClientBase *conn, string dbName, string collectionName)
{
    _conn=conn;
    _dbName=dbName;
    _collectionName=collectionName;
    _isFirst=true;
    this->repaint();
}

void MapWidget::DrawData(QPainter &painter)
{
    SnapShot tempSnapShot(_conn,_dbName,_collectionName);
    MBR envelope=tempSnapShot.getEnvelope();
    //Transformation things
    QSize mapSize=this->size();
    double mapRatio=mapSize.width()*1.0f/mapSize.height();
    double dataRatio= (envelope.MaxX-envelope.MinX)/(envelope.MaxY-envelope.MinY);
    double ctx=(envelope.MaxX+envelope.MinX)/2;
    double cty=(envelope.MaxY+envelope.MinY)/2;
    _ctx=ctx;
    _cty=cty;
    if(dataRatio>mapRatio) //displayed Horizontally
    {
        dataspaceMBR.MinX=envelope.MinX;
        dataspaceMBR.MaxX=envelope.MaxX;
        dataspaceMBR.MinY=cty-(mapSize.height()/2.0)*(envelope.MaxX-envelope.MinX)/mapSize.width();
        dataspaceMBR.MaxY=cty+(mapSize.height()/2.0)*(envelope.MaxX-envelope.MinX)/mapSize.width();
        _ratio=(envelope.MaxX-envelope.MinX)/mapSize.width();
    }
    else//displayed vertically
    {
        dataspaceMBR.MaxY=envelope.MaxY;
        dataspaceMBR.MinY=envelope.MinY;
        dataspaceMBR.MinX=ctx-(mapSize.width()/2.0)*(envelope.MaxY-envelope.MinY)/mapSize.height();
        dataspaceMBR.MaxX=ctx+(mapSize.width()/2.0)*(envelope.MaxY-envelope.MinY)/mapSize.height();
        _ratio=(envelope.MaxY-envelope.MinY)/mapSize.height();
    }
    //find Rects;

    historyBMRs=tempSnapShot.getSnapShot(dataspaceMBR);
    vector<MBR> mbrs2Draw=historyBMRs;
    //startDrawing
    HistoryLevel=tempSnapShot.Level2Find;
    if(tempSnapShot.Level2Find==0)
    {
        QPen penn;
        penn.setColor(QColor::fromRgb(0.5,0.5,0.5));
        penn.setStyle(Qt::PenStyle::DashLine);
        painter.setPen(penn);
        painter.setBrush(QColor::fromRgbF(0.6,0.6,0.6,0.1));
    }
    else
    {
        QPen penn;
        penn.setColor(QColor::fromRgb(0.321,0.572,0.886));
        penn.setStyle(Qt::PenStyle::DashLine);
        painter.setPen(penn);
        painter.setBrush(QColor::fromRgbF(0.321,0.572,0.886,0.1));
    }
    for(int i=0;i<mbrs2Draw.size();i++)
    {
       MBR datam=mbrs2Draw[i];
       qreal minx=(datam.MinX-dataspaceMBR.MinX)/(dataspaceMBR.MaxX-dataspaceMBR.MinX)*mapSize.width();
       qreal maxx=(datam.MaxX-dataspaceMBR.MinX)/(dataspaceMBR.MaxX-dataspaceMBR.MinX)*mapSize.width();
       qreal miny=(datam.MinY-dataspaceMBR.MinY)/(dataspaceMBR.MaxY-dataspaceMBR.MinY)*mapSize.height();
       qreal maxy=(datam.MaxY-dataspaceMBR.MinY)/(dataspaceMBR.MaxY-dataspaceMBR.MinY)*mapSize.height();
       QRectF rect(minx,miny,maxx-minx,maxy-miny);
       painter.drawRect(rect);
    }

}


void MapWidget::DrawData2(QPainter &painter)
{
    //keep the location of ctPoint and ratio
    QSize mapSize=this->size();
    //calculatingdataspaceBMRs while maintaining ctPoint unchange
    dataspaceMBR.MinX=_ctx- mapSize.width()/2*_ratio+_dx*_ratio;
    dataspaceMBR.MaxX=_ctx+ mapSize.width()/2*_ratio+_dx*_ratio;
    dataspaceMBR.MinY=_cty- mapSize.height()/2*_ratio+_dy*_ratio;
    dataspaceMBR.MaxY=_cty+ mapSize.height()/2*_ratio+_dy*_ratio;
    SnapShot tempSnapShot(_conn,_dbName,_collectionName);
    vector<MBR> mbrs2Draw;
    if(timeElps==5)// from 4++ ~5   timeElps==5
    {
        historyBMRs.clear();
        historyBMRs=tempSnapShot.getSnapShot(dataspaceMBR);
        HistoryLevel=tempSnapShot.Level2Find;
    }
    mbrs2Draw=historyBMRs;

    if(HistoryLevel==0)
    {
        QPen penn;
        penn.setColor(QColor::fromRgb(0.5,0.5,0.5));
        penn.setStyle(Qt::PenStyle::DashLine);
        painter.setPen(penn);
        painter.setBrush(QColor::fromRgbF(0.6,0.6,0.6,0.1));
    }
    else
    {
        QPen penn;
        penn.setColor(QColor::fromRgb(0.321,0.572,0.886));
        penn.setStyle(Qt::PenStyle::DashLine);
        painter.setPen(penn);
        painter.setBrush(QColor::fromRgbF(0.321,0.572,0.886,0.1));
    }

    for(int i=0;i<mbrs2Draw.size();i++)
    {
       MBR datam=mbrs2Draw[i];
       if(datam.MinX==datam.MaxX && datam.MaxY==datam.MinY)//for Point data
       {
           QPointF p;
           QPen penn;
           penn.setColor(QColor::fromRgb(0.5,0.5,0.5));
           penn.setWidthF(1.5);
           painter.setPen(penn);
           p.setX((datam.MinX-dataspaceMBR.MinX)/(dataspaceMBR.MaxX-dataspaceMBR.MinX)*mapSize.width());
           p.setY((datam.MinY-dataspaceMBR.MinY)/(dataspaceMBR.MaxY-dataspaceMBR.MinY)*mapSize.height());
           painter.drawEllipse(p,2,2);
       }
       else
       {
           qreal minx=(datam.MinX-dataspaceMBR.MinX)/(dataspaceMBR.MaxX-dataspaceMBR.MinX)*mapSize.width();
           qreal maxx=(datam.MaxX-dataspaceMBR.MinX)/(dataspaceMBR.MaxX-dataspaceMBR.MinX)*mapSize.width();
           qreal miny=(datam.MinY-dataspaceMBR.MinY)/(dataspaceMBR.MaxY-dataspaceMBR.MinY)*mapSize.height();
           qreal maxy=(datam.MaxY-dataspaceMBR.MinY)/(dataspaceMBR.MaxY-dataspaceMBR.MinY)*mapSize.height();
           QRectF rect(minx,miny,maxx-minx,maxy-miny);
           painter.drawRect(rect);
       }


    }
}

void MapWidget::wheelEvent(QWheelEvent *e)
{
    if(currentTool==1)
    {
        timeElps=0;
        timer->start(100);
        double ratio2go=0;
        if(e->delta()>0)
        {
            //zoom in ;


            ratio2go=0.9;
        }
        else
        {
            ratio2go=1/0.9;
        }
        MBR oldDataSpace;
        MBR newDataSpace;
        QSize mapSize=this->size();
        oldDataSpace.MinX=_ctx- mapSize.width()/2*_ratio;
        oldDataSpace.MaxX=_ctx+ mapSize.width()/2*_ratio;
        oldDataSpace.MinY=_cty- mapSize.height()/2*_ratio;
        oldDataSpace.MaxY=_cty+ mapSize.height()/2*_ratio;

        double oldClickX= _ctx+(e->x()- mapSize.width()/2)*_ratio;
        double oldClickY= _cty+(e->y()-mapSize.height()/2)*_ratio;
        newDataSpace.MinX= oldClickX+(oldDataSpace.MinX-oldClickX)*ratio2go;
        newDataSpace.MaxX= oldClickX+(oldDataSpace.MaxX-oldClickX)*ratio2go;
        newDataSpace.MinY= oldClickY+(oldDataSpace.MinY-oldClickY)*ratio2go;
        newDataSpace.MaxY= oldClickY+(oldDataSpace.MaxY-oldClickY)*ratio2go;
        //recalculate _ctx , _cty;
        _ctx=(newDataSpace.MinX+newDataSpace.MaxX)/2;
        _cty=(newDataSpace.MinY+newDataSpace.MaxY)/2;

        _ratio*=ratio2go;
        this->repaint();
    }

}

void MapWidget::mousePressEvent(QMouseEvent *e)
{
    _isPressed=true;
    _startX=e->x();
    _startY=e->y();
    if(currentTool==4)
    {
        nearX=dataspaceMBR.MinX+e->x()*_ratio;
        nearY=dataspaceMBR.MinY+e->y()*_ratio;
    }
    if(currentTool==2 || currentTool==3)
    {
        if(!_isRangeBegin)
        {
            _isRangeBegin=true;// start recording query polygons
            rangePolygon.clear();
            lastX=e->x();
            lastY=e->y();
            //first Point
            QPointF p1;
            p1.setX(dataspaceMBR.MinX+e->x()*_ratio);
            p1.setY(dataspaceMBR.MinY+e->y()*_ratio);
            rangePolygon.push_back(p1);
            rangePolygon.push_back(p1);// double time for mod
        }
        else
        {
            lastX=e->x();
            lastY=e->y();
            QPointF p1;
            p1.setX(dataspaceMBR.MinX+e->x()*_ratio);
            p1.setY(dataspaceMBR.MinY+e->y()*_ratio);
            rangePolygon[rangePolygon.size()-1]=p1;
            rangePolygon.push_back(p1);
        }
    }

}

void MapWidget::mouseReleaseEvent(QMouseEvent *e)
{
    _isPressed=false;
    if(currentTool==1)
    {
        _dx=e->x()-_startX;
        _dy=e->y()-_startY;
        _ctx-=_dx*_ratio;
        _cty-=_dy*_ratio;
        _dx=0;
        _dy=0;
    }
    if(currentTool>1)
    {
       lastTool=currentTool;

       if(currentTool==4) //geoNear
       {
           //like mouse move , this should change R too
           nearR=sqrt((e->x()-_startX)*(e->x()-_startX)+(e->y()-_startY)*(e->y()-_startY))*_ratio;

           _queriedPolygons.clear();
           _queriedPoints.clear();
           _queriedPolylines.clear();
           geoNearQuery(nearX,nearY,nearR);

       }

    }
    this->repaint();
}

void MapWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
    if(currentTool==2 ||currentTool==3)
    {
        //check out of e.pos()==lastPos
        if(e->x()==lastX && e->y()==lastY)
        {
            _isRangeBegin=false;//end of Querying MBRs
            _queriedPolygons.clear();
            _queriedPoints.clear();
            _queriedPolylines.clear();

            QPointF p=rangePolygon[0];
            rangePolygon.push_back(p);

            geoRangeQUery(rangePolygon,currentTool);
        }
    }
    this->repaint();
}


void MapWidget::DrawData3(QPainter &painter)
{
    QSize sizeOfMapViewer=this->size();
    if(_needRedraw)
    {

        pix=QPixmap(sizeOfMapViewer.width(),sizeOfMapViewer.height());
        QPainter base(&pix);
        base.setRenderHint(QPainter::Antialiasing, true);
        base.setBrush(Qt::white);
        base.drawRect(0,0,sizeOfMapViewer.width(),sizeOfMapViewer.height());
        DrawFrameWork(base);
        DrawData2(base);
        _needRedraw=false;
    }
    if(currentTool==4)//near
    {
        //drawCircle
        QPixmap image=pix.copy(0,0,sizeOfMapViewer.width(),sizeOfMapViewer.height());
        QPainter final(&image);
        QPen pen;
        pen.setColor(QColor::fromRgb(235,128,85));
        pen.setStyle(Qt::PenStyle::DashLine);
        final.setPen(pen);
        final.setBrush(QColor::fromRgbF(0.921,0.501,0.227,0.2));
        double fx= (nearX-dataspaceMBR.MinX)/(dataspaceMBR.MaxX-dataspaceMBR.MinX)*sizeOfMapViewer.width();
        double fy=(nearY-dataspaceMBR.MinY)/(dataspaceMBR.MaxY-dataspaceMBR.MinY)*sizeOfMapViewer.height();
        final.drawEllipse(QPointF(qreal(fx),qreal(fy)),nearR/_ratio,nearR/_ratio);
        painter.drawPixmap(0,0,image);
    }
    if(currentTool==2 || currentTool==3)
    {
        //drawCircle
        QPixmap image=pix.copy(0,0,sizeOfMapViewer.width(),sizeOfMapViewer.height());
        QPainter final(&image);
        QPen pen;
        pen.setColor(QColor::fromRgb(235,128,85));
        pen.setStyle(Qt::PenStyle::DashLine);
        final.setPen(pen);
        final.setBrush(QColor::fromRgbF(0.921,0.501,0.227,0.2));
        QVector<QPointF> transformPoints;
        for(int i=0;i<rangePolygon.size();i++)
        {
            QPointF p;
            QPointF p0=rangePolygon[i];
            p.setX( (p0.x()-dataspaceMBR.MinX)/(dataspaceMBR.MaxX-dataspaceMBR.MinX)*sizeOfMapViewer.width());
            p.setY( (p0.y()-dataspaceMBR.MinY)/(dataspaceMBR.MaxY-dataspaceMBR.MinY)*sizeOfMapViewer.height());
            transformPoints.push_back(p);
        }
        QPolygonF rangepoly2Draw(transformPoints);
        final.drawPolygon(rangepoly2Draw);
        painter.drawPixmap(0,0,image);

    }


    DrawGeometry(painter);


}

void MapWidget::setCurrentTool(int toolType)
{
    if(toolType==1)
    {
        currentTool=1;
    }
    if(toolType==4)
    {
        currentTool=4;
        _needRedraw=true;
    }
    if(toolType==3)
    {
        currentTool=3;
        _needRedraw=true;
    }
    if(toolType==2)
    {
        currentTool=2;
        _needRedraw=true;
    }
}

void MapWidget::DrawGeometry(QPainter &painter)
{
    if(_queriedPolygons.size()>0)
    {
        QPen pen;
        pen.setColor(QColor::fromRgb(235,128,85));
        pen.setWidth(2);
        painter.setPen(pen);
        painter.setBrush(QColor::fromRgbF(0.921,0.501,0.227,0.5));


        for(int i=0;i<_queriedPolygons.size();i++)
        {
            vector<QPointF> thePolygon=_queriedPolygons[i];
            QVector<QPointF> transPolygonV;
            for (int j = 0; j < thePolygon.size(); j++)
            {

                QPointF oneCoord;
                oneCoord.setX((thePolygon[j].x()-dataspaceMBR.MinX)/(dataspaceMBR.MaxX-dataspaceMBR.MinX)*this->width());
                oneCoord.setY((thePolygon[j].y()-dataspaceMBR.MinY)/(dataspaceMBR.MaxY-dataspaceMBR.MinY)*this->height());
                transPolygonV.push_back(oneCoord);
            }
            QPolygonF pPolygon=QPolygonF(transPolygonV);
            painter.drawPolygon(pPolygon);

        }

    }
    if(_queriedPolylines.size()>0)
    {
        QPen pen;
        pen.setColor(QColor::fromRgb(235,128,85));
        pen.setWidth(2);
        painter.setPen(pen);


        for(int i=0;i<_queriedPolylines.size();i++)
        {
            vector<QPointF> theLineString=_queriedPolylines[i];
            QVector<QPointF> transLineStringV;
            for (unsigned int j = 0; j < theLineString.size(); j++)
            {

                QPointF oneCoord;
                oneCoord.setX((theLineString[j].x()-dataspaceMBR.MinX)/(dataspaceMBR.MaxX-dataspaceMBR.MinX)*this->width());
                oneCoord.setY((theLineString[j].y()-dataspaceMBR.MinY)/(dataspaceMBR.MaxY-dataspaceMBR.MinY)*this->height());
                transLineStringV.push_back(oneCoord);
            }
            for(int i=0;i<transLineStringV.size()-1;i++)
            {
                painter.drawLine(transLineStringV[i],transLineStringV[i+1]);
            }


        }
    }

    if(_queriedPoints.size()>0)
    {
        QPen pen;
        pen.setColor(QColor::fromRgb(235,128,85));
        pen.setWidth(2);
        painter.setPen(pen);


        for(int i=0;i<_queriedPoints.size();i++)
        {

            QPointF oneCoord;
            oneCoord.setX((_queriedPoints[i].x()-dataspaceMBR.MinX)/(dataspaceMBR.MaxX-dataspaceMBR.MinX)*this->width());
            oneCoord.setY((_queriedPoints[i].y()-dataspaceMBR.MinY)/(dataspaceMBR.MaxY-dataspaceMBR.MinY)*this->height());

            painter.drawEllipse(oneCoord,1.5,1.5);
        }
    }


}

void MapWidget::geoNearQuery(double ctx, double cty, double r)
{
    SnapShot snp=SnapShot(_conn,_dbName,_collectionName);
    string geoFiled=snp.getGeometryField();

    BSONArrayBuilder coordinateBdr1;
    coordinateBdr1.append(ctx);
    coordinateBdr1.append(cty);

    BSONObjBuilder geoBDR;
    geoBDR.append("type", "Point");
    geoBDR.append("coordinates", coordinateBdr1.arr());

    BSONObjBuilder geocondBDR;
    geocondBDR.append("$geometry", geoBDR.obj());
    geocondBDR.append("$maxDistance", r);
    geocondBDR.append("$minDistance", 0);
    BSONObjBuilder filterBDR;
    filterBDR.append("$near", geocondBDR.obj());

    BSONObjBuilder condBDR;
    condBDR.append(geoFiled, filterBDR.obj());
    std::unique_ptr<DBClientCursor> _Current_Cursor = _conn->query(_dbName+"."+_collectionName, condBDR.obj());

    while (_Current_Cursor->more())
    {
        BSONObj oneObj = _Current_Cursor->next();
        parseGeometry(oneObj,geoFiled);
    }
}

void MapWidget::geoRangeQUery(vector<QPointF> polygon, int type)
{
    if(type!=2 && type!=3)
    {
        return;
    }
    if(polygon.size()<4)
    {
        return;
    }

    SnapShot snp=SnapShot(_conn,_dbName,_collectionName);
    string geoFiled=snp.getGeometryField();

    BSONArrayBuilder polygonBdr;
    BSONArrayBuilder ringBdr;

    for(int i=0;i<polygon.size();i++)
    {
        BSONArrayBuilder coordinateBdr1;
        QPointF pf=polygon[i];
        double x=pf.x();
        double y=pf.y();
        coordinateBdr1.append(x);
        coordinateBdr1.append(y);
        ringBdr.append(coordinateBdr1.arr());
    }
    polygonBdr.append(ringBdr.arr());
    BSONObjBuilder geoBDR;
    geoBDR.append("type", "Polygon");
    geoBDR.append("coordinates", polygonBdr.arr());
    BSONObjBuilder geocondBDR;
    geocondBDR.append("$geometry", geoBDR.obj());

    BSONObjBuilder filterBDR;
    if(type==2)
    {
        filterBDR.append("$geoWithin", geocondBDR.obj());
    }
    if(type=3)
    {
        filterBDR.append("$geoIntersects", geocondBDR.obj());
    }


    BSONObjBuilder condBDR;
    condBDR.append(geoFiled, filterBDR.obj());

    std::unique_ptr<DBClientCursor> _Current_Cursor = _conn->query(_dbName+"."+_collectionName, condBDR.obj());


    while (_Current_Cursor->more())
    {
        BSONObj oneObj = _Current_Cursor->next();
        parseGeometry(oneObj,geoFiled);
    }

}

void MapWidget::parseGeometry(BSONObj &obj, string geoField)
{
    string parsedGeoType=obj[geoField].Obj()["type"].str();
    if(parsedGeoType=="Polygon")
    {

        BSONObj coords = obj[geoField].Obj()["coordinates"].Obj();
        std::vector<BSONElement> Rings;
        coords.elems(Rings);
        BSONObj oneRing = Rings[0].Obj();
        std::vector<BSONElement> Points;
        oneRing.elems(Points);
        vector<QPointF> thePolygon;
        for (int j = 0; j < Points.size(); j++)
        {

            QPointF oneCoord;
            std::vector<BSONElement> vv;
            Points[j].Obj().elems(vv);
            oneCoord.setX(vv[0].number());
            oneCoord.setY(vv[1].number());
            thePolygon.push_back(oneCoord);
        }
        _queriedPolygons.push_back(thePolygon);
    }
    if(parsedGeoType=="LineString")
    {
        BSONObj oneRing = obj[geoField].Obj()["coordinates"].Obj();
        std::vector<BSONElement> Points;
        oneRing.elems(Points);
        vector<QPointF> theLineString;
        for (unsigned int j = 0; j < Points.size(); j++)
        {

            QPointF oneCoord;
            std::vector<BSONElement> vv;
            Points[j].Obj().elems(vv);
            oneCoord.setX(vv[0].number());
            oneCoord.setY(vv[1].number());
            theLineString.push_back(oneCoord);
        }
        _queriedPolylines.push_back(theLineString);
    }
    if(parsedGeoType=="Point")
    {
        BSONObj Points = obj[geoField].Obj()["coordinates"].Obj();
        QPointF oneCoord;
        std::vector<BSONElement> vv;
        Points.elems(vv);
        oneCoord.setX(vv[0].number());
        oneCoord.setY(vv[1].number());
        _queriedPoints.push_back(oneCoord);
    }

}

void MapWidget::DrawQueryRange(QPainter &painter)
{
    QSize sizeOfMapViewer=this->size();
    if(lastTool==4)
    {
        //drawCircle
        QPen pen;
        pen.setColor(QColor::fromRgb(235,128,85));
        pen.setStyle(Qt::PenStyle::DashLine);
        painter.setPen(pen);
        painter.setBrush(QColor::fromRgbF(0.921,0.501,0.227,0.2));
        double fx= (nearX-dataspaceMBR.MinX)/(dataspaceMBR.MaxX-dataspaceMBR.MinX)*sizeOfMapViewer.width();
        double fy=(nearY-dataspaceMBR.MinY)/(dataspaceMBR.MaxY-dataspaceMBR.MinY)*sizeOfMapViewer.height();
        painter.drawEllipse(QPointF(qreal(fx),qreal(fy)),nearR/_ratio,nearR/_ratio);
    }

    if(lastTool==2 || lastTool==3)
    {
        //drawCircle

        QPen pen;
        pen.setColor(QColor::fromRgb(235,128,85));
        pen.setStyle(Qt::PenStyle::DashLine);
        painter.setPen(pen);
        painter.setBrush(QColor::fromRgbF(0.921,0.501,0.227,0.2));
        QVector<QPointF> transformPoints;
        for(int i=0;i<rangePolygon.size();i++)
        {
            QPointF p;
            QPointF p0=rangePolygon[i];
            p.setX( (p0.x()-dataspaceMBR.MinX)/(dataspaceMBR.MaxX-dataspaceMBR.MinX)*sizeOfMapViewer.width());
            p.setY( (p0.y()-dataspaceMBR.MinY)/(dataspaceMBR.MaxY-dataspaceMBR.MinY)*sizeOfMapViewer.height());
            transformPoints.push_back(p);
        }
        QPolygonF rangepoly2Draw(transformPoints);
        painter.drawPolygon(rangepoly2Draw);


    }
}

void MapWidget::resizeEvent(QResizeEvent *e)
{
    _needRedraw=true;
}

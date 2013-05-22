#ifndef SHAPEOBJECT_H
#define SHAPEOBJECT_H

#include <QObject>

#include <TopoDS_Shape.hxx>

#include <osg/Switch>

#include "./modelviz/connector.h"

class ShapeObject : public QObject
{
    Q_OBJECT
public:
    explicit ShapeObject(QObject *parent);
    void setShape(const TopoDS_Shape &shapeIn);
    TopoDS_Shape getShape(){return shape;}
    osg::Switch* getMainSwitch(){return mainSwitch.get();}
    int getShapeHash(){return shapeHash;}
    
signals:
    
public slots:

private:
    int shapeHash;
    TopoDS_Shape shape;
    osg::ref_ptr<osg::Switch> mainSwitch;
    ModelViz::Connector connector;
};

#endif // SHAPEOBJECT_H

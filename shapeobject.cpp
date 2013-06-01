#include <iostream>

#include "globalutilities.h"
#include "nodemaskdefs.h"
#include "shapeobject.h"
#include "modelviz/graph.h"

ShapeObject::ShapeObject(QObject *parent) :
    QObject(parent)
{
    mainSwitch = new osg::Switch();
    mainSwitch->setNodeMask(NodeMask::object);
}

void ShapeObject::setShape(const TopoDS_Shape &shapeIn)
{
    shape = shapeIn;
    shapeHash = GU::getShapeHash(shape);
    mainSwitch->setUserValue(GU::hashAttributeTitle, shapeHash);

    ModelViz::BuildConnector connectBuilder(shape);
    connector = connectBuilder.getConnector();
//    connectBuilder.getConnector().outputGraphviz("graphTest");

    ModelViz::Build builder(shape);
    if (builder.go(0.25, 0.5))
    {
        mainSwitch->addChild(builder.getViz().get());
    }
//    builder.go(0.5, 1.0);
//    builder.go(1.0, 2.0);

}

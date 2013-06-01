#ifndef GLOBALUTILITIES_H
#define GLOBALUTILITIES_H

#include <TopoDS_Shape.hxx>

namespace osg
{
class Geometry;
class Node;
}

namespace GU
{
int getShapeHash(const TopoDS_Shape &shape);
int getHash(const osg::Geometry *geometry);
int getHash(const osg::Node *node);
static const std::string hashAttributeTitle = "ShapeHash";

}

#endif // GLOBALUTILITIES_H

#ifndef GLOBALUTILITIES_H
#define GLOBALUTILITIES_H

#include <boost/uuid/uuid.hpp>

#include <TopoDS_Shape.hxx>

namespace osg
{
class Geometry;
class Node;
}

namespace GU
{
int getShapeHash(const TopoDS_Shape &shape);
boost::uuids::uuid getId(const osg::Geometry *geometry);
boost::uuids::uuid getId(const osg::Node *node);
std::string idToString(const boost::uuids::uuid &);
boost::uuids::uuid stringToId(const std::string &);
static const std::string idAttributeTitle = "id";

}

#endif // GLOBALUTILITIES_H

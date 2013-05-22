#ifndef GLOBALUTILITIES_H
#define GLOBALUTILITIES_H

#include <TopoDS_Shape.hxx>

namespace GU
{
int getShapeHash(const TopoDS_Shape &shape);
static std::string hashAttributeTitle = "ShapeHash";

}

#endif // GLOBALUTILITIES_H

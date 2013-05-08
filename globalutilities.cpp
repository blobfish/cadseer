#include <limits>
#include "globalutilities.h"

int GU::getShapeHash(const TopoDS_Shape &shape)
{
    int hashOut;
    hashOut = shape.HashCode(std::numeric_limits<int>::max());
    return hashOut;
}

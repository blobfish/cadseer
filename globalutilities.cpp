#include <limits>
#include <assert.h>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/ValueObject>
#include "globalutilities.h"

int GU::getShapeHash(const TopoDS_Shape &shape)
{
    int hashOut;
    hashOut = shape.HashCode(std::numeric_limits<int>::max());
    return hashOut;
}

int GU::getHash(const osg::Geometry *geometry)
{
    return getHash(geometry->getParent(0));
}

int GU::getHash(const osg::Node *node)
{
    int selectedHash;
    if (!node->getUserValue(GU::hashAttributeTitle, selectedHash))
        assert(0);
    return selectedHash;
}

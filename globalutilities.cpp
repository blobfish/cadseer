#include <limits>
#include <assert.h>
#include <vector>

#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/ValueObject>

#include "globalutilities.h"

int GU::getShapeHash(const TopoDS_Shape &shape)
{
//     assert(!shape.IsNull()); need to get shape hash of null.
    int hashOut;
    hashOut = shape.HashCode(std::numeric_limits<int>::max());
    return hashOut;
}

boost::uuids::uuid GU::getId(const osg::Geometry *geometry)
{
    return getId(geometry->getParent(0));
}

boost::uuids::uuid GU::getId(const osg::Node *node)
{
  std::string stringId;
  if (!node->getUserValue(GU::idAttributeTitle, stringId))
      assert(0);
  return stringToId(stringId);
}

std::string GU::idToString(const boost::uuids::uuid &idIn)
{
  return boost::lexical_cast<std::string>(idIn);
}

boost::uuids::uuid GU::stringToId(const std::string &stringIn)
{
  return boost::lexical_cast<boost::uuids::uuid>(stringIn);
}

std::string GU::getShapeTypeString(const TopoDS_Shape &shapeIn)
{
  static const std::vector<std::string> strings = 
  {
    "Compound",
    "CompSolid",
    "Solid",
    "Shell",
    "Face",
    "Wire",
    "Edge",
    "Vertex",
    "Shape"
  };
  
  std::size_t index = static_cast<std::size_t>(shapeIn.ShapeType());
  assert(index < strings.size());
  return strings.at(index);
};

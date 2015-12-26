/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2015  Thomas S. Anderson blobfish.at.gmx.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <limits>
#include <assert.h>
#include <vector>

#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/string_generator.hpp>

#include <TopoDS_Shape.hxx>
#include <gp_Ax2.hxx>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/ValueObject>

#include <globalutilities.h>

int gu::getShapeHash(const TopoDS_Shape &shape)
{
//     assert(!shape.IsNull()); need to get shape hash of null.
    int hashOut;
    hashOut = shape.HashCode(std::numeric_limits<int>::max());
    return hashOut;
}

boost::uuids::uuid gu::getId(const osg::Geometry *geometry)
{
    return getId(geometry->getParent(0));
}

boost::uuids::uuid gu::getId(const osg::Node *node)
{
  std::string stringId;
  if (!node->getUserValue(gu::idAttributeTitle, stringId))
      assert(0);
  return boost::uuids::string_generator()(stringId);
}

std::string gu::getShapeTypeString(const TopoDS_Shape &shapeIn)
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

osg::Vec3d gu::toOsg(const gp_Vec &occVecIn)
{
  osg::Vec3d out;
  
  out.x() = occVecIn.X();
  out.y() = occVecIn.Y();
  out.z() = occVecIn.Z();
  
  return out;
}

osg::Vec3d gu::toOsg(const gp_Pnt &occPointIn)
{
  osg::Vec3d out;
  
  out.x() = occPointIn.X();
  out.y() = occPointIn.Y();
  out.z() = occPointIn.Z();
  
  return out;
}

osg::Matrixd gu::toOsg(const gp_Ax2 &systemIn)
{
  
  osg::Vec3d xVector = toOsg(gp_Vec(systemIn.XDirection()));
  osg::Vec3d yVector = toOsg(gp_Vec(systemIn.YDirection()));
  osg::Vec3d zVector = toOsg(gp_Vec(systemIn.Direction()));
  osg::Vec3d origin = toOsg(systemIn.Location());
  
  //row major for openscenegraph.
  osg::Matrixd out
  (
    xVector.x(), xVector.y(), xVector.z(), 0.0,
    yVector.x(), yVector.y(), yVector.z(), 0.0,
    zVector.x(), zVector.y(), zVector.z(), 0.0,
    origin.x(), origin.y(), origin.z(), 1.0
  );
  
  return out;
}

gp_Ax2 gu::toOcc(const osg::Matrixd &m)
{
  gp_Ax2 out
  (
    gp_Pnt(m(3, 0), m(3, 1), m(3, 2)), //origin
    gp_Dir(m(2, 0), m(2, 1), m(2, 2)), //z vector
    gp_Dir(m(0, 0), m(0, 1), m(0, 2))  //x vector
  );
  
  return out;
}

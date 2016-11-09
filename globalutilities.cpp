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
#include <TopoDS.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Ax2.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRep_Tool.hxx>

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

osg::Vec3d gu::toOsg(const TopoDS_Vertex &vertexIn)
{
  return gu::toOsg(BRep_Tool::Pnt(vertexIn));
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

osg::Matrixd gu::toOsg(const gp_Trsf &t)
{
  osg::Matrixd out
  (
    t.Value(1, 1), t.Value(2, 1), t.Value(3, 1), 0.0,
    t.Value(1, 2), t.Value(2, 2), t.Value(3, 2), 0.0,
    t.Value(1, 3), t.Value(2, 3), t.Value(3, 3), 0.0,
    t.Value(1, 4), t.Value(2, 4), t.Value(3, 4), 1.0
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

gp_Vec gu::toOcc(const osg::Vec3d& v)
{
  return gp_Vec
  (
    v.x(),
    v.y(),
    v.z()
  );
}


osg::Vec3d gu::getXVector(const osg::Matrixd& m)
{
  osg::Vec3d out;
  out.x() = m(0,0);
  out.y() = m(0,1);
  out.z() = m(0,2);
  
  return out;
}

osg::Vec3d gu::getYVector(const osg::Matrixd& m)
{
  osg::Vec3d out;
  out.x() = m(1,0);
  out.y() = m(1,1);
  out.z() = m(1,2);
  
  return out;
}

osg::Vec3d gu::getZVector(const osg::Matrixd& m)
{
  osg::Vec3d out;
  out.x() = m(2,0);
  out.y() = m(2,1);
  out.z() = m(2,2);
  
  return out;
}

osg::Vec3d gu::gleanVector(const TopoDS_Shape& shapeIn, const osg::Vec3d &pickPoint)
{
  osg::Vec3d out;
  //nan signals couldn't find a vector.
  out.x() = std::numeric_limits<double>::quiet_NaN();
  out.y() = std::numeric_limits<double>::quiet_NaN();
  out.z() = std::numeric_limits<double>::quiet_NaN();
  
  if (shapeIn.ShapeType() == TopAbs_EDGE)
  {
    BRepAdaptor_Curve cAdaptor(TopoDS::Edge(shapeIn));
    if (cAdaptor.GetType() == GeomAbs_Line)
    {
      osg::Vec3d firstPoint = toOsg(cAdaptor.Value(cAdaptor.FirstParameter()));
      osg::Vec3d lastPoint = toOsg(cAdaptor.Value(cAdaptor.LastParameter()));
      double firstDistance = (pickPoint - firstPoint).length2();
      double secondDistance = (pickPoint - lastPoint).length2();
      if (firstDistance < secondDistance)
      {
	out = firstPoint - lastPoint;
	out.normalize();
      }
      else
      {
	out = lastPoint - firstPoint;
	out.normalize();
      }
    }
  }
  else if (shapeIn.ShapeType() == TopAbs_FACE)
  {
  }
  
  return out;
}

std::ostream& operator<<(std::ostream &st, const TopoDS_Shape &sh)
{
  st << "Shape hash: " << gu::getShapeHash(sh)
    << "    Shape type: " << gu::getShapeTypeString(sh);
  return st;
}

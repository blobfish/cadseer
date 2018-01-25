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

#include <TopoDS_Shape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Ax2.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRep_Tool.hxx>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/ValueObject>

#include <tools/idtools.h>
#include <globalutilities.h>

boost::uuids::uuid gu::getId(const osg::Geometry *geometry)
{
    return getId(geometry->getParent(0));
}

boost::uuids::uuid gu::getId(const osg::Node *node)
{
  std::string stringId;
  if (!node->getUserValue<std::string>(gu::idAttributeTitle, stringId))
      assert(0);
  return gu::stringToId(stringId);
}

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

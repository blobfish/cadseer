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

#ifndef GLOBALUTILITIES_H
#define GLOBALUTILITIES_H

#include <string>

#include <boost/uuid/uuid.hpp>

class TopoDS_Shape;
class gp_Vec;
class gp_Pnt;
class gp_Ax2;

namespace osg
{
class Geometry;
class Node;
class Vec3d;
class Matrixd;
}

namespace gu
{
int getShapeHash(const TopoDS_Shape &shape);
boost::uuids::uuid getId(const osg::Geometry *geometry);
boost::uuids::uuid getId(const osg::Node *node);
static const std::string idAttributeTitle = "id";
std::string getShapeTypeString(const TopoDS_Shape&);
osg::Vec3d toOsg(const gp_Vec &occVecIn);
osg::Vec3d toOsg(const gp_Pnt &occPointIn);
osg::Matrixd toOsg(const gp_Ax2 &systemIn);
gp_Ax2 toOcc(const osg::Matrixd &m);
osg::Vec3d getXVector(const osg::Matrixd &m);
osg::Vec3d getYVector(const osg::Matrixd &m);
osg::Vec3d getZVector(const osg::Matrixd &m);
osg::Vec3d gleanVector(const TopoDS_Shape &shapeIn, const osg::Vec3d &pickPoint);
}

#endif // GLOBALUTILITIES_H

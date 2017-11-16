/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef SQS_TOOLS_H
#define SQS_TOOLS_H

#include <osg/Vec3d>
#include <osg/io_utils>

#include "mesh.h"

namespace sqs
{
  Vector getFaceNormal(const Mesh&, const Face&);
  Vector normalized(const Vector&);
  Point getMidPoint(const Point&, const Point&);
  Point getMidPoint(const std::vector<Point>&);
  osg::Vec3d getMidPoint(const std::vector<osg::Vec3d>&);
  osg::Vec3d getAverageNormal(const Mesh&, const Faces&);
  
  inline osg::Vec3d toOsg(const Point &pointIn)
  {
    return osg::Vec3d(pointIn.x(), pointIn.y(), pointIn.z());
  }
  
  inline osg::Vec3d toOsg(const Vector &vecIn)
  {
    return osg::Vec3d(vecIn.x(), vecIn.y(), vecIn.z());
  }
  
  inline Point toCgal(const osg::Vec3d &vecIn)
  {
    return Point(vecIn.x(), vecIn.y(), vecIn.z());
  }
  
  inline Point getBSphereCenter(BSphere &sIn)
  {
    const auto it = sIn.center_cartesian_begin();
    return Point(*it, *(it + 1), *(it + 2));
  }
  
  inline osg::BoundingSphered toOsg(BSphere &sIn) //sphere can't be const?
  {
    osg::Vec3d center(sqs::toOsg(getBSphereCenter(sIn)));
    double radius = sIn.radius();
    
    return osg::BoundingSphered(center, radius);
  }
}

#endif // SQS_TOOLS_H

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

#include "tools.h"

Vector sqs::getFaceNormal(const Mesh &meshIn, const Face &faceIn)
{
  HalfEdge halfEdge = meshIn.halfedge(faceIn);
  Point p1 = meshIn.point(meshIn.source(halfEdge));
  halfEdge = meshIn.next(halfEdge);
  Point p2 = meshIn.point(meshIn.source(halfEdge));
  halfEdge = meshIn.next(halfEdge);
  Point p3 = meshIn.point(meshIn.source(halfEdge));
  
  return CGAL::unit_normal(p1, p2, p3);
}

//I am missing something in cgal. this seems silly
Vector sqs::normalized(const Vector &vectorIn)
{
  double mag = 1.0 / std::sqrt(vectorIn.squared_length());
  Vector out;
  out = vectorIn * mag;
  return out;
}

Point sqs::getMidPoint(const Point &point1, const Point &point2)
{
  Vector projection = point2 - point1;
  double mag = 1.0 / std::sqrt(projection.squared_length()) / 2.0;
  projection = normalized(projection);
  projection = projection * mag;
  return point1 + projection;
}

Point sqs::getMidPoint(const std::vector<Point> &pointsIn)
{
  osg::Vec3d accum(0.0,0.0,0.0);
  for (const auto &point : pointsIn)
    accum += osg::Vec3d(point.x(), point.y(), point.z());
  accum /= pointsIn.size();
  return Point(accum.x(), accum.y(), accum.z());
}

osg::Vec3d sqs::getMidPoint(const std::vector<osg::Vec3d> &psIn)
{
  osg::Vec3d accum(0.0,0.0,0.0);
  for (const auto &p : psIn)
    accum += p;
  accum /= psIn.size();
  return accum;
}

osg::Vec3d sqs::getAverageNormal(const Mesh &mIn, const Faces &fsI)
{
  osg::Vec3d out(0.0, 0.0, 0.0);
  for (const auto &face : fsI)
  {
    if (face == mIn.null_face())
      continue;
    out = out + sqs::toOsg(getFaceNormal(mIn, face));
  }
  out.normalize();
  return out;
}

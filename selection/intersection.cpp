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

#include <selection/intersection.h>

using namespace slc;

Intersection::Intersection(){}

Intersection::~Intersection(){}

Intersection::Intersection(const osgUtil::LineSegmentIntersector::Intersection &in)
{
  nodePath = in.nodePath;
  drawable = in.drawable;
  matrix = in.matrix;
  localIntersectionPoint = in.localIntersectionPoint;
  primitiveIndex = in.primitiveIndex;
}

Intersection::Intersection(const osgUtil::PolytopeIntersector::Intersection &in)
{
  nodePath = in.nodePath;
  drawable = in.drawable;
  matrix = in.matrix;
  localIntersectionPoint = in.localIntersectionPoint;
  primitiveIndex = in.primitiveIndex;
}

Intersections& slc::append(Intersections &target, const osgUtil::LineSegmentIntersector::Intersections &in)
{
  for (const auto &intersect : in)
    target.push_back(Intersection(intersect));
  return target;
}

Intersections& slc::append(Intersections &target, const osgUtil::PolytopeIntersector::Intersections &in)
{
  for (const auto &intersect : in)
    target.push_back(Intersection(intersect));
  return target;
}

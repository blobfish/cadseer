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

#include <assert.h>
#include <algorithm>

#include <osg/Geometry>
#include <osgUtil/SmoothingVisitor>

#include "circlebuilder.h"
#include "conebuilder.h"

using namespace lbr;

void ConeBuilder::setIsoLines(std::size_t isoLinesIn)
{
  isoLines = std::max(static_cast<std::size_t>(3), isoLinesIn);
}

void ConeBuilder::setDeviation(double deviationIn)
{
  assert(deviationIn > 0.0);
  
  std::size_t tempSegments = static_cast<std::size_t>
    (std::ceil(360.0 / (2 * std::acos((radius - deviationIn) / radius))));
  isoLines = std::max(static_cast<std::size_t>(3), tempSegments);
}

void ConeBuilder::setRadius(double radiusIn)
{
  assert(radiusIn > 0.0);
  radius = radiusIn;
}

void ConeBuilder::setHeight(double heightIn)
{
  assert(heightIn > 0.0);
  height = heightIn;
}

ConeBuilder::operator osg::Geometry* () const
{
  CircleBuilder basePointBuilder;
  basePointBuilder.setRadius(radius);
  basePointBuilder.setSegments(isoLines);
  basePointBuilder.setAngularSpanDegrees(360.0);
  std::vector<osg::Vec3d> basePoints = basePointBuilder;
  
  osg::Vec3Array *points = new osg::Vec3Array();
  points->push_back(osg::Vec3d(0.0, 0.0, 0.0)); //center point for base fan
  points->push_back(basePoints.front()); //connect back to start.
  std::copy(basePoints.rbegin(), basePoints.rend(), std::back_inserter(*points));
  std::size_t perimeterStart = points->size();
  points->push_back(osg::Vec3d(0.0, 0.0, height));
  std::copy(basePoints.begin(), basePoints.end(), std::back_inserter(*points));
  points->push_back(basePoints.front()); //connect back to start.
  
  osg::ref_ptr<osg::Geometry> out = new osg::Geometry();
  out->setVertexArray(points);
  out->setUseDisplayList(false);
  out->setUseVertexBufferObjects(true);

  out->setNormalArray(new osg::Vec3Array(points->size()));
  out->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
  
  out->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, 0, perimeterStart));
  out->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, perimeterStart, points->size() - perimeterStart));
  osgUtil::SmoothingVisitor::smooth(*out);
  
  return out.release();
}

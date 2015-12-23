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

#include <algorithm>
#include <assert.h>

#include <osg/Vec3d>
#include <osg/Quat>
#include <osg/Geometry>
#include <osg/Point>

#include "circlebuilder.h"

using namespace lbr;

void CircleBuilder::setRadius(double radiusIn)
{
  radius = std::max(0.01, radiusIn);
}

void CircleBuilder::setSegments(std::size_t segmentsIn)
{
  segments = std::max(static_cast<std::size_t>(3), segmentsIn);
}

void CircleBuilder::setAngularSpanDegrees(double degreesIn)
{
  assert(degreesIn > 0.0);
  assert(degreesIn <= 360.0);
  angularSpan = osg::DegreesToRadians(degreesIn);
}

void CircleBuilder::setAngularSpanRadians(double degreesIn)
{
  assert(degreesIn > 0.0);
  assert(degreesIn <= 2 * osg::PI);
  angularSpan = degreesIn;
}

void CircleBuilder::setDeviation(double deviationIn)
{
  assert(deviationIn > 0.0);
  
  std::size_t tempSegments = static_cast<std::size_t>
    (std::ceil(angularSpan / (2 * std::acos((radius - deviationIn) / radius))));
  segments = std::max(static_cast<std::size_t>(3), tempSegments);
}

bool CircleBuilder::isCompleteCircle() const
{
  return osg::equivalent(angularSpan, 2.0 * osg::PI);
}

CircleBuilder::operator std::vector<osg::Vec3d> () const
{
  std::vector<osg::Vec3d> out;
  
  osg::Quat rotation(angularSpan / static_cast<double>(segments), osg::Vec3d(0.0, 0.0, 1.0));
  osg::Vec3d startingPoint(radius, 0.0, 0.0);
  out.push_back(startingPoint);
  osg::Vec3d lastPoint = startingPoint;
  std::size_t loopCount = segments;
  if (isCompleteCircle()) //don't duplicate first and last point on full circle.
    loopCount = loopCount - 1;
  for (std::size_t index = 0; index < loopCount; ++index)
  {
    osg::Vec3d nextPoint = rotation * lastPoint;
    out.push_back(nextPoint);
    lastPoint = nextPoint;
  }
  
  return out;
}

CircleBuilder::operator osg::Geometry* () const
{
  std::vector<osg::Vec3d> allPoints = *this;
  
  osg::ref_ptr<osg::Geometry> out = new osg::Geometry();
  out->setUseDisplayList(false);
  out->setUseVertexBufferObjects(true);
  osg::Vec3Array *array = new osg::Vec3Array();
  std::copy(allPoints.begin(), allPoints.end(), std::back_inserter(*array));
  out->setVertexArray(array);
  out->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, array->size()));
  out->getOrCreateStateSet()->setAttribute(new osg::Point(5.0));
  
  return out.release();
}
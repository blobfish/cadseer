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
#include "cylinderbuilder.h"

using namespace lbr;

void CylinderBuilder::setIsoLines(std::size_t isoLinesIn)
{
  isoLines = std::max(static_cast<std::size_t>(3), isoLinesIn);
}

void CylinderBuilder::setAngularSpanDegrees(double angularSpanIn)
{
  assert(angularSpanIn > 0.0);
  assert(angularSpanIn <= 360.0);
  angularSpan = osg::DegreesToRadians(angularSpanIn);
}

void CylinderBuilder::setAngularSpanRadians(double angularSpanIn)
{
  assert(angularSpanIn > 0.0);
  assert(angularSpanIn <= 2 * osg::PI);
  angularSpan = angularSpanIn;
}

void CylinderBuilder::setRadius(double radiusIn)
{
  assert(radiusIn > 0.0);
  radius = radiusIn;
}

void CylinderBuilder::setHeight(double heightIn)
{
  assert(heightIn > 0.0);
  height = heightIn;
}

void CylinderBuilder::setDeviation(double deviationIn)
{
  assert(deviationIn > 0.0);
  
  std::size_t tempSegments = static_cast<std::size_t>
    (std::ceil(angularSpan / (2 * std::acos((radius - deviationIn) / radius))));
  isoLines = std::max(static_cast<std::size_t>(3), tempSegments);
}

CylinderBuilder::operator osg::Geometry* () const
{
  CircleBuilder basePointBuilder;
  basePointBuilder.setRadius(radius);
  basePointBuilder.setSegments(isoLines);
  basePointBuilder.setAngularSpanRadians(angularSpan);
  std::vector<osg::Vec3d> basePoints = basePointBuilder;
  
  osg::Vec3Array *points = new osg::Vec3Array();
  osg::ref_ptr<osg::Geometry> out = new osg::Geometry();
  out->setVertexArray(points);
  out->setUseDisplayList(false);
  out->setUseVertexBufferObjects(true);
  
  osg::Matrixd transform = osg::Matrixd::translate(osg::Vec3d(0.0, 0.0, height));
  
  for (const auto &currentPoint : basePoints)
  {
    points->push_back(currentPoint * transform);
    points->push_back(currentPoint);
  }
  
  bool isFullCircle = osg::equivalent(angularSpan, 2.0 * osg::PI);
  std::size_t instanceCalc = points->size();
  if (isFullCircle)
    instanceCalc += 2;
  osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt
    (osg::PrimitiveSet::QUAD_STRIP, instanceCalc);
  std::size_t indicesIndex = 0;
  for (std::size_t loopIndex = 0; loopIndex < points->size(); ++loopIndex) 
  {
    (*indices)[indicesIndex] = indicesIndex;
    indicesIndex++;
  }
  if (isFullCircle)
  {
    (*indices)[indicesIndex] = 0;
    indicesIndex++;
    (*indices)[indicesIndex] = 1;
    indicesIndex++;
  }
  
  out->addPrimitiveSet(indices);
  osgUtil::SmoothingVisitor::smooth(*out);
  
  return out.release();
}

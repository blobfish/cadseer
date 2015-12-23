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

#include <osg/Geometry>
#include <osgUtil/SmoothingVisitor>

#include "circlebuilder.h"

#include "torusbuilder.h"

using namespace lbr;

static osg::Matrixd makeTransform(const osg::Vec3d &xVector, const osg::Vec3d &yVector, const osg::Vec3d &zVector)
{
  //row major.
  osg::Matrixd out
  (
    xVector.x(), xVector.y(), xVector.z(), 0.0,
    yVector.x(), yVector.y(), yVector.z(), 0.0,
    zVector.x(), zVector.y(), zVector.z(), 0.0,
    0.0, 0.0, 0.0, 1.0
  );
  
  return out;
}

void TorusBuilder::setMajorRadius(double radiusIn)
{
  majorRadius = std::max(0.01, radiusIn);
}

void TorusBuilder::setMajorIsoLines(std::size_t isoLinesIn)
{
  majorIsoLines = std::max(static_cast<std::size_t>(3), isoLinesIn);
}

void TorusBuilder::setMinorRadius(double radiusIn)
{
  minorRadius = std::max(0.001, radiusIn);
}

void TorusBuilder::setMinorIsoLines(std::size_t isoLinesIn)
{
  minorIsoLines = std::max(static_cast<std::size_t>(3), isoLinesIn);
}

void TorusBuilder::setAngularSpanDegrees(double angleIn)
{
  assert(angleIn > 0.0);
  assert(angleIn <= 360.0);
  
  angularSpan = osg::DegreesToRadians(angleIn);
}

void TorusBuilder::setAngularSpanRadians(double angleIn)
{
  assert(angleIn > 0.0);
  assert(angleIn <= 2 * osg::PI);
  
  angularSpan = angleIn;
}

void TorusBuilder::setDeviation(double deviationIn)
{
  std::size_t tempMajorIsoLines = static_cast<std::size_t>
    (std::ceil(calculateAngle(majorRadius, deviationIn)));
  majorIsoLines = std::max(static_cast<std::size_t>(3), tempMajorIsoLines);
  
  std::size_t tempMinorIsoLines = static_cast<std::size_t>
    (std::ceil(calculateAngle(minorRadius, deviationIn)));
  minorIsoLines = std::max(static_cast<std::size_t>(3), tempMinorIsoLines);
}

double TorusBuilder::calculateAngle(double radiusIn, double deviationIn) const
{
  assert(radiusIn > 0.0);
  assert(deviationIn > 0.0);
  
  return 2 * std::acos((radiusIn - deviationIn) / radiusIn);
}

TorusBuilder::operator osg::Geometry* () const
{
  CircleBuilder majorPointBuilder;
  majorPointBuilder.setRadius(majorRadius);
  majorPointBuilder.setSegments(majorIsoLines);
  majorPointBuilder.setAngularSpanRadians(angularSpan);
  std::vector<osg::Vec3d> majorPoints = majorPointBuilder;
  
  CircleBuilder minorPointBuilder;
  minorPointBuilder.setRadius(minorRadius);
  minorPointBuilder.setSegments(minorIsoLines);
  minorPointBuilder.setAngularSpanDegrees(360.0);
  std::vector<osg::Vec3d> innerTemplatePoints = minorPointBuilder;
  
  osg::Vec3Array *minorPoints = new osg::Vec3Array();
  osg::ref_ptr<osg::Geometry> out = new osg::Geometry();
  out->setVertexArray(minorPoints);
  out->setUseDisplayList(false);
  out->setUseVertexBufferObjects(true);
  
  osg::Vec3d zAxis(0.0, 0.0, 1.0);
  for (const auto &currentPoint : majorPoints)
  {
    osg::Vec3d xAxis = currentPoint;
    xAxis.normalize();
    osg::Vec3d yAxis = xAxis ^ zAxis;
    osg::Matrixd transform = makeTransform(xAxis, zAxis, yAxis);
    transform.setTrans(currentPoint);
    
    for (const auto &innerPoint : innerTemplatePoints)
      minorPoints->push_back(innerPoint * transform);
  }
  
  bool isFullCircle = osg::equivalent(angularSpan, 2.0 * osg::PI);
  std::size_t fullCircleCheat = isFullCircle ? 0 : 1;
  std::size_t stride = minorIsoLines;
  std::size_t instanceCalc = 2 * (innerTemplatePoints.size() + 1) * (majorPoints.size() - fullCircleCheat);
  osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt
    (osg::PrimitiveSet::QUAD_STRIP, instanceCalc);
  std::size_t indicesIndex = 0;
  for (std::size_t outerIndex = 0; outerIndex < (majorPoints.size() - fullCircleCheat); ++outerIndex) 
  {
    //when we have a full circle we need to connect the last section with the first.
    //hack lets us trick the loops for this condition.
    std::size_t hack = outerIndex * stride + stride;
    if (isFullCircle && (outerIndex == (majorPoints.size() - 1)))
      hack = 0;
    for (std::size_t innerIndex = 0; innerIndex < innerTemplatePoints.size(); ++innerIndex)
    {
      (*indices)[indicesIndex] = outerIndex * stride + innerIndex;
      indicesIndex++;
      (*indices)[indicesIndex] = hack + innerIndex;
      indicesIndex++;
    }
    (*indices)[indicesIndex] = outerIndex * stride;
    indicesIndex++;
    (*indices)[indicesIndex] = hack;
    indicesIndex++;
  }
  
  out->addPrimitiveSet(indices);
  osgUtil::SmoothingVisitor::smooth(*out);
  
  return out.release();
}


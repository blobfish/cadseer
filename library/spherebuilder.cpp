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

#include <algorithm> //for std::max
#include <assert.h>

#include <osg/Geometry>
#include <osgUtil/SmoothingVisitor>

#include "spherebuilder.h"

using namespace lbr;

void SphereBuilder::setRadius(double radiusIn)
{
  radius = std::max(0.01, radiusIn);
}

void SphereBuilder::setIsoLines(std::size_t isoLinesIn)
{
  isoLines = std::max(static_cast<std::size_t>(4), isoLinesIn);
}

void SphereBuilder::setDeviation(double deviationIn)
{
  assert(deviationIn > 0.0);
  std::size_t tempIsoLines = static_cast<std::size_t>
    (std::ceil(osg::PI / std::acos((radius - deviationIn) / radius)));
  isoLines = std::max(static_cast<std::size_t>(4), tempIsoLines);
}

SphereBuilder::operator osg::Geometry* () const
{
  std::vector<osg::Vec3d> allPoints;
  
  osg::Vec3d templateStartPoint(0.0, -radius, 0.0);
  std::vector<osg::Vec3d> templatePoints;
  osg::Quat templateRotation(osg::PI / (isoLines - 1), osg::Vec3d(0.0, 0.0, 1.0));
  osg::Vec3d currentPoint = templateStartPoint;
  templatePoints.push_back(currentPoint);
  for (std::size_t index = 0; index < (isoLines - 1); ++index)
  {
    currentPoint = templateRotation * currentPoint;
    templatePoints.push_back(currentPoint);
  }
  
  allPoints.push_back(templatePoints.front());
  osg::Quat rotation(2.0 * osg::PI / isoLines, osg::Vec3d(0.0, -1.0, 0.0));
  //notice we are skipping the first entry and the last.
  for (std::size_t index1 = 1; index1 < (templatePoints.size() - 1); ++index1)
  {
    currentPoint = templatePoints.at(index1);
    allPoints.push_back(currentPoint);
    for (std::size_t index2 = 0; index2 < (isoLines - 1); ++index2) //first and last NOT a duplicate
    {
      currentPoint = rotation * currentPoint;
      allPoints.push_back(currentPoint);
    }
  }
  allPoints.push_back(templatePoints.back());
  
  osg::ref_ptr<osg::Geometry> out = new osg::Geometry();
  out->setUseDisplayList(false);
  out->setUseVertexBufferObjects(true);
  osg::Vec3Array *array = new osg::Vec3Array();
  std::copy(allPoints.begin(), allPoints.end(), std::back_inserter(*array));
  out->setVertexArray(array);
  
  //bottom
  osg::ref_ptr<osg::DrawElementsUInt> fan1 = new osg::DrawElementsUInt
    (osg::PrimitiveSet::TRIANGLE_FAN, isoLines + 2);
  std::size_t indicesIndex = 0;
  for (std::size_t innerIndex = 0; innerIndex < isoLines + 1; ++innerIndex)
  {
    (*fan1)[indicesIndex] = indicesIndex;
    indicesIndex++;
  }
  (*fan1)[indicesIndex] = 1;
  out->addPrimitiveSet(fan1.get());
  
  //top
  //something is not right with the output sphere. this top
  //segment renders different in different polygon modes.
  //can't find the problem. I have manually created 2 triangles
  //instead of the following, but it rendered the same.
  osg::ref_ptr<osg::DrawElementsUInt> fan2 = new osg::DrawElementsUInt
    (osg::PrimitiveSet::TRIANGLE_FAN, isoLines + 2);
  indicesIndex = 0;
  for (std::vector<osg::Vec3d>::reverse_iterator it = allPoints.rbegin(); it != allPoints.rbegin() + isoLines + 1; ++it)
  {
    (*fan2)[indicesIndex] = std::distance(allPoints.begin(), it.base()) - 1;
    ++indicesIndex;
  }
  (*fan2)[indicesIndex] = allPoints.size() - 2;
  out->addPrimitiveSet(fan2.get());
  
  //center
  //the +1 skips the first isoline, which is degenerate to vertex.
  std::size_t stride = isoLines;
  for (std::size_t outerIndex = 0; outerIndex < (isoLines - 3); ++outerIndex)
  {
    indicesIndex = 0;
    osg::ref_ptr<osg::DrawElementsUInt> tris = new osg::DrawElementsUInt
      (osg::PrimitiveSet::TRIANGLE_STRIP, (isoLines + 1) * 2);
    for (std::size_t innerIndex = 0; innerIndex < isoLines; ++innerIndex)
    {
      (*tris)[indicesIndex] = outerIndex * stride + innerIndex + 1;
      indicesIndex++;
      
      (*tris)[indicesIndex] = outerIndex * stride + stride + innerIndex + 1;
      indicesIndex++;
    }
    (*tris)[indicesIndex] = outerIndex * stride + 1;
    indicesIndex++;
    
    (*tris)[indicesIndex] = outerIndex * stride + stride + 1;
    indicesIndex++;
    out->addPrimitiveSet(tris.get());
  }
  
  osgUtil::SmoothingVisitor::smooth(*out);
  
  return out.release();
}

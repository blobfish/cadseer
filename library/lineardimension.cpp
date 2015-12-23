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


//geometry constructed in positive quadrant. Like dimensioning in Y.

#include <cassert>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>

#include <osg/Geometry>
#include <osg/AutoTransform>
#include <osg/Billboard>
#include <osg/Material>
#include <osg/ComputeBoundsVisitor>
#include <osgText/Text>
#include <osgQt/QFontImplementation>

#include <application/application.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <library/lineardimension.h>

using namespace lbr;

DimensionArrow::DimensionArrow()
{
  build();
}

DimensionArrow::DimensionArrow(const DimensionArrow &dimArrowIn, const osg::CopyOp& copyOperation) :
MatrixTransform(dimArrowIn, copyOperation)
{
  //META_Node macros needs copy costructor but I am not using it.

}

void DimensionArrow::build()
{
  autoArrowScale = new osg::AutoTransform();
  autoArrowScale->setAutoScaleToScreen(true);
  this->addChild(autoArrowScale.get());
  
  arrowScale = new osg::MatrixTransform();
  arrowScale->setMatrix(osg::Matrixd::scale(75.0, 75.0, 75.0));
  autoArrowScale->addChild(arrowScale.get());
  
  billboard = new osg::Billboard();
  billboard->setAxis(osg::Vec3d(0.0, 1.0, 0.0));
  billboard->setNormal(osg::Vec3d(0.0, 0.0, 1.0));
  billboard->setMode(osg::Billboard::AXIAL_ROT);
  arrowScale->addChild(billboard.get());
  
  const prf::InteractiveParameter& iPref = prf::manager().rootPtr->interactiveParameter();
  arrowWidth = iPref.arrowWidth();
  arrowHeight = iPref.arrowHeight();
  updateArrow();
  
  line = new osg::Geometry();
  line->setDataVariance(osg::Object::DYNAMIC);
  osg::Vec3Array *vertexArray = new osg::Vec3Array();
  vertexArray->push_back(osg::Vec3d(0.0, 0.0, 0.0));
  vertexArray->push_back(osg::Vec3d(0.0, lineLength, 0.0));
  line->setVertexArray(vertexArray);
  line->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, 2));
  this->addChild(line.get());
}

void DimensionArrow::setLineLength(double lineLengthIn)
{
  if (lineLengthIn > 0.0)
  {
    lineLength = lineLengthIn;
    updateLine();
  }
}

void DimensionArrow::setInsideArrows()
{
  this->setMatrix(osg::Matrixd::scale(1.0, 1.0, 1.0));
}

void DimensionArrow::setOutsideArrows()
{
  this->setMatrix(osg::Matrixd::scale(1.0, -1.0, 1.0));
}

void DimensionArrow::updateLine()
{
  assert(line);
  osg::Vec3Array *vertexArray = dynamic_cast<osg::Vec3Array*>(line->getVertexArray());
  assert(vertexArray);
  assert(vertexArray->size() == 2);
  vertexArray->at(1) = osg::Vec3d(0.0, lineLength, 0.0);
  line->dirtyDisplayList();
  line->dirtyBound();
}

void DimensionArrow::setArrowSize(double arrowWidthIn, double arrowHeightIn)
{
  arrowWidth = arrowWidthIn;
  arrowHeight = arrowHeightIn;
  updateArrow();
}

osg::BoundingBox DimensionArrow::getArrowBoundingBox() const
{
  osg::ComputeBoundsVisitor visitor;
  autoArrowScale->accept(visitor);
  return visitor.getBoundingBox();
}

void DimensionArrow::updateArrow()
{
  if (arrow)
    billboard->removeDrawable(arrow.get());
  arrow = buildArrow(arrowWidth, arrowHeight);
  billboard->addDrawable(arrow.get());
}

osg::Geometry* DimensionArrow::buildArrow(double arrowWidthIn, double arrowHeightIn) const
{
  osg::Geometry *out = new osg::Geometry();
  
  osg::Vec3Array *array = new osg::Vec3Array();
  array->push_back(osg::Vec3(0.0, 0.0, 0.0));
  array->push_back(osg::Vec3(arrowWidthIn / 2.0, arrowHeightIn, 0.0));
  array->push_back(osg::Vec3(-arrowWidthIn / 2.0, arrowHeightIn, 0.0));
  out->setVertexArray(array);
  
  osg::Vec3Array *normalArray = new osg::Vec3Array();
  normalArray->push_back(osg::Vec3(0.0, 0.0, 1.0));
  out->setNormalArray(normalArray);
  out->setNormalBinding(osg::Geometry::BIND_OVERALL);
  
  out->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, 3));
  return out;
}

LinearDimension::LinearDimension()
{
  build();
  
  material = new osg::Material();
  material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0, 1.0, 0.0, 1.0));
  material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0, 1.0, 0.0, 1.0));
  this->getOrCreateStateSet()->setAttributeAndModes(material.get());
  this->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
  
  setNumChildrenRequiringUpdateTraversal(1);
}

LinearDimension::LinearDimension(const LinearDimension &linearDimIn, const osg::CopyOp& copyOperation) :
MatrixTransform(linearDimIn, copyOperation)
{
  //META_Node macros needs copy costructor but I am not using it.

}

void LinearDimension::traverse(osg::NodeVisitor& nv)
{
  osg::Group::traverse(nv);
  
  if (nv.getVisitorType() != osg::NodeVisitor::UPDATE_VISITOR)
    return;
  
  if
  (
    (!cachedTextBound.valid()) ||
    (cachedTextBound != getTextBoundingBox())
  )
    dirty = true;
  
  if (dirty)
  {
    cachedTextBound = getTextBoundingBox();
    update();
    dirty = false;
  }
}

void LinearDimension::setFlipped(bool signalIn)
{
  if (signalIn)
    flippedFactor = -1.0;
  else
    flippedFactor = 1.0;
  
  dirty = true;
}

void LinearDimension::setSpread(double spreadIn)
{
  spread = spreadIn;
  spreadY->setMatrix(osg::Matrixd::translate(osg::Vec3d(0.0, spread, 0.0)));
  
  std::ostringstream stream;
  stream << std::setprecision(3) << std::fixed << spread << std::endl;
  text->setText(stream.str());
  
  dirty = true;
}

void LinearDimension::update()
{
  //should we caching somethng here?
  offset = (cachedTextBound.radius() + squeeze) * flippedFactor;
  offsetX->setMatrix(osg::Matrixd::translate(osg::Vec3d(offset, 0.0, 0.0)));
  updateLineArrowTextPosition();
  updateExtensionLine();
}

void LinearDimension::setSqueeze(double squeezeIn)
{
  if (squeezeIn == squeeze)
    return;
  squeeze = squeezeIn;
  dirty = true;
}

void LinearDimension::setExtensionOffset(double offsetIn)
{
  extensionOffset = offsetIn * flippedFactor;
  dirty = true;
}

void LinearDimension::setCharacterSize(double sizeIn)
{
  characterSize = sizeIn;
  text->setCharacterSize(characterSize);
  dirty = true;
}

void LinearDimension::setColor(const osg::Vec4& colorIn)
{
  material->setAmbient(osg::Material::FRONT_AND_BACK, colorIn);
  material->setDiffuse(osg::Material::FRONT_AND_BACK, colorIn);
}

void LinearDimension::setArrowSize(double arrowWidthIn, double arrowHeightIn)
{
  //cache these for purposes.
  arrowWidth = arrowWidthIn;
  arrowHeight = arrowHeightIn;
  arrow->setArrowSize(arrowWidth, arrowHeight);
  
  dirty = true;
}

osg::BoundingBox LinearDimension::getTextBoundingBox()
{
  osg::ComputeBoundsVisitor visitor;
  textPosition->accept(visitor);
  return visitor.getBoundingBox();
}

void LinearDimension::build()
{
  const prf::InteractiveParameter& iPref = prf::manager().rootPtr->interactiveParameter();
  characterSize = iPref.characterSize();
  
  arrow = new DimensionArrow();
  extensionLine = buildExtensionLine();
  
  offsetX = new osg::MatrixTransform();
  offsetX->addChild(arrow.get());
  this->addChild(offsetX.get());
  
  mirrorY = new osg::MatrixTransform();
  mirrorY->setMatrix(osg::Matrixd::scale(osg::Vec3d(1.0, -1.0, 1.0)));
  mirrorY->addChild(offsetX.get());
  
  spreadY = new osg::MatrixTransform();
  spreadY->addChild(extensionLine.get());
  spreadY->addChild(mirrorY.get());
  this->addChild(spreadY.get());
  
  this->addChild(extensionLine.get());
  
  textPosition = new osg::AutoTransform();
  textPosition->setAutoRotateMode(osg::AutoTransform::ROTATE_TO_SCREEN);
  textPosition->setAutoScaleToScreen(true);
  this->addChild(textPosition.get());
  
  textScale = new osg::MatrixTransform();
  textScale->setMatrix(osg::Matrixd::scale(75.0, 75.0, 75.0));
  textPosition->addChild(textScale.get());
  
  text = new osgText::Text();
  text->setName("DimensionText");
  osg::ref_ptr<osgQt::QFontImplementation> fontImplement(new osgQt::QFontImplementation(qApp->font()));
  osg::ref_ptr<osgText::Font> textFont(new osgText::Font(fontImplement.get()));
  text->setFont(textFont);
  text->setColor(osg::Vec4(0.0, 0.0, 1.0, 1.0));
  text->setBackdropType(osgText::Text::OUTLINE);
  text->setBackdropColor(osg::Vec4(1.0, 1.0, 1.0, 1.0));
  text->setCharacterSize(characterSize);
  text->setAlignment(osgText::Text::CENTER_CENTER);
  textScale->addChild(text.get());
}

osg::Geometry* LinearDimension::buildExtensionLine()
{
  osg::Geometry *out = new osg::Geometry();
  out->setDataVariance(osg::Object::DYNAMIC);
  osg::Vec3Array *vertexArray = new osg::Vec3Array();
  vertexArray->push_back(osg::Vec3d(0.0, 0.0, 0.0));
  vertexArray->push_back(osg::Vec3d(0.1, 0.0, 0.0));
  out->setVertexArray(vertexArray);
  out->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, 2));
  
  return out;
}

void LinearDimension::updateExtensionLine()
{
  osg::Vec3Array *vertexArray = dynamic_cast<osg::Vec3Array*>(extensionLine->getVertexArray());
  assert(vertexArray);
  assert(vertexArray->size() == 2);
  vertexArray->at(0) = osg::Vec3d(extensionOffset, 0.0, 0.0);
  
  double extensionEnd = offset + (cachedTextBound.radius() * flippedFactor / 2.0) ;
  vertexArray->at(1) = osg::Vec3d(extensionEnd, 0.0, 0.0);
  extensionLine->dirtyDisplayList();
  extensionLine->dirtyBound();
}

void LinearDimension::updateLineArrowTextPosition()
{
  double radius = cachedTextBound.radius();
  double lineLength = (std::fabs(spread) - (2.0 * radius)) / 2.0;
  osg::BoundingBox arrowBox = arrow->getArrowBoundingBox();
  if (lineLength < (arrowBox.radius() * 2.0))
  {
    lineLength = arrowBox.radius() * 2.0;
    if (spread >= 0.0)
      arrow->setOutsideArrows();
    else
      arrow->setInsideArrows();
  }
  else
  {
    if (spread >= 0.0)
      arrow->setInsideArrows();
    else
      arrow->setOutsideArrows();
  }
  arrow->setLineLength(lineLength);
  
  double yPosition = spread / 2.0;
  if (std::fabs(yPosition) < radius)
  {
    yPosition = arrowBox.radius() * 2.0 + std::fabs(spread) + radius;
    if (spread < 0.0)
      yPosition *= -1.0;
  }
  textPosition->setPosition(osg::Vec3d(offset, yPosition, 0.0));
}

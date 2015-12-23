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

#ifndef LINEARDIMENSION_H
#define LINEARDIMENSION_H

#include <osg/ref_ptr>
#include <osg/MatrixTransform>

namespace osg{class Geometry; class Material; class Billboard; class AutoTransform;}
namespace osgText{class Text;}

namespace lbr
{
class DimensionArrow : public osg::MatrixTransform
{
public:
  DimensionArrow();
  DimensionArrow(const DimensionArrow&, const osg::CopyOp &copyOperation = osg::CopyOp::SHALLOW_COPY);
  META_Node(osg, DimensionArrow);
  
  void setArrowSize(double arrowWidthIn, double arrowHeightIn);
  osg::BoundingBox getArrowBoundingBox() const;
  void setLineLength(double lineLengthIn);
  osg::Geometry* buildArrow(double arrowWidthIn, double arrowHeightIn) const;
  void setInsideArrows();
  void setOutsideArrows();
  
protected:
  double lineLength = 2.0;
  double arrowWidth = 0.5;
  double arrowHeight = 1.0;
  
  void build();
  void updateLine();
  void updateArrow();
  
  osg::ref_ptr<osg::Geometry> arrow;
  osg::ref_ptr<osg::AutoTransform> autoArrowScale;
  osg::ref_ptr<osg::MatrixTransform> arrowScale;
  osg::ref_ptr<osg::Geometry> line;
  osg::ref_ptr<osg::Billboard> billboard;
};


class LinearDimension : public osg::MatrixTransform
{
public:
  LinearDimension();
  LinearDimension(const LinearDimension&, const osg::CopyOp &copyOperation = osg::CopyOp::SHALLOW_COPY);
  META_Node(osg, LinearDimension);
  
  virtual void traverse(osg::NodeVisitor& nv) override;
  
  void setSpread(double spreadIn);
  double getSpread() const {return spread;}
  void setSqueeze(double offsetIn);
  double getSqueeze() const {return squeeze;}
  void setExtensionOffset(double);
  double getExtensionOffset() const {return extensionOffset;}
  void setFlipped(bool signalIn);
  bool isFlipped() const {return flippedFactor == -1.0;}
  double getFlippedFactor() const {return flippedFactor;}
  void setCharacterSize(double sizeIn);
  void setColor(const osg::Vec4 &colorIn);
  void setArrowSize(double arrowWidthIn, double arrowHeightIn);
  osg::BoundingBox getTextBoundingBox();
protected:
  void build();
  osg::Geometry* buildExtensionLine();
  void updateExtensionLine();
  void updateLineArrowTextPosition();
  void update();
  
  osg::ref_ptr<osg::Material> material;
  
  osg::ref_ptr<DimensionArrow> arrow;
  osg::ref_ptr<osg::Geometry> extensionLine;
  osg::ref_ptr<osgText::Text> text;
  osg::ref_ptr<osg::AutoTransform> textPosition;
  osg::ref_ptr<osg::MatrixTransform> textScale;
  
  osg::ref_ptr<osg::MatrixTransform> spreadY; //!< translation in y.
  osg::ref_ptr<osg::MatrixTransform> offsetX; //!< translation in x.
  osg::ref_ptr<osg::MatrixTransform> mirrorY;
  
  osg::BoundingBox cachedTextBound;
  double spread = 0.0;
  double offset = 0.0;
  double squeeze = 0.0;
  double characterSize = 1.0;
  double arrowWidth = 0.5;
  double arrowHeight = 1.0;
  double extensionOffset = 0.0;
  double flippedFactor = 1.0;
  bool dirty = true;
};
}

#endif // LINEARDIMENSION_H

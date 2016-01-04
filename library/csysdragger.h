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

#ifndef CSYSDRAGGER_H
#define CSYSDRAGGER_H

#include <osgManipulator/Translate1DDragger>
#include <osg/ShapeDrawable>
#include <osg/Geometry>
#include <osg/LineWidth>
#include <osg/Switch>
#include <osg/AutoTransform>

#include "rotatecirculardragger.h"

namespace osgManipulator{class GridConstraint;}

namespace lbr
{

/*! @brief Coordinate system dragger
 * 
 * Has depth test turned off and cull back face enabled.
 * Don't forgot to add to renderbin to allow "overlay"
 * property
 */
class CSysDragger : public osgManipulator::CompositeDragger
{
public:
  CSysDragger();

//         META_OSGMANIPULATOR_Object(osgManipulator,CSysDragger)
  //manual implementation of META_OSGMANIPULATOR_Object(osgManipulator, CSysDragger)
  virtual osg::Object* cloneType() const {return new CSysDragger();}
  virtual bool isSameKindAs(const osg::Object* obj) const {return dynamic_cast<const CSysDragger *>(obj)!=NULL;}
  virtual const char* libraryName() const {return "osgManipulator"; }
  virtual const char* className() const {return "CSysDragger";}

  void setupDefaultGeometry();
  void setScreenScale(float);
  float getScreenScale(){return screenScale;}
  
  void setTranslationIncrement(double);
  void setRotationIncrement(double);
  
  void linkToMatrix(osg::MatrixTransform *);
  void unlinkToMatrix(osg::MatrixTransform *);
  void setLink();
  void setUnlink();
  bool isLinked(){return matrixLinked;}
  
  void updateMatrix(const osg::Matrixd &);
  
  //indexes in dragger switch 
  enum class SwitchIndexes
  {
    XTranslate = 0,
    YTranslate,
    ZTranslate,
    XRotate,
    YRotate,
    ZRotate,
    Origin,
    LinkIcon,
  };
  void show(SwitchIndexes index);
  void hide(SwitchIndexes index);
  
  void highlightOrigin();
  void unHighlightOrigin();
  
protected:
  virtual ~CSysDragger();
  void setupDefaultTranslation();
  void setupDefaultRotation();
  void setupIcons();
  
  osg::ref_ptr<osgManipulator::Translate1DDragger> xTranslate;
  osg::ref_ptr<osgManipulator::Translate1DDragger> yTranslate;
  osg::ref_ptr<osgManipulator::Translate1DDragger> zTranslate;
  osg::ref_ptr<RotateCircularDragger> xRotate;
  osg::ref_ptr<RotateCircularDragger> yRotate;
  osg::ref_ptr<RotateCircularDragger> zRotate;

  float screenScale;
  std::vector<osg::ref_ptr<osg::MatrixTransform> > matrixLinks;
  bool matrixLinked = true;

  osg::ref_ptr<osg::Switch> draggerSwitch;
  osg::ref_ptr<osg::Geode> originGeode;
  osg::ref_ptr<osg::AutoTransform> autoTransform;
  osg::ref_ptr<osg::MatrixTransform> matrixTransform;
  osg::ref_ptr<osg::Switch> iconSwitch;
  
  osg::ref_ptr<osgManipulator::GridConstraint> translateConstraint;
  osg::ref_ptr<AngleConstraint> rotateConstraint;
};
}

#endif // CSYSDRAGGER_H

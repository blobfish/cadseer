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

#include <gp_Trsf.hxx>
#include <gp_Pnt.hxx>
#include <gp_Ax2.hxx>

#include <osg/MatrixTransform>

#include <nodemaskdefs.h>
#include <feature/csysbase.h>

using namespace ftr;

static osg::Vec3d toOsg(const gp_Vec &occVecIn)
{
  osg::Vec3d out;
  
  out.x() = occVecIn.X();
  out.y() = occVecIn.Y();
  out.z() = occVecIn.Z();
  
  return out;
}

static osg::Vec3d toOsg(const gp_Pnt &occPointIn)
{
  osg::Vec3d out;
  
  out.x() = occPointIn.X();
  out.y() = occPointIn.Y();
  out.z() = occPointIn.Z();
  
  return out;
}

static osg::Matrixd toOsg(const gp_Ax2 &systemIn)
{
  
  osg::Vec3d xVector = toOsg(gp_Vec(systemIn.XDirection()));
  osg::Vec3d yVector = toOsg(gp_Vec(systemIn.YDirection()));
  osg::Vec3d zVector = toOsg(gp_Vec(systemIn.Direction()));
  osg::Vec3d origin = toOsg(systemIn.Location());
  
  //row major for openscenegraph.
  osg::Matrixd out
  (
    xVector.x(), xVector.y(), xVector.z(), 0.0,
    yVector.x(), yVector.y(), yVector.z(), 0.0,
    zVector.x(), zVector.y(), zVector.z(), 0.0,
    origin.x(), origin.y(), origin.z(), 1.0
  );
  
  return out;
}

static gp_Ax2 toOcc(const osg::Matrixd &m)
{
  gp_Ax2 out
  (
    gp_Pnt(m(3, 0), m(3, 1), m(3, 2)), //origin
    gp_Dir(m(2, 0), m(2, 1), m(2, 2)), //z vector
    gp_Dir(m(0, 0), m(0, 1), m(0, 2))  //x vector
  );
  
  return out;
}

bool DCallBack::receive(const osgManipulator::MotionCommand &commandIn)
{
  bool out = osgManipulator::DraggerCallback::receive(commandIn);
  
  //assuming at this point the transform matrix has been updated.
  //so copy it back to gp_Ax2 and mark the feature dirty.
  if (commandIn.getStage() == osgManipulator::MotionCommand::FINISH)
  {
    assert(csysBase);
    csysBase->setSystem(getTransform()->getMatrix());
  }
  
  return out;
}

CSysBase::CSysBase() : Base(), system()
{
  dragger = new CSysDragger();
  overlaySwitch->addChild(dragger);
  dragger->setScreenScale(50.0f);
  dragger->setRotationIncrement(15.0);
  dragger->setTranslationIncrement(0.25);
  dragger->setHandleEvents(true);
  dragger->setupDefaultGeometry();
  dragger->linkToMatrix(mainTransform.get());
  
  callBack = new DCallBack(dragger.get(), this);
  dragger->addDraggerCallback(callBack.get());
}

void CSysBase::setSystem(const gp_Ax2& systemIn)
{
  system = systemIn;
  
  bool wasDraggerLinked = dragger->isLinked();
  dragger->setUnlink();
  dragger->setMatrix(toOsg(systemIn));
  if (wasDraggerLinked)
    dragger->setLink();
  
  setModelDirty();
}

void CSysBase::setSystem(const osg::Matrixd& systemIn)
{
  system = toOcc(systemIn);
  setModelDirty();
}

void CSysBase::updateVisual()
{
  ftr::Base::updateVisual();
  
  //after dragging we need to reset the transform.
  if (isVisualClean())
    mainTransform->setMatrix(osg::Matrixd::identity());
}


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

#include <sstream>
#include <iomanip>
#include <cmath>
#include <limits>

#include <gp_Trsf.hxx>
#include <gp_Pnt.hxx>
#include <gp_Ax2.hxx>

#include <osg/MatrixTransform>
#include <osgManipulator/Command>

#include <nodemaskdefs.h>
#include <feature/csysbase.h>
#include <message/dispatch.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>

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

DCallBack::DCallBack(osg::MatrixTransform *t, CSysBase *csysBaseIn) : 
      osgManipulator::DraggerTransformCallback(t), csysBase(csysBaseIn)
{
  connectMessageOut(boost::bind(&msg::Dispatch::messageInSlot, &msg::dispatch(), _1));
  //we don't intake messages from the dispatcher.
}

bool DCallBack::receive(const osgManipulator::MotionCommand &commandIn)
{
  bool out = osgManipulator::DraggerCallback::receive(commandIn);
  
  static double lastTranslation;
  static int lastRotation;
  
  const osgManipulator::TranslateInLineCommand *tCommand = 
    dynamic_cast<const osgManipulator::TranslateInLineCommand *>(&commandIn);
  const osgManipulator::Rotate3DCommand *rCommand = 
    dynamic_cast<const osgManipulator::Rotate3DCommand *>(&commandIn);
  
  std::ostringstream stream;
  if (commandIn.getStage() == osgManipulator::MotionCommand::START)
  {
    if (tCommand)
      originStart = tCommand->getTranslation() * tCommand->getLocalToWorld();
    lastTranslation = 0.0;
    lastRotation = 0.0;
    
    lbr::CSysDragger &dragger = csysBase->getDragger();
    dragger.setTranslationIncrement(prf::manager().rootPtr->dragger().linearIncrement());
    dragger.setRotationIncrement(prf::manager().rootPtr->dragger().angularIncrement());
  }
    
  if (commandIn.getStage() == osgManipulator::MotionCommand::MOVE)
  {
    if (tCommand)
    {
      osg::Vec3d tVec = tCommand->getTranslation() * tCommand->getLocalToWorld();
      double diff = (tVec-originStart).length();
      if (std::fabs(diff - lastTranslation) < std::numeric_limits<double>::epsilon())
	return out;
      lastTranslation = diff;
      stream << QObject::tr("Translation Increment: ").toUtf8().data() << std::setprecision(3) << std::fixed <<
	prf::manager().rootPtr->dragger().linearIncrement() << std::endl <<
        QObject::tr("Translation: ").toUtf8().data() << std::setprecision(3) << std::fixed << diff;
    }
    if (rCommand)
    {
      double w = rCommand->getRotation().asVec4().w();
      int angle = static_cast<int>(std::round(osg::RadiansToDegrees(2.0 * std::acos(w))));
      if (angle == lastRotation)
	return out;
      lastRotation = angle;
      stream << QObject::tr("Rotation Increment: ").toUtf8().data() << 
	static_cast<int>(prf::manager().rootPtr->dragger().angularIncrement()) <<
	std::endl << QObject::tr("Rotation: ").toUtf8().data() << angle;
    }
  }
  
  //assuming at this point the transform matrix has been updated.
  //so copy it back to gp_Ax2 and mark the feature dirty.
  if (commandIn.getStage() == osgManipulator::MotionCommand::FINISH)
  {
    assert(csysBase);
    csysBase->setModelDirty();
    
    if (prf::manager().rootPtr->dragger().triggerUpdateOnFinish())
    {
      msg::Message uMessage;
      uMessage.mask = msg::Request | msg::Update;
      messageOutSignal(uMessage);
    }
  }
  
  msg::Message messageOut;
  messageOut.mask = msg::Request | msg::StatusText;
  vwr::Message vMessageOut;
  vMessageOut.text = stream.str();
  messageOut.payload = vMessageOut;
  messageOutSignal(messageOut);
  
  return out;
}

CSysBase::CSysBase() : Base(), system()
{
  dragger = new lbr::CSysDragger();
  overlaySwitch->addChild(dragger);
  dragger->setScreenScale(75.0f);
  dragger->setRotationIncrement(prf::manager().rootPtr->dragger().angularIncrement());
  dragger->setTranslationIncrement(prf::manager().rootPtr->dragger().linearIncrement());
  dragger->setHandleEvents(false);
  dragger->setupDefaultGeometry();
  dragger->linkToMatrix(mainTransform.get());
  
  callBack = new DCallBack(dragger.get(), this);
  dragger->addDraggerCallback(callBack.get());
}

void CSysBase::setSystem(const gp_Ax2& systemIn)
{
  system = systemIn;
  setModelDirty();
}

void CSysBase::setSystem(const osg::Matrixd& systemIn)
{
  setSystem(toOcc(systemIn));
}

void CSysBase::updateDragger()
{
  bool wasDraggerLinked = dragger->isLinked();
  dragger->setUnlink();
  dragger->setMatrix(toOsg(system));
  if (wasDraggerLinked)
    dragger->setLink();
}

void CSysBase::updateModel(const UpdateMap &)
{
  setSystem(toOsg(system) * mainTransform->getMatrix());
  mainTransform->setMatrix(osg::Matrixd::identity());
}

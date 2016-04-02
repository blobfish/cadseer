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
#include <globalutilities.h>
#include <message/dispatch.h>
#include <message/observer.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <project/serial/xsdcxxoutput/featurecsysbase.h>
#include <feature/csysbase.h>

using namespace ftr;

DCallBack::DCallBack(osg::MatrixTransform *t, CSysBase *csysBaseIn) : 
      osgManipulator::DraggerTransformCallback(t), csysBase(csysBaseIn)
{
  observer = std::move(std::unique_ptr<msg::Observer>(new msg::Observer()));
}

bool DCallBack::receive(const osgManipulator::MotionCommand &commandIn)
{
  bool out = osgManipulator::DraggerCallback::receive(commandIn);
  
  static double lastTranslation = 0.0;
  static int lastRotation = 0.0;
  
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
    if (lastTranslation != 0.0 || lastRotation != 0.0)
    {
      assert(csysBase);
      csysBase->setModelDirty();
      
      //add git message.
      std::ostringstream gitStream;
      gitStream << QObject::tr("Reposition feature: ").toStdString() << csysBase->getName().toStdString() <<
	" id: " << boost::uuids::to_string(csysBase->getId());
      msg::Message gitMessage;
      gitMessage.mask = msg::Request | msg::GitMessage;
      prj::Message pMessage;
      pMessage.gitMessage = gitStream.str();
      gitMessage.payload = pMessage;
      observer->messageOutSignal(gitMessage);
      
      if (prf::manager().rootPtr->dragger().triggerUpdateOnFinish())
      {
	msg::Message uMessage;
	uMessage.mask = msg::Request | msg::Update;
	observer->messageOutSignal(uMessage);
      }
    }
  }
  
  msg::Message messageOut;
  messageOut.mask = msg::Request | msg::StatusText;
  vwr::Message vMessageOut;
  vMessageOut.text = stream.str();
  messageOut.payload = vMessageOut;
  observer->messageOutSignal(messageOut);
  
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
  dragger->setUserValue(gu::idAttributeTitle, boost::uuids::to_string(id));
  
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
  setSystem(gu::toOcc(systemIn));
}

void CSysBase::updateDragger()
{
  bool wasDraggerLinked = dragger->isLinked();
  dragger->setUnlink();
  dragger->setMatrix(gu::toOsg(system));
  if (wasDraggerLinked)
    dragger->setLink();
}

void CSysBase::updateModel(const UpdateMap &)
{
  setSystem(gu::toOsg(system) * mainTransform->getMatrix());
  mainTransform->setMatrix(osg::Matrixd::identity());
}

prj::srl::FeatureCSysBase CSysBase::serialOut()
{
  prj::srl::FeatureBase fBase = Base::serialOut();
  
  osg::Matrixd m = gu::toOsg(system);
  prj::srl::CSys systemOut
  (
    m(0,0), m(0,1), m(0,2), m(0,3),
    m(1,0), m(1,1), m(1,2), m(1,3),
    m(2,0), m(2,1), m(2,2), m(2,3),
    m(3,0), m(3,1), m(3,2), m(3,3)
  );
  
  return prj::srl::FeatureCSysBase(fBase, systemOut);
}

void CSysBase::serialIn(const prj::srl::FeatureCSysBase& sCSysBaseIn)
{
  Base::serialIn(sCSysBaseIn.featureBase());
  dragger->setUserValue(gu::idAttributeTitle, boost::uuids::to_string(id));
  
  const prj::srl::CSys &s = sCSysBaseIn.csys();
  osg::Matrixd m;
  m(0,0) = s.i0j0(); m(0,1) = s.i0j1(); m(0,2) = s.i0j2(); m(0,3) = s.i0j3();
  m(1,0) = s.i1j0(); m(1,1) = s.i1j1(); m(1,2) = s.i1j2(); m(1,3) = s.i1j3();
  m(2,0) = s.i2j0(); m(2,1) = s.i2j1(); m(2,2) = s.i2j2(); m(2,3) = s.i2j3();
  m(3,0) = s.i3j0(); m(3,1) = s.i3j1(); m(3,2) = s.i3j2(); m(3,3) = s.i3j3();

  this->setSystem(m);
  updateDragger();
}

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

#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <limits>

#include <QObject>
#include <QMetaObject>
#include <QCoreApplication>

#include <osg/Switch>
#include <osg/AutoTransform>

#include <feature/parameter.h>
#include <message/dispatch.h>
#include <message/observer.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <library/lineardimension.h>
#include <library/lineardragger.h>
#include <library/ipgroup.h>

using namespace lbr;

IPGroup::IPGroup()
{
  assert(0);
}

IPGroup::IPGroup(ftr::prm::Parameter *parameterIn) :
osg::MatrixTransform(),
mainDim(),
differenceDim(),
overallDim(),
parameter(parameterIn)
{
  observer = std::move(std::unique_ptr<msg::Observer>(new msg::Observer()));
  observer->name = "IPGroup";
  
  rotation = new osg::AutoTransform();
  this->addChild(rotation.get());
  
  dimSwitch = new osg::Switch();
  rotation->addChild(dimSwitch.get());
  
  mainDim = new LinearDimension();
  dimSwitch->addChild(mainDim.get());
  
  differenceDim = new LinearDimension();
  differenceDim->setColor(osg::Vec4d(0.79, 0.13, 0.48, 1.0));
  dimSwitch->addChild(differenceDim.get());
  dimSwitch->setChildValue(differenceDim.get(), false);
  
  overallDim = new LinearDimension();
  overallDim->setColor(osg::Vec4d(0.79, 0.13, 0.48, 1.0));
  dimSwitch->addChild(overallDim.get());
  dimSwitch->setChildValue(overallDim.get(), false);
  
  draggerMatrix = new osg::MatrixTransform();
  draggerMatrix->setMatrix(osg::Matrixd::identity());
  rotation->addChild(draggerMatrix.get());
  
  draggerSwitch = new osg::Switch();
  draggerMatrix->addChild(draggerSwitch.get());
  
  dragger = new LinearDragger();
  dragger->setScreenScale(75.0);
  dragger->setColor(osg::Vec4d(0.79, 0.13, 0.48, 1.0));
  draggerSwitch->addChild(dragger.get());
  
  ipCallback = new IPCallback(dragger.get(), this);
  dragger->addDraggerCallback(ipCallback.get());
  
  parameter->connectValue(boost::bind(&IPGroup::valueHasChanged, this));
  parameter->connectConstant(boost::bind(&IPGroup::constantHasChanged, this));
}

IPGroup::IPGroup(const IPGroup& copy, const osg::CopyOp& copyOp) : osg::MatrixTransform(copy, copyOp)
{
  assert(0);
}

void IPGroup::setRotationAxis(const osg::Vec3d &axisIn, const osg::Vec3d &normalIn)
{
  rotation->setAutoRotateMode(osg::AutoTransform::ROTATE_TO_AXIS);
  rotation->setAxis(axisIn);
  rotation->setNormal(normalIn);
}

void IPGroup::setMatrixDims(const osg::Matrixd& matrixIn)
{
  mainDim->setMatrix(matrixIn);
  differenceDim->setMatrix(matrixIn);
  overallDim->setMatrix(matrixIn);
}

void IPGroup::setMatrixDragger(const osg::Matrixd& matrixIn)
{
  draggerMatrix->setMatrix(matrixIn);
}

void IPGroup::setDimsFlipped(bool flippedIn)
{
  mainDim->setFlipped(flippedIn);
  differenceDim->setFlipped(flippedIn);
  overallDim->setFlipped(flippedIn);
}

void IPGroup::noAutoRotateDragger()
{
  rotation->removeChild(draggerMatrix.get());
  this->addChild(draggerMatrix.get());
}

osg::BoundingBox IPGroup::maxTextBoundingBox()
{
  osg::BoundingBox box = mainDim->getTextBoundingBox();
  if (differenceDim->getTextBoundingBox().radius() > box.radius())
    box = differenceDim->getTextBoundingBox();
  if (overallDim->getTextBoundingBox().radius() > box.radius())
    box = overallDim->getTextBoundingBox();
  
  return box;
}

void IPGroup::valueHasChanged()
{
  assert(parameter);
  setParameterValue(static_cast<double>(*parameter));
}

void IPGroup::constantHasChanged()
{
  assert(parameter);
  if (parameter->isConstant())
    draggerShow();
  else
    draggerHide();
}

void IPGroup::setParameterValue(double valueIn)
{
  if (value == valueIn)
    return;
  value = valueIn;
  mainDim->setSpread(value);
  overallDim->setSpread(value);
  
  dragger->setMatrix(osg::Matrixd::translate(0.0, 0.0, value));
}

void IPGroup::draggerShow()
{
  draggerSwitch->setAllChildrenOn();
}

void IPGroup::draggerHide()
{
  draggerSwitch->setAllChildrenOff();
}

bool IPGroup::processMotion(const osgManipulator::MotionCommand &commandIn)
{
  assert(parameter);
  
  static double lastTranslation;
  
  const osgManipulator::TranslateInLineCommand *tCommand = 
    dynamic_cast<const osgManipulator::TranslateInLineCommand *>(&commandIn);
  assert(tCommand);
  
  std::ostringstream stream;
  if (commandIn.getStage() == osgManipulator::MotionCommand::START)
  {
    originStart = tCommand->getTranslation() * tCommand->getLocalToWorld();
    lastTranslation = 0.0;
    
    osg::Matrixd temp = differenceDim->getMatrix();
    temp.setTrans(temp.getRotate() * osg::Vec3d(0.0, value, 0.0));
    differenceDim->setMatrix(temp);
    differenceDim->setSpread(0.0);
    
    //set other dimensions properties relative to mains.
    double mainTextDia = mainDim->getTextBoundingBox().radius() * 2.0;
    double extensionOffset = mainDim->getExtensionOffset() * mainDim->getFlippedFactor();
    differenceDim->setSqueeze(mainDim->getSqueeze() + mainTextDia);
    differenceDim->setExtensionOffset(extensionOffset);
    overallDim->setSqueeze(differenceDim->getSqueeze() + mainTextDia);
    overallDim->setExtensionOffset(extensionOffset);
    
    dragger->setIncrement(prf::manager().rootPtr->dragger().linearIncrement());
    
    dimSwitch->setChildValue(differenceDim.get(), true);
    dimSwitch->setChildValue(overallDim.get(), true);
  }
    
  if (commandIn.getStage() == osgManipulator::MotionCommand::MOVE)
  {
    //the constraint on dragger limits the values but still passes all
    //events on through. so we filter them out.
    //have to be in world to calculate the value
    osg::Vec3d tVec = tCommand->getTranslation() * tCommand->getLocalToWorld();
    double diff = (tVec-originStart).length();
    if (std::fabs(diff - lastTranslation) < std::numeric_limits<double>::epsilon())
      return false;
    
    //but have to be local to get sign of value.
    double directionFactor = 1.0;
    if (tCommand->getTranslation().z() < 0.0)
      directionFactor = -1.0;
    double freshOverall = diff * directionFactor + value;
    
    //limit to positive overall size.
    //might make this a custom manipulator constraint.
    if (!parameter->isValidValue(freshOverall))
    {
      osg::Matrixd temp = dragger->getMatrix();
      temp.setTrans(osg::Vec3d(0.0, 0.0, overallDim->getSpread()));
      dragger->setMatrix(temp);
      observer->out(msg::buildStatusMessage(
        QObject::tr("Value out of range").toStdString()));
      return false;
    }
    lastTranslation = diff;
    
    differenceDim->setSpread(diff * directionFactor);
    overallDim->setSpread(freshOverall);
    
    stream << QObject::tr("Translation Increment: ").toUtf8().data() << std::setprecision(3) << std::fixed <<
      prf::manager().rootPtr->dragger().linearIncrement() << std::endl <<
      QObject::tr("Translation: ").toUtf8().data() << std::setprecision(3) << std::fixed << diff;
  }
  
  if (commandIn.getStage() == osgManipulator::MotionCommand::FINISH)
  {
    dimSwitch->setChildValue(differenceDim.get(), false);
    dimSwitch->setChildValue(overallDim.get(), false);
    parameter->setValue(overallDim->getSpread());
    
    //add git message.
    std::ostringstream gitStream;
    gitStream << QObject::tr("Parameter ").toStdString() << parameter->getName().toStdString() <<
      QObject::tr(" changed to: ").toStdString() << static_cast<double>(*parameter);
    observer->out(msg::buildGitMessage(gitStream.str()));
    
    if (prf::manager().rootPtr->dragger().triggerUpdateOnFinish())
    {
      msg::Message messageOut(msg::Request | msg::Project | msg::Update);
      QMetaObject::invokeMethod(qApp, "messageSlot", Qt::QueuedConnection, Q_ARG(msg::Message, messageOut));
      
//       observer->out(msg::Mask(msg::Request | msg::Project | msg::Update));
    }
  }
  
  observer->out(msg::buildStatusMessage(stream.str()));
  
  return true;
}

IPCallback::IPCallback(osg::MatrixTransform* t, IPGroup *groupIn):
  DraggerTransformCallback(t, HANDLE_TRANSLATE_IN_LINE), group(groupIn)
{
  assert(group);
}

bool IPCallback::receive(const osgManipulator::TranslateInLineCommand& commandIn)
{
  return group->processMotion(commandIn);
}

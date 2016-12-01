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
#include <assert.h>

#include <osg/Geometry>
#include <osg/LineWidth>
#include <osgUtil/SmoothingVisitor>

#include "circlebuilder.h"
#include "torusbuilder.h"
#include "rotatecirculardragger.h"

using namespace osg;
using namespace osgManipulator;
using namespace lbr;

AngleConstraint::AngleConstraint(osg::Node& refNodeIn, const osg::Vec3d& startIn, osg::Vec3d::value_type radiansIn) :
  Constraint(refNodeIn), start(startIn), increment(radiansIn)
{
}

bool AngleConstraint::constrain(Rotate3DCommand& command) const
{
  if (command.getStage() == osgManipulator::MotionCommand::START)
        computeLocalToWorldAndWorldToLocal();
  else if (command.getStage() == osgManipulator::MotionCommand::FINISH)
      return true;
  
  if(command.getRotation().zeroRotation())
    return true;
  
  osg::Vec3d rotationVectorIn;
  osg::Vec3d::value_type rotationAngleIn;
  assert(increment > 0.0);
  command.getRotation().getRotate(rotationAngleIn, rotationVectorIn);
  osg::Vec3d::value_type whole = std::floor(rotationAngleIn / increment);
  osg::Vec3d::value_type fractional = std::fmod(rotationAngleIn, increment);
  if (fractional > (increment / 2.0))
    whole++;
  
  osg::Quat rotationOut(whole * increment, rotationVectorIn);
  command.setRotation(rotationOut);

  return true;
}

RotateCircularDragger::RotateCircularDragger() : Dragger()
{
  _projector = new CircleProjector();
  setColor(Vec4(0.0f, 1.0f, 0.0f, 1.0f));
  setPickColor(Vec4(1.0f, 1.0f, 0.0f, 1.0f));
  
  majorRadius = 0.75;
  majorIsoLines = 16;
  minorRadius = 0.05 * majorRadius;
  minorIsoLines = 8;
  angularSpan = 90.0;
}

RotateCircularDragger::~RotateCircularDragger()
{
}

void RotateCircularDragger::setAngularSpan(const osg::Vec3d::value_type &angularSpanIn)
{
  if (angularSpanIn == angularSpan)
    return;
  angularSpan = angularSpanIn;
  update();
}

void RotateCircularDragger::setMajorRadius(const osg::Vec3d::value_type &majorRadiusIn)
{
  if (majorRadiusIn == majorRadius)
    return;
  majorRadius = majorRadiusIn;
  _projector->setRadius(majorRadius);
  update();
}

void RotateCircularDragger::setMinorRadius(const osg::Vec3d::value_type &minorRadiusIn)
{
  if (minorRadiusIn == minorRadius)
    return;
  minorRadius = minorRadiusIn;
  update();
}

void RotateCircularDragger::setMajorIsoLines(const std::size_t &majorSegmentsIn)
{
  if (majorSegmentsIn == majorIsoLines)
    return;
  majorIsoLines = majorSegmentsIn;
  update();
}

void RotateCircularDragger::setMinorIsoLines(const std::size_t &minorSegmentsIn)
{
  if (minorSegmentsIn == minorIsoLines)
    return;
  minorIsoLines = minorSegmentsIn;
  update();
}

void RotateCircularDragger::update()
{
  if (line)
  {
    const osg::Node::ParentList &lineParents = line->getParents();
    osg::Geometry *oldLine = line.get();
    line = buildLine();
    for (auto &node : lineParents)
      node->replaceChild(oldLine, line.get());
  }
  
  if (torus)
  {
    const osg::Node::ParentList &torusParents = torus->getParents();
    osg::Geometry *oldTorus = torus.get();
    torus = buildTorus();
    for (auto &node : torusParents)
      node->replaceChild(oldTorus, torus.get());
  }
}

void RotateCircularDragger::setupDefaultGeometry()
{
  assert(!line); //don't call setupDefaultGeometry twice.
  assert(!torus); //don't call setupDefaultGeometry twice.
  
  line = buildLine();
  torus = buildTorus();
  
  osg::LineWidth *lineWidth = new osg::LineWidth();
  lineWidth->setWidth(5.0);
  line->getOrCreateStateSet()->setAttributeAndModes(lineWidth, osg::StateAttribute::ON);
  
  this->addChild(line.get());
  this->addChild(torus.get());
}

osg::Geometry* RotateCircularDragger::buildLine() const
{
  CircleBuilder cBuilder;
  cBuilder.setSegments(majorIsoLines);
  cBuilder.setRadius(majorRadius);
  cBuilder.setAngularSpanDegrees(angularSpan);
  std::vector<osg::Vec3d> circlePoints = cBuilder;
  
  osg::Vec3Array *points = new osg::Vec3Array();
  std::copy(circlePoints.begin(), circlePoints.end(), std::back_inserter(*points));
  
  osg::Geometry *out = new osg::Geometry();
  out->setVertexArray(points);
  out->setUseDisplayList(false);
  out->setUseVertexBufferObjects(true);
  
  osg::PrimitiveSet::Mode mode = (cBuilder.isCompleteCircle()) ? osg::PrimitiveSet::LINE_LOOP :
    osg::PrimitiveSet::LINE_STRIP;

  out->addPrimitiveSet(new osg::DrawArrays(mode, 0, points->size()));
  
  out->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
  
  return out;
}

osg::Geometry* RotateCircularDragger::buildTorus() const
{
  TorusBuilder tBuilder;
  tBuilder.setMajorRadius(majorRadius);
  tBuilder.setMajorIsoLines(majorIsoLines);
  tBuilder.setMinorRadius(minorRadius);
  tBuilder.setMinorIsoLines(minorIsoLines);
  tBuilder.setAngularSpanDegrees(angularSpan);
  
  osg::Geometry *localTorus = tBuilder;
  setDrawableToAlwaysCull(*localTorus);
  
  return localTorus;
}

osg::Vec3d RotateCircularDragger::calculateArcMidPoint() const
{
  osg::Vec3d out(majorRadius, 0.0, 0.0);
  osg::Quat rotation(osg::DegreesToRadians(angularSpan) / 2.0, osg::Vec3d(0.0, 0.0, 1.0));
  return rotation * out;
}

bool RotateCircularDragger::handle(const PointerInfo& pointer, const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
{
  // Check if the dragger node is in the nodepath.
  if (!pointer.contains(this)) return false;
  
  switch (ea.getEventType())
    {
        // Pick start.
        case (osgGA::GUIEventAdapter::PUSH):
        {
            // Get the LocalToWorld matrix for this node and set it for the projector.
            osg::NodePath nodePathToRoot;
            computeNodePathToRoot(*this,nodePathToRoot);
            osg::Matrix localToWorld = osg::computeLocalToWorld(nodePathToRoot);
            _projector->setLocalToWorld(localToWorld);

            _startLocalToWorld = _projector->getLocalToWorld();
            _startWorldToLocal = _projector->getWorldToLocal();

            osg::Vec3d projectedPoint;
            dragStarted = false;
            if (_projector->project(pointer, projectedPoint))
            {
                // Generate the motion command.
                osg::ref_ptr<Rotate3DCommand> cmd = new Rotate3DCommand();
                cmd->setStage(MotionCommand::START);
                cmd->setLocalToWorldAndWorldToLocal(_startLocalToWorld,_startWorldToLocal);

                // Dispatch command.
                dispatch(*cmd);

                // Set color to pick color.
                setMaterialColor(_pickColor,*this);

                startPoint = projectedPoint;
                aa.requestRedraw();
                dragStarted = true;
            }
            return true;
        }

        // Pick move.
        case (osgGA::GUIEventAdapter::DRAG):
        {
            if (!dragStarted)
                return false;
            // Get the LocalToWorld matrix for this node and set it for the projector.
            _projector->setLocalToWorld(_startLocalToWorld);

            osg::Vec3d projectedPoint;
            if (_projector->project(pointer, projectedPoint))
            {
                osg::Quat rotation;
                rotation.makeRotate(startPoint, projectedPoint);

                // Generate the motion command.
                osg::ref_ptr<Rotate3DCommand> cmd = new Rotate3DCommand();
                cmd->setStage(MotionCommand::MOVE);
                cmd->setLocalToWorldAndWorldToLocal(_startLocalToWorld,_startWorldToLocal);
                cmd->setRotation(rotation);

                // Dispatch command.
                dispatch(*cmd);

                aa.requestRedraw();
            }
            return true;
        }

        // Pick finish.
        case (osgGA::GUIEventAdapter::RELEASE):
        {
            if (!dragStarted)
                return false;
            osg::ref_ptr<Rotate3DCommand> cmd = new Rotate3DCommand();

            cmd->setStage(MotionCommand::FINISH);
            cmd->setLocalToWorldAndWorldToLocal(_startLocalToWorld,_startWorldToLocal);

            // Dispatch command.
            dispatch(*cmd);

            // Reset color.
            setMaterialColor(_color,*this);

            aa.requestRedraw();

            return true;
        }
        default:
            return false;
    }
}

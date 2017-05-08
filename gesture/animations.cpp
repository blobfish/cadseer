/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#include <osg/MatrixTransform>
#include <osg/Geometry>
#include <osg/Switch>
#include <osg/Material>

#include <gesture/animations.h>

using namespace osg;
using namespace osgAnimation;


bool gsn::isCloseEnough(const osg::Vec3d &vec1, const osg::Vec3d &vec2)
{
  double epsilon = .01;
  if ((vec1 - vec2).length() < epsilon)
    return true;
  return false;
}

bool gsn::isCloseEnough(const float &time1, const float &time2)
{
  float epsilon = .001;
  if (fabs(time1 - time2) < epsilon)
    return true;
  return false;
}

gsn::NodeBase::NodeBase(const Vec3d &sourceIn, const Vec3d &targetIn, const float &timeSpanIn) :
    source(sourceIn), target(targetIn), direction(1.0, 0.0, 0.0), timeSpan(timeSpanIn), length(0.0),
    finished(false), lastTime(-1.0)
{
  Vec3d temp = targetIn - sourceIn;
  length = temp.length();
  direction = temp;
  direction.normalize();
}

void gsn::NodeBase::operator()(osg::Node*, osg::NodeVisitor*)
{
  assert(0);
}

gsn::NodeExpand::NodeExpand(const osg::Vec3d &sourceIn, const osg::Vec3d &targetIn, const float &timeSpanIn) :
  NodeBase(sourceIn, targetIn, timeSpanIn)
{
  motion = new InOutCubicMotion(0.0, timeSpan, 1.0, Motion::CLAMP);
}

void gsn::NodeExpand::operator()(osg::Node* node, osg::NodeVisitor* nv)
{
  if (finished)
  {
    traverse(node, nv);
    return;
  }
  
  MatrixTransform* transform = dynamic_cast<MatrixTransform*>(node);
  assert(transform);

  double currentTime = nv->getFrameStamp()->getSimulationTime();

  if(lastTime < 0.0f)
  {
    lastTime = currentTime;
    Switch *aSwitch = dynamic_cast<Switch*> (transform->getChild(transform->getNumChildren() - 1));
    assert(aSwitch);
    aSwitch->setAllChildrenOn();
  }

  motion->update(currentTime - lastTime);
  lastTime = currentTime;
  
  float motionValue = motion->getValue();
  transform->setMatrix(Matrixd::translate((direction * motionValue * length) + source + osg::Vec3d(0.0, 0.0, -0.001)));
  
  if (isCloseEnough(motionValue, 1.0))
  {
    //put right at target to elliminate slop of movement.
    transform->setMatrix(Matrixd::translate(target));
    finished = true;
  }
  
  traverse(node, nv);
}

void gsn::NodeExpand::setCurrent(const osg::Vec3d &)
{
  //nothing for now.
}

gsn::NodeCollapse::NodeCollapse(const osg::Vec3d &sourceIn, const osg::Vec3d &targetIn, const float &timeSpanIn) :
  gsn::NodeBase(sourceIn, targetIn, timeSpanIn)
{
  motion = new InOutCubicMotion(0.0, timeSpan, 1.0, Motion::CLAMP);
}

void gsn::NodeCollapse::operator()(osg::Node* node, osg::NodeVisitor* nv)
{
  if (finished)
  {
    traverse(node, nv);
    return;
  }
  
  MatrixTransform* transform = dynamic_cast<MatrixTransform*>(node);
  assert(transform);

  double currentTime = nv->getFrameStamp()->getSimulationTime();

  if(lastTime < 0.0f)
    lastTime = currentTime;

  motion->update(currentTime - lastTime);
  lastTime = currentTime;
  
  float motionValue = motion->getValue();
  transform->setMatrix(Matrix::translate((direction * motionValue * length) + source + osg::Vec3d(0.0, 0.0, -0.001)));
  
  if (isCloseEnough(motionValue, 1.0))
  {
    //put right at target to elliminate slop of movement.
    transform->setMatrix(Matrixd::translate(target));
    Switch *aSwitch = dynamic_cast<Switch*> (transform->getChild(transform->getNumChildren() - 1));
    assert(aSwitch);
    aSwitch->setAllChildrenOff();
    finished = true;
  }
  
  traverse(node, nv);
}

void gsn::NodeCollapse::setCurrent(const osg::Vec3d &)
{
  //nothing for now.
}

gsn::GeometryBase::GeometryBase(const osg::Vec3d &sourceIn, const osg::Vec3d &targetIn, const float &timeSpanIn) : 
    source(sourceIn), target(targetIn), direction(1.0, 0.0, 0.0), timeSpan(timeSpanIn), length(0.0),
    finished(false), lastTime(-1.0)
{
  Vec3d temp = targetIn - sourceIn;
  length = temp.length();
  direction = temp;
  direction.normalize();
  normal = osg::Quat(osg::PI_2, osg::Vec3d(0.0, 0.0, 1.0)) * direction;
}

void gsn::GeometryBase::update(osg::NodeVisitor *, osg::Drawable *)
{
  assert(0);
}

gsn::GeometryExpand::GeometryExpand(const osg::Vec3d &sourceIn, const osg::Vec3d &targetIn, const float &timeSpanIn) :
  gsn::GeometryBase(sourceIn, targetIn, timeSpanIn)
{
  motion = new InOutCubicMotion(0.0, timeSpan, 1.0, Motion::CLAMP);
}

void gsn::GeometryExpand::update(osg::NodeVisitor *visitor, osg::Drawable *drawable)
{
  if (finished)
    return;
  
  Geometry *geometry = dynamic_cast<Geometry*>(drawable);
  assert(geometry);
  Vec3Array *vertices = dynamic_cast<Vec3Array *>(geometry->getVertexArray());
  assert(vertices);
  assert(vertices->size() == 4);
  
  double currentTime = visitor->getFrameStamp()->getSimulationTime();
  if(lastTime < 0.0f)
    lastTime = currentTime;
  motion->update(currentTime - lastTime);
  lastTime = currentTime;
  
  (*vertices)[0] = -normal * lineWidth;
  (*vertices)[1] = (*vertices)[0] + (direction * (length * motion->getValue()));
  (*vertices)[2] = (*vertices)[1] + (normal * (2.0 * lineWidth));
  (*vertices)[3] = normal * lineWidth;
  vertices->dirty();
  geometry->dirtyDisplayList();
  geometry->dirtyBound();
  
  if (isCloseEnough(1.0, motion->getValue()))
    finished = true;
}

void gsn::GeometryExpand::setCurrent(const osg::Vec3d &)
{
  //not used right now.
}

gsn::GeometryCollapse::GeometryCollapse(const osg::Vec3d &sourceIn, const osg::Vec3d &targetIn, const float &timeSpanIn) :
  gsn::GeometryBase(sourceIn, targetIn, timeSpanIn)
{
  motion = new InOutCubicMotion(0.0, timeSpan, 1.0, Motion::CLAMP);
}

void gsn::GeometryCollapse::update(osg::NodeVisitor *visitor, osg::Drawable *drawable)
{
  if (finished)
    return;
  
  Geometry *geometry = dynamic_cast<Geometry*>(drawable);
  assert(geometry);
  Vec3Array *vertices = dynamic_cast<Vec3Array *>(geometry->getVertexArray());
  assert(vertices);
  assert(vertices->size() == 4);
  
  double currentTime = visitor->getFrameStamp()->getSimulationTime();
  if(lastTime < 0.0f)
    lastTime = currentTime;
  motion->update(currentTime - lastTime);
  lastTime = currentTime;
  
  (*vertices)[0] = -normal * lineWidth;
  (*vertices)[1] = (*vertices)[0] + (direction * (length * (1.0 - motion->getValue())));
  (*vertices)[2] = (*vertices)[1] + (normal * (2.0 * lineWidth));
  (*vertices)[3] = normal * lineWidth;
  
  vertices->dirty();
  geometry->dirtyDisplayList();
  geometry->dirtyBound();
  
  if (isCloseEnough(1.0, motion->getValue()))
    finished = true;
}

void gsn::GeometryCollapse::setCurrent(const osg::Vec3d &)
{
  //not used right now.
}


gsn::FadeState::FadeState(const float &timeSpanIn, const gsn::FadeState::Fade &fadeSignal) :
  lastTime(-1.0), finished(false), fade(fadeSignal)
{
  motion = new osgAnimation::InOutCubicMotion(0.0, timeSpanIn, 1.0, osgAnimation::Motion::CLAMP);
}

void gsn::FadeState::operator()(osg::StateAttribute *attribute, osg::NodeVisitor *visitor)
{
  if (finished)
    return;
  osg::Material *material = dynamic_cast<osg::Material *>(attribute);
  assert(material);
  
  double currentTime = visitor->getFrameStamp()->getSimulationTime();
  if(lastTime < 0.0f)
    lastTime = currentTime;
  motion->update(currentTime - lastTime);
  lastTime = currentTime;
  
  if (isCloseEnough(1.0, motion->getValue()))
    finished = true;
  
  float motionValue = motion->getValue();
  if(fade == gsn::FadeState::Out)
    motionValue = 1.0 - motionValue;
  
  osg::Vec4 diffuse = material->getDiffuse(osg::Material::FRONT_AND_BACK);
  diffuse[3] = motionValue;
  material->setDiffuse(osg::Material::FRONT_AND_BACK, diffuse);
}

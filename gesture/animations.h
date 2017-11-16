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

#ifndef GSN_ANIMATIONS_H
#define GSN_ANIMATIONS_H

#include <assert.h>

#include <osg/NodeCallback>
#include <osg/Drawable>
#include <osgAnimation/EaseMotion>


//goofy declations for friend stream operator inside a namespace.
namespace gsn
{
  class NodeBase;
}
std::ostream& operator<<(std::ostream& os, const gsn::NodeBase& obj);

namespace gsn
{
  bool isCloseEnough(const osg::Vec3d &vec1, const osg::Vec3d &vec2);
  bool isCloseEnough(const float &time1, const float &time2);

class NodeBase : public osg::NodeCallback
{
public:
  NodeBase(const osg::Vec3d &sourceIn, const osg::Vec3d &targetIn, const float &timeSpanIn);
  osg::Vec3d getSource(){return source;}
  osg::Vec3d getTarget(){return target;}
  bool isFinished(){return finished;}
  virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);
  virtual void setCurrent(const osg::Vec3d &currentIn) = 0;
  
protected:
  osg::Vec3d source;
  osg::Vec3d target;
  osg::Vec3d direction;
  float timeSpan;
  double length;
  bool finished;
  float lastTime;
  osg::ref_ptr<osgAnimation::InOutCubicMotion> motion;
  friend std::ostream& ::operator<<(std::ostream& os, const gsn::NodeBase& obj);
};

class NodeExpand : public NodeBase
{
public:
  NodeExpand(const osg::Vec3d &sourceIn, const osg::Vec3d &targetIn, const float &timeSpanIn);
  virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);
  virtual void setCurrent(const osg::Vec3d &currentIn);
};

class NodeCollapse : public NodeBase
{
public:
  NodeCollapse(const osg::Vec3d &sourceIn, const osg::Vec3d &targetIn, const float &timeSpanIn);
  virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);
  virtual void setCurrent(const osg::Vec3d &currentIn);
};

class GeometryBase : public osg::Drawable::UpdateCallback
{
public:
  GeometryBase(const osg::Vec3d &sourceIn, const osg::Vec3d &targetIn, const float &timeSpanIn);
  osg::Vec3d getSource(){return source;}
  osg::Vec3d getTarget(){return target;}
  bool isFinished(){return finished;}
  virtual void update(osg::NodeVisitor *visitor, osg::Drawable *drawable);
  virtual void setCurrent(const osg::Vec3d &currentIn) = 0;
  
protected:
  osg::Vec3d source;
  osg::Vec3d target;
  osg::Vec3d direction;
  osg::Vec3d normal; //90 deg to direction
  double lineWidth = 2.0; //pref? expose?
  float timeSpan;
  double length;
  bool finished;
  float lastTime;
  osg::ref_ptr<osgAnimation::InOutCubicMotion> motion;
};

class GeometryExpand : public GeometryBase
{
public:
  GeometryExpand(const osg::Vec3d &sourceIn, const osg::Vec3d &targetIn, const float &timeSpanIn);
  virtual void update(osg::NodeVisitor *visitor, osg::Drawable *drawable);
  virtual void setCurrent(const osg::Vec3d &currentIn);
};

class GeometryCollapse : public GeometryBase
{
public:
  GeometryCollapse(const osg::Vec3d &sourceIn, const osg::Vec3d &targetIn, const float &timeSpanIn);
  virtual void update(osg::NodeVisitor *visitor, osg::Drawable *drawable);
  virtual void setCurrent(const osg::Vec3d &currentIn);
};

class FadeState : public osg::StateAttribute::Callback
{
public:
  enum Fade{In, Out};
  FadeState(const float &timeSpanIn, const Fade &fadeSignal);
  virtual void operator()(osg::StateAttribute *attribute, osg::NodeVisitor *visitor);
protected:
  osg::ref_ptr<osgAnimation::InOutCubicMotion> motion;
  float lastTime;
  bool finished;
  Fade fade;
};

}


#endif // GSN_ANIMATIONS_H

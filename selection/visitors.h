/*
 * Copyright 2015 <copyright holder> <email>
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 */

#ifndef VISITORS_H
#define VISITORS_H

#include <boost/uuid/uuid.hpp>

#include <osg/NodeVisitor>

namespace mdv{class ShapeGeometry;}

namespace slc
{
  class MainSwitchVisitor : public osg::NodeVisitor
  {
  public:
    MainSwitchVisitor(const boost::uuids::uuid &idIn);
    virtual void apply(osg::Switch &switchIn) override;
    osg::Switch *out = nullptr;
    
  protected:
    const boost::uuids::uuid &id;
  };

  class ParentMaskVisitor : public osg::NodeVisitor
  {
  public:
    ParentMaskVisitor(std::size_t maskIn);
    virtual void apply(osg::Node &nodeIn) override;
    osg::Node *out = nullptr;
    
  protected:
    std::size_t mask;
  };

  class HighlightVisitor : public osg::NodeVisitor
  {
  public:
    enum class Operation
    {
      None = 0,		//!< not set. Error
      PreHighlight,	//!< color ids with prehighight color stored in shapegeometry
      Highlight,		//!< color ids with highlight color stored in shapegeometry
      Restore		//!< restore the color stored in the shapegeometry
    };
    HighlightVisitor(const std::vector<boost::uuids::uuid> &idsIn, Operation operationIn);
    virtual void apply(osg::Geometry &geometryIn) override;
  protected:
    const std::vector<boost::uuids::uuid> &ids;
    Operation operation;
    void setPreHighlight(mdv::ShapeGeometry *sGeometryIn);
    void setHighlight(mdv::ShapeGeometry *sGeometryIn);
    void setRestore(mdv::ShapeGeometry *sGeometryIn);
  }; 
}

#endif // VISITORS_H

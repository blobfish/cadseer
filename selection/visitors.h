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

#ifndef SLC_VISITORS_H
#define SLC_VISITORS_H

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
  
  template<typename T>
  class TypedAccrueVisitor : public osg::NodeVisitor
  {
  public:
    TypedAccrueVisitor(std::vector<T*> &collectionIn) :
    NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), collection(collectionIn)
    {
    }
    virtual void apply(osg::Node &nodeIn) override
    {
      T* ptr = dynamic_cast<T*>(&nodeIn);
      if (ptr)
        collection.push_back(ptr);
      traverse(nodeIn);
    }
    
  protected:
    std::vector<T*> &collection;
  };
  
  class PLODPathVisitor : public osg::NodeVisitor
  {
  public:
    PLODPathVisitor(const std::string &pathIn);
    virtual void apply(osg::PagedLOD&) override;
    
  protected:
    const std::string &path;
  };
  
  class PLODIdVisitor : public osg::NodeVisitor
  {
  public:
    PLODIdVisitor(const boost::uuids::uuid&);
    virtual void apply(osg::PagedLOD&) override;
    virtual void apply(osg::Switch&) override;
    osg::PagedLOD *out = nullptr;
    
  protected:
    const boost::uuids::uuid &id;
  };
}

#endif // SLC_VISITORS_H

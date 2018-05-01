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

#include <cassert>

#include <osg/Switch>
#include <osg/ValueObject>
#include <osg/PagedLOD>

#include <globalutilities.h>
#include <tools/idtools.h>
#include <modelviz/nodemaskdefs.h>
#include <modelviz/shapegeometry.h>
#include <selection/visitors.h>

using namespace slc;
using boost::uuids::uuid;

MainSwitchVisitor::MainSwitchVisitor(const uuid& idIn):
  NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), id(idIn)
{

}

void MainSwitchVisitor::apply(osg::Switch& switchIn)
{
  std::string userValue;
  if (switchIn.getUserValue<std::string>(gu::idAttributeTitle, userValue))
  {
    boost::uuids::uuid switchId = gu::stringToId(userValue);
    if (switchId == id)
    {
      out = &switchIn;
      return; //no need to continue search.
    }
  }

  traverse(switchIn);
}

ParentMaskVisitor::ParentMaskVisitor(std::size_t maskIn):
  NodeVisitor(osg::NodeVisitor::TRAVERSE_PARENTS), mask(maskIn)
{

}

void ParentMaskVisitor::apply(osg::Node& nodeIn)
{
  if(nodeIn.getNodeMask() == mask)
  {
    out = &nodeIn;
    return; //no need to continue search.
  }
  traverse(nodeIn);
}

HighlightVisitor::HighlightVisitor(const std::vector< uuid >& idsIn, HighlightVisitor::Operation operationIn):
  NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), ids(idsIn), operation(operationIn)
{
  //possible run time speed up and compile time slowdown:
  //setup a std::funciton dispatcher here and eliminate if/then/else tree in apply.
}

void HighlightVisitor::apply(osg::Geometry& geometryIn)
{
  assert (operation != Operation::None);
  
  if (ids.empty())
  {
    mdv::Base *base = dynamic_cast<mdv::Base*>(&geometryIn);
    if (base)
    {
      if (operation == Operation::PreHighlight)
        base->setToPreHighlight();
      else if(operation == Operation::Highlight)
        base->setToHighlight();
      else //has to equal restore.
        base->setToColor();
    }
  }
  else
  {
    mdv::ShapeGeometry *sGeometry = dynamic_cast<mdv::ShapeGeometry*>(&geometryIn);
    if (!sGeometry)
      return;
    if (operation == Operation::PreHighlight)
    {
      for (const auto &id : ids)
        sGeometry->setToPreHighlight(id);
    }
    else if(operation == Operation::Highlight)
    {
      for (const auto &id : ids)
        sGeometry->setToHighlight(id);
    }
    else //has to equal restore.
    {
      for (const auto &id : ids)
        sGeometry->setToColor(id);
    }
  }
  
  //don't need to call traverse because geometry should be 'end of line'
}

PLODPathVisitor::PLODPathVisitor(const std::string &pathIn) : 
NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
path(pathIn){}
void PLODPathVisitor::apply(osg::PagedLOD &lodIn)
{
  lodIn.setDatabasePath(path);
  traverse(lodIn);
}

PLODIdVisitor::PLODIdVisitor(const boost::uuids::uuid &idIn) :
NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
id(idIn){}
void PLODIdVisitor::apply(osg::Switch &switchIn)
{
  //this filters the traversal to only feature id.
  if (switchIn.getNodeMask() != mdv::object)
    return;
  std::string uValue;
  switchIn.getUserValue<std::string>(gu::idAttributeTitle, uValue);
  assert(!uValue.empty());
  boost::uuids::uuid nodeId = gu::stringToId(uValue);
  if (nodeId != id)
    return;
  traverse(switchIn);
}
void PLODIdVisitor::apply(osg::PagedLOD &lodIn)
{
  assert(out == nullptr); //how can we ever get here twice?
  out = &lodIn;
  //shouldn't need to traverse deeper.
}

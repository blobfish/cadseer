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

#include <cassert>

#include <boost/uuid/string_generator.hpp>

#include <osg/Switch>
#include <osg/ValueObject>

#include <globalutilities.h>
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
  if (switchIn.getUserValue(GU::idAttributeTitle, userValue))
  {
    boost::uuids::uuid switchId = boost::uuids::string_generator()(userValue);
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
  mdv::ShapeGeometry *sGeometry = dynamic_cast<mdv::ShapeGeometry*>(&geometryIn);
  if (!sGeometry)
    return;
  assert (operation != Operation::None);
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
  
  //don't need to call traverse because geometry should be 'end of line'
}

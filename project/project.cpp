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

#include <assert.h>
#include <iostream>

#include <boost/graph/topological_sort.hpp>

#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <TopoDS_Iterator.hxx>

#include <osg/ValueObject>

#include "../globalutilities.h"
#include "../modelviz/graph.h"
#include "../modelviz/connector.h"
#include "../feature/base.h"
#include "../feature/inert.h"
#include "project.h"

using namespace ProjectGraph;
using boost::uuids::uuid;

Project::Project()
{
}

void Project::update()
{
  writeGraphViz("/home/tanderson/temp/cadseer.dot");
  
  Path sorted;
  try
  {
    boost::topological_sort(projectGraph, std::back_inserter(sorted));
  }
  catch(const boost::not_a_dag &)
  {
    std::cout << std::endl << "Graph is not a dag exception in Project::update()" << std::endl << std::endl;
    return;
  }
  
  //the update of a feature will trigger a state change signal.
  //we don't want to handle that state change here in the project
  //so we block.
  std::vector<boost::signals2::shared_connection_block> blockVector;
  BGL_FORALL_VERTICES(currentVertex, projectGraph, Graph)
  {
    boost::signals2::shared_connection_block currentBlock(projectGraph[currentVertex].connection);
    blockVector.push_back(currentBlock);
  }
  
  //loop through and update each feature.
  for (auto it = sorted.rbegin(); it != sorted.rend(); ++it)
  {
    Vertex currentVertex = *it;
    if (projectGraph[currentVertex].feature->isModelClean())
      continue;
    
    Feature::UpdateMap updateMap;
    InEdgeIterator inEdgeIt, inEdgeItDone;
    boost::tie(inEdgeIt, inEdgeItDone) = boost::in_edges(currentVertex, projectGraph);
    for(;inEdgeIt != inEdgeItDone; ++inEdgeIt)
    {
      Vertex source = boost::source(*inEdgeIt, projectGraph);
      updateMap.insert(std::make_pair(projectGraph[*inEdgeIt].inputType, projectGraph[source].feature.get()));
    }
    projectGraph[currentVertex].feature->update(updateMap);
  }
  
  projectUpdatedSignal();
}

void Project::updateVisual()
{
  Path sorted;
  try
  {
    boost::topological_sort(projectGraph, std::back_inserter(sorted));
  }
  catch(const boost::not_a_dag &)
  {
    std::cout << std::endl << "Graph is not a dag exception in Project::update()" << std::endl << std::endl;
    return;
  }
  
  for(auto it = sorted.rbegin(); it != sorted.rend(); ++it)
  {
    auto feature = projectGraph[*it].feature;
    if
    (
      feature->isVisible3D() &&
      feature->isActive() &&
      feature->isSuccess() &&
      feature->isVisualDirty()
    )
      feature->updateVisual();
  }
}

void Project::writeGraphViz(const std::string& fileName)
{
  outputGraphviz(projectGraph, fileName);
}


void Project::readOCC(const std::string &fileName, osg::Group *root)
{
    TopoDS_Shape base;
    BRep_Builder junk;
    std::fstream file(fileName.c_str());
    BRepTools::Read(base, file, junk);

    //structure understood to be a compound of compounds.
    //check base is a compound.
    if (base.ShapeType() != TopAbs_COMPOUND)
    {
        std::cout << "expected base compound in reading of OCC" << std::endl;
        return;
    }

    TopoDS_Iterator it;
    for (it.Initialize(base); it.More(); it.Next())
    {
        TopoDS_Shape current = it.Value();
        if (current.ShapeType() != TopAbs_COMPOUND)
        {
            std::cout << "expected compound in reading of OCC. Got " << current.ShapeType() << std::endl;
//            continue;
        }

        addOCCShape(current, root);
    }
}

Feature::Base* Project::findFeature(const uuid &idIn)
{
  return projectGraph[findVertex(idIn)].feature.get();
}

ProjectGraph::Vertex Project::findVertex(const uuid& idIn)
{
  IdVertexMap::const_iterator it;
  it = map.find(idIn);
  assert(it != map.end());
  return it->second;
}

void Project::addOCCShape(const TopoDS_Shape &shapeIn, osg::Group *root)
{
  std::shared_ptr<Feature::Inert> inert(new Feature::Inert(shapeIn));
  addFeature(inert, root);
}

void Project::addFeature(std::shared_ptr<Feature::Base> feature, osg::Group *root)
{
  Vertex newVertex = boost::add_vertex(projectGraph);
  projectGraph[newVertex].feature = feature;
  map.insert(std::make_pair(feature->getId(), newVertex));
  featureAddedSignal(feature);
  
  root->addChild(feature->getMainSwitch());
  
  projectGraph[newVertex].connection = feature->connectState(boost::bind(&Project::stateChangedSlot, this, _1, _2));
}

void Project::connect(const boost::uuids::uuid& parentIn, const boost::uuids::uuid& childIn, Feature::InputTypes type)
{
  Vertex parent = map.at(parentIn);
  Vertex child = map.at(childIn);
  Edge edge = connectVertices(parent, child, type);
  projectGraph[edge].inputType = type;
  
  connectionAddedSignal(parentIn, childIn, type);
}

void Project::stateChangedSlot(const uuid& featureIdIn, std::size_t stateIn)
{
  if (stateIn != Feature::StateOffset::ModelDirty)
    return;
  
  //the visitor will be setting features to a dirty state which
  //trigger the signal and we would end up back in this function recursively.
  //so we block all the connections to avoid this recursion.
  std::vector<boost::signals2::shared_connection_block> blockVector;
  BGL_FORALL_VERTICES(currentVertex, projectGraph, Graph)
  {
    boost::signals2::shared_connection_block currentBlock(projectGraph[currentVertex].connection);
    blockVector.push_back(currentBlock);
  }
  
  ProjectGraph::Vertex vertex = findVertex(featureIdIn);
  SetDirtyVisitor visitor;
  boost::breadth_first_search(projectGraph, vertex, boost::visitor(visitor));
}

Edge Project::connectVertices(Vertex parent, Vertex child, Feature::InputTypes type)
{
  bool results;
  Edge newEdge;
  boost::tie(newEdge, results) = boost::add_edge(parent, child, projectGraph);
  assert(results);
  projectGraph[newEdge].inputType = type;
  return newEdge;
}

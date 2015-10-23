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
#include <boost/graph/filtered_graph.hpp>

#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <TopoDS_Iterator.hxx>

#include <osg/ValueObject>

#include "../globalutilities.h"
#include "../modelviz/graph.h"
#include "../modelviz/connector.h"
#include "../feature/base.h"
#include "../feature/inert.h"
#include <project/message.h>
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
  
  updateLeafStatus();
  projectUpdatedSignal();
}

void Project::updateVisual()
{
  Path sorted;
  try
  {
    indexVerticesEdges();
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
  indexVerticesEdges();
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

void Project::removeFeature(const uuid& idIn, osg::Group *root)
{
  Vertex vertex = findVertex(idIn);
  std::shared_ptr<Feature::Base> feature = projectGraph[vertex].feature;
  feature->setModelDirty(); //this will make all children dirty.
  projectGraph[vertex].connection.disconnect();
  
  VertexEdgePairs parents = getParents(vertex);
  VertexEdgePairs children = getChildren(vertex);
  //for now all children get connected to target parent.
  if (!parents.empty())
  {
    Vertex targetParent = boost::graph_traits<Graph>::null_vertex();
    for (const auto &current : parents)
    {
      if (projectGraph[current.second].inputType == Feature::InputTypes::target)
      {
	targetParent = current.first;
	break;
      }
    }
    assert(targetParent != boost::graph_traits<Graph>::null_vertex());
    for (const auto &current : children)
      connect(targetParent, current.first, projectGraph[current.second].inputType);
  }
  
  for (const auto &current : parents)
    connectionRemovedSignal(projectGraph[current.first].feature->getId(), idIn, projectGraph[current.second].inputType);
  
  for (const auto &current : children)
    connectionRemovedSignal(idIn, projectGraph[current.first].feature->getId(), projectGraph[current.second].inputType);
  
  root->removeChild(feature->getMainSwitch());
  featureRemovedSignal(feature);
  
  boost::clear_vertex(vertex, projectGraph);
  boost::remove_vertex(vertex, projectGraph);
}

void Project::setFeatureActive(const uuid& idIn)
{
  indexVerticesEdges();
  
  //the visitor will be setting features to an inactive state which
  //triggers the signal and we would end up back into this->stateChangedSlot.
  //so we block all the connections to avoid this.
  std::vector<boost::signals2::shared_connection_block> blockVector;
  BGL_FORALL_VERTICES(currentVertex, projectGraph, Graph)
  {
    boost::signals2::shared_connection_block currentBlock(projectGraph[currentVertex].connection);
    blockVector.push_back(currentBlock);
  }
  
  ProjectGraph::Vertex vertex = findVertex(idIn);
  
  GraphReversed rGraph = boost::make_reverse_graph(projectGraph);
  
  //parents
  SetActiveVisitor activeVisitorParents;
  boost::breadth_first_search(rGraph, vertex, boost::visitor(activeVisitorParents));
  SetHiddenVisitor hideVisitorParents;
  boost::breadth_first_search(rGraph, vertex, boost::visitor(hideVisitorParents));
  
  //children
  SetInactiveVisitor inactiveVisitor;
  boost::breadth_first_search(projectGraph, vertex, boost::visitor(inactiveVisitor));
  SetHiddenVisitor hideVisitor;
  boost::breadth_first_search(projectGraph, vertex, boost::visitor(hideVisitor));
  
  //now we don't want the actual feature inactive just it's children so we
  //turn it back on because the BFS and visitor turned it off.
  projectGraph[vertex].feature->setActive();
  projectGraph[vertex].feature->show3D();
  
  updateLeafStatus();
}

void Project::updateLeafStatus()
{
  indexVerticesEdges(); //redundent for setFeatureActive call.
  
  //first set all features to non leaf.
  BGL_FORALL_VERTICES(currentVertex, projectGraph, Graph)
  {
    projectGraph[currentVertex].feature->setNonLeaf();
  }
  
  ActiveFilter <Graph> activeFilter(projectGraph);
  
  typedef boost::filtered_graph<Graph, boost::keep_all, ActiveFilter<Graph> > FilteredGraphType;
  typedef boost::graph_traits<FilteredGraphType>::vertex_iterator FilteredVertexIterator;
  
  FilteredGraphType filteredGraph(projectGraph, boost::keep_all(), activeFilter);
  
  FilteredVertexIterator vIt, vItEnd;
  boost::tie(vIt, vItEnd) = boost::vertices(filteredGraph);
  for (; vIt != vItEnd; ++vIt)
  {
    if (boost::out_degree(*vIt, filteredGraph) == 0)
      filteredGraph[*vIt].feature->setLeaf();
  }
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
  indexVerticesEdges();
  
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

void Project::messageInSlot(const ProjectSpace::Message& messageIn)
{
  //block connection back to project.
  std::vector<boost::signals2::shared_connection_block> blockVector;
  BGL_FORALL_VERTICES(currentVertex, projectGraph, Graph)
  {
    boost::signals2::shared_connection_block currentBlock(projectGraph[currentVertex].connection);
    blockVector.push_back(currentBlock);
  }
  if 
  (
    (messageIn.type == ProjectSpace::Message::Type::Request) &&
    (messageIn.action == ProjectSpace::Message::Action::SetCurrentLeaf)
  )
  {
    setFeatureActive(messageIn.featureId);
  }
  if 
  (
    (messageIn.type == ProjectSpace::Message::Type::Request) &&
    (messageIn.action == ProjectSpace::Message::Action::RemoveFeature)
  )
  {
    //hold off until message system is in place.
    std::cout << "inside: " << __PRETTY_FUNCTION__ << "  for remove feature" << std::endl;
//     removeFeature(messageIn.featureId);
  }
}

void Project::indexVerticesEdges()
{
  std::size_t index = 0;
  
  //index vertices.
  BGL_FORALL_VERTICES(currentVertex, projectGraph, Graph)
  {
    boost::put(boost::vertex_index, projectGraph, currentVertex, index);
    index++;
  }

  //index edges.
  index = 0;
  BGL_FORALL_EDGES(currentEdge, projectGraph, Graph)
  {
    boost::put(boost::edge_index, projectGraph, currentEdge, index);
    index++;
  }
}

Edge Project::connect(Vertex parentIn, Vertex childIn, Feature::InputTypes type)
{
  Edge edge = connectVertices(parentIn, childIn, type);
  projectGraph[edge].inputType = type;
  
  connectionAddedSignal(projectGraph[parentIn].feature->getId(), projectGraph[childIn].feature->getId(), type);
  return edge;
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

void Project::setAllVisualDirty()
{
  BGL_FORALL_VERTICES(currentVertex, projectGraph, Graph)
  {
    projectGraph[currentVertex].feature->setVisualDirty();
  }
}

Project::VertexEdgePairs Project::getParents(ProjectGraph::Vertex vertexIn)
{
  VertexEdgePairs out;
  InEdgeIterator inEdgeIt, inEdgeItDone;
  boost::tie(inEdgeIt, inEdgeItDone) = boost::in_edges(vertexIn, projectGraph);
  for (; inEdgeIt != inEdgeItDone; ++inEdgeIt)
  {
    VertexEdgePair tempPair;
    tempPair.first = boost::source(*inEdgeIt, projectGraph);
    tempPair.second = *inEdgeIt;
    out.push_back(tempPair);
  }
  return out;
}

Project::VertexEdgePairs Project::getChildren(ProjectGraph::Vertex vertexIn)
{
  VertexEdgePairs out;
  OutEdgeIterator outEdgeIt, outEdgeItDone;
  boost::tie(outEdgeIt, outEdgeItDone) = boost::out_edges(vertexIn, projectGraph);
  for (; outEdgeIt != outEdgeItDone; ++outEdgeIt)
  {
    VertexEdgePair tempPair;
    tempPair.first = boost::target(*outEdgeIt, projectGraph);
    tempPair.second = *outEdgeIt;
    out.push_back(tempPair);
  }
  return out;
}



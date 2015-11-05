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

#include <application/application.h>
#include <globalutilities.h>
#include <modelviz/graph.h>
#include <modelviz/connector.h>
#include <feature/base.h>
#include <feature/inert.h>
#include <project/message.h>
#include <project/project.h>
#include <message/message.h>
#include <message/dispatch.h>

using namespace prj;
using namespace prg;
using boost::uuids::uuid;

Project::Project()
{
  setupDispatcher();
}

void Project::updateModel()
{
  msg::Message preMessage;
  preMessage.mask = msg::Response | msg::Pre | msg::UpdateModel;
  messageOutSignal(preMessage);
  
  //something here to check preferences about writing this out.
  QString fileName = static_cast<app::Application *>(qApp)->getApplicationDirectory().path();
  fileName += QDir::separator();
  fileName += "project.dot";
  writeGraphViz(fileName.toStdString().c_str());
  
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
    
    ftr::UpdateMap updateMap;
    InEdgeIterator inEdgeIt, inEdgeItDone;
    boost::tie(inEdgeIt, inEdgeItDone) = boost::in_edges(currentVertex, projectGraph);
    for(;inEdgeIt != inEdgeItDone; ++inEdgeIt)
    {
      Vertex source = boost::source(*inEdgeIt, projectGraph);
      updateMap.insert(std::make_pair(projectGraph[*inEdgeIt].inputType, projectGraph[source].feature.get()));
    }
    projectGraph[currentVertex].feature->updateModel(updateMap);
  }
  
  updateLeafStatus();
  
  msg::Message postMessage;
  postMessage.mask = msg::Response | msg::Post | msg::UpdateModel;
  messageOutSignal(postMessage);
}

void Project::updateVisual()
{
  msg::Message preMessage;
  preMessage.mask = msg::Response | msg::Pre | msg::UpdateVisual;
  messageOutSignal(preMessage);
  
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
  
  msg::Message postMessage;
  postMessage.mask = msg::Response | msg::Post | msg::UpdateVisual;
  messageOutSignal(postMessage);
}

void Project::writeGraphViz(const std::string& fileName)
{
  indexVerticesEdges();
  outputGraphviz(projectGraph, fileName);
}


void Project::readOCC(const std::string &fileName)
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

        addOCCShape(current);
    }
}

ftr::Base* Project::findFeature(const uuid &idIn)
{
  return projectGraph[findVertex(idIn)].feature.get();
}

prg::Vertex Project::findVertex(const uuid& idIn)
{
  IdVertexMap::const_iterator it;
  it = map.find(idIn);
  assert(it != map.end());
  return it->second;
}

void Project::addOCCShape(const TopoDS_Shape &shapeIn)
{
  std::shared_ptr<ftr::Inert> inert(new ftr::Inert(shapeIn));
  addFeature(inert);
}

void Project::addFeature(std::shared_ptr<ftr::Base> feature)
{
  //no pre message.
  
  Vertex newVertex = boost::add_vertex(projectGraph);
  projectGraph[newVertex].feature = feature;
  map.insert(std::make_pair(feature->getId(), newVertex));
  msg::Message postMessage;
  postMessage.mask = msg::Response | msg::Post | msg::AddFeature;
  prj::Message pMessage;
  pMessage.feature = feature;
  postMessage.payload = pMessage;
  messageOutSignal(postMessage);
  
  projectGraph[newVertex].connection = feature->connectState(boost::bind(&Project::stateChangedSlot, this, _1, _2));
}

void Project::removeFeature(const uuid& idIn)
{
  Vertex vertex = findVertex(idIn);
  std::shared_ptr<ftr::Base> feature = projectGraph[vertex].feature;
  
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
      if (projectGraph[current.second].inputType == ftr::InputTypes::target)
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
  {
    msg::Message preMessage;
    preMessage.mask = msg::Response | msg::Pre | msg::RemoveConnection;
    prj::Message pMessage;
    pMessage.featureId = projectGraph[current.first].feature->getId();
    pMessage.featureId2 = idIn;
    pMessage.inputType = projectGraph[current.second].inputType;
    preMessage.payload = pMessage;
    messageOutSignal(preMessage);
  }
  
  for (const auto &current : children)
  {
    msg::Message preMessage;
    preMessage.mask = msg::Response | msg::Pre | msg::RemoveConnection;
    prj::Message pMessage;
    pMessage.featureId = idIn;
    pMessage.featureId2 = projectGraph[current.first].feature->getId();
    pMessage.inputType = projectGraph[current.second].inputType;
    preMessage.payload = pMessage;
    messageOutSignal(preMessage);
  }
  
  msg::Message preMessage;
  preMessage.mask = msg::Response | msg::Pre | msg::RemoveFeature;
  prj::Message pMessage;
  pMessage.feature = feature;
  preMessage.payload = pMessage;
  messageOutSignal(preMessage);
  
  boost::clear_vertex(vertex, projectGraph);
  boost::remove_vertex(vertex, projectGraph);
  
  //no post message.
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
  
  prg::Vertex vertex = findVertex(idIn);
  
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

void Project::connect(const boost::uuids::uuid& parentIn, const boost::uuids::uuid& childIn, ftr::InputTypes type)
{
  Vertex parent = map.at(parentIn);
  Vertex child = map.at(childIn);
  Edge edge = connectVertices(parent, child, type);
  projectGraph[edge].inputType = type;
  
  msg::Message postMessage;
  postMessage.mask = msg::Response | msg::Post | msg::AddConnection;
  prj::Message pMessage;
  pMessage.featureId = parentIn;
  pMessage.featureId2 = childIn; 
  pMessage.inputType = type;
  postMessage.payload = pMessage;
  messageOutSignal(postMessage);
}

void Project::stateChangedSlot(const uuid& featureIdIn, std::size_t stateIn)
{
  if (stateIn != ftr::StateOffset::ModelDirty)
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
  
  prg::Vertex vertex = findVertex(featureIdIn);
  SetDirtyVisitor visitor;
  boost::breadth_first_search(projectGraph, vertex, boost::visitor(visitor));
}

void Project::messageInSlot(const msg::Message &messageIn)
{
//   std::cout << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::MessageDispatcher::iterator it = dispatcher.find(messageIn.mask);
  if (it == dispatcher.end())
    return;
  
  //block the signals back into project. should this be done here
  //or should this be done in dispatched function on a case by case basis?
  //to be determined.
  std::vector<boost::signals2::shared_connection_block> blockVector;
  BGL_FORALL_VERTICES(currentVertex, projectGraph, Graph)
  {
    boost::signals2::shared_connection_block currentBlock(projectGraph[currentVertex].connection);
    blockVector.push_back(currentBlock);
  }
  
  it->second(messageIn);
}

void Project::setupDispatcher()
{
  msg::Mask mask;
  
  mask = msg::Request | msg::SetCurrentLeaf;
  dispatcher.insert(std::make_pair(mask, boost::bind(&Project::setCurrentLeafDispatched, this, _1)));
  
  mask = msg::Request | msg::RemoveFeature;
  dispatcher.insert(std::make_pair(mask, boost::bind(&Project::removeFeatureDispatched, this, _1)));
  
  mask = msg::Request | msg::UpdateModel;
  dispatcher.insert(std::make_pair(mask, boost::bind(&Project::updateModelDispatched, this, _1)));
  
  mask = msg::Request | msg::UpdateVisual;
  dispatcher.insert(std::make_pair(mask, boost::bind(&Project::updateVisualDispatched, this, _1)));
}

void Project::setCurrentLeafDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  prj::Message message = boost::get<prj::Message>(messageIn.payload);
  //send response signal out 'pre set current feature'.
  setFeatureActive(message.featureId);
  //send response signal out 'post set current feature'.
}

void Project::removeFeatureDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  prj::Message message = boost::get<prj::Message>(messageIn.payload);
  removeFeature(message.featureId);
}

void Project::updateModelDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  updateModel();
}

void Project::updateVisualDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  updateVisual();
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

Edge Project::connect(Vertex parentIn, Vertex childIn, ftr::InputTypes type)
{
  Edge edge = connectVertices(parentIn, childIn, type);
  
  msg::Message postMessage;
  postMessage.mask = msg::Response | msg::Post | msg::AddConnection;
  prj::Message pMessage;
  pMessage.featureId = projectGraph[parentIn].feature->getId();
  pMessage.featureId2 = projectGraph[childIn].feature->getId(); 
  pMessage.inputType = type;
  postMessage.payload = pMessage;
  messageOutSignal(postMessage);
  
  return edge;
}

Edge Project::connectVertices(Vertex parent, Vertex child, ftr::InputTypes type)
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

Project::VertexEdgePairs Project::getParents(prg::Vertex vertexIn)
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

Project::VertexEdgePairs Project::getChildren(prg::Vertex vertexIn)
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



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
#include <stack>

#include <QTextStream>

#include <boost/graph/topological_sort.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/depth_first_search.hpp>

#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <TopoDS_Iterator.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>

#include <osg/ValueObject>

#include <application/application.h>
#include <globalutilities.h>
#include <tools/idtools.h>
#include <feature/base.h>
#include <feature/seershape.h>
#include <feature/inert.h>
#include <expressions/expressionmanager.h>
#include <expressions/stringtranslator.h> //for serialize.
#include <project/message.h>
#include <message/message.h>
#include <message/dispatch.h>
#include <message/observer.h>
#include <project/gitmanager.h>
#include <project/featureload.h>
#include <project/serial/xsdcxxoutput/project.h>
#include <project/project.h>
#include <tools/graphtools.h>

using namespace prj;
using namespace prg;
using boost::uuids::uuid;

Project::Project()
{
  observer = std::move(std::unique_ptr<msg::Observer>(new msg::Observer()));
  observer->name = "prj::Project";
  setupDispatcher();
  
  std::unique_ptr<GitManager> tempManager(new GitManager());
  gitManager = std::move(tempManager);
  
  std::unique_ptr<expr::ExpressionManager> tempEManager(new expr::ExpressionManager());
  expressionManager = std::move(tempEManager);
}

Project::~Project()
{
}

QTextStream& Project::getInfo(QTextStream &stream) const
{
  stream
  << QObject::tr("Project Directory: ") << QString::fromStdString(getSaveDirectory()) << endl;
  //maybe some git stuff.
  
  expressionManager->getInfo(stream);
  
  return stream;
}

void Project::updateModel()
{
  observer->out(msg::Message(msg::Response | msg::Pre | msg::UpdateModel));
  
  expressionManager->update();
  
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
  
  //the update of a feature will trigger a state change signal.
  //we don't want to handle that state change here in the project
  //so we block.
  auto block = observer->createBlocker();
  
  //loop through and update each feature.
  for (auto it = sorted.rbegin(); it != sorted.rend(); ++it)
  {
    Vertex currentVertex = *it;
    if
    (
      (projectGraph[currentVertex].feature->isModelClean()) ||
      (projectGraph[currentVertex].feature->isInactive())
    )
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
    projectGraph[currentVertex].feature->serialWrite(QDir(QString::fromStdString(saveDirectory)));
  }
  
  serialWrite();
  gitManager->update();
  
  updateLeafStatus();
  
  observer->out(msg::Message(msg::Response | msg::Post | msg::UpdateModel));
}

void Project::updateVisual()
{
  //if we have selection and then destroy the geometry when the
  //the visual updates, things get out of sync. so clear the selection.
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  observer->out(msg::Message(msg::Response | msg::Pre | msg::UpdateVisual));
  
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
  
  auto block = observer->createBlocker();
  
  for(auto it = sorted.rbegin(); it != sorted.rend(); ++it)
  {
    auto feature = projectGraph[*it].feature;
    if
    (
      feature->isModelClean() &&
      feature->isVisible3D() &&
      feature->isActive() &&
//       feature->isSuccess() && //regenerate from parent shape on failure.
      feature->isVisualDirty()
    )
      feature->updateVisual();
  }
  
  observer->out(msg::Message(msg::Response | msg::Post | msg::UpdateVisual));
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

ftr::Parameter* Project::findParameter(const uuid &idIn) const
{
  BGL_FORALL_VERTICES(currentVertex, projectGraph, Graph)
  {
    if (!projectGraph[currentVertex].feature->hasParameter(idIn))
      continue;
    return projectGraph[currentVertex].feature->getParameter(idIn);
  }
  assert(0); //no such parameter.
  return nullptr;
}

prg::Vertex Project::findVertex(const uuid& idIn) const
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
  
  //log action to git if not loading.
  if (!isLoading)
  {
    std::ostringstream gitMessage;
    gitMessage << QObject::tr("Adding feature ").toStdString() << feature->getTypeString();
    gitManager->appendGitMessage(gitMessage.str());
  }
  
  msg::Message postMessage(msg::Response | msg::Post | msg::Add | msg::Feature);
  prj::Message pMessage;
  pMessage.feature = feature;
  postMessage.payload = pMessage;
  observer->out(postMessage);
}

void Project::removeFeature(const uuid& idIn)
{
  Vertex vertex = findVertex(idIn);
  std::shared_ptr<ftr::Base> feature = projectGraph[vertex].feature;
  
  feature->setModelDirty(); //this will make all children dirty.
  
  //shouldn't need anymore messages into project for this function call.
  auto block = observer->createBlocker();
  
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
    msg::Message preMessage(msg::Response | msg::Pre | msg::Remove | msg::Connection);
    prj::Message pMessage;
    pMessage.featureId = projectGraph[current.first].feature->getId();
    pMessage.featureId2 = idIn;
    pMessage.inputType = projectGraph[current.second].inputType;
    preMessage.payload = pMessage;
    observer->out(preMessage);
    
    //make parents have same visible state as the feature being removed.
    if (feature->isVisible3D())
      projectGraph[current.first].feature->show3D();
    else
      projectGraph[current.first].feature->hide3D();
  }
  
  for (const auto &current : children)
  {
    msg::Message preMessage(msg::Response | msg::Pre | msg::Remove | msg::Connection);
    prj::Message pMessage;
    pMessage.featureId = idIn;
    pMessage.featureId2 = projectGraph[current.first].feature->getId();
    pMessage.inputType = projectGraph[current.second].inputType;
    preMessage.payload = pMessage;
    observer->out(preMessage);
  }
  
  msg::Message preMessage(msg::Response | msg::Pre | msg::Remove | msg::Feature);
  prj::Message pMessage;
  pMessage.feature = feature;
  preMessage.payload = pMessage;
  observer->out(preMessage);
  
  //remove file if exists.
  QString fileName = QString::fromStdString(feature->getFileName());
  QDir dir = QString::fromStdString(saveDirectory);
  assert(dir.exists());
  if (dir.exists(fileName))
    dir.remove(fileName);
  
  boost::clear_vertex(vertex, projectGraph);
  boost::remove_vertex(vertex, projectGraph);
  
  //log action to git.
  std::ostringstream gitMessage;
  gitMessage << QObject::tr("Removing feature ").toStdString() << feature->getTypeString();
  gitManager->appendGitMessage(gitMessage.str());
  
  //no post message.
}

void Project::setCurrentLeaf(const uuid& idIn)
{
  indexVerticesEdges();
  
  //sometimes the visual of a feature is dirty and doesn't get updated 
  //until we try to show it. However it might be selected meaning that
  //we will destroy the old geometry that is highlighted. So clear the seleciton.
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  //the visitor will be setting features to an inactive state which
  //triggers the signal and we would end up back into this->stateChangedSlot.
  //so we block all the connections to avoid this.
  auto block = observer->createBlocker();
  
  prg::Vertex vertex = findVertex(idIn);
  
  GraphReversed rGraph = boost::make_reverse_graph(projectGraph);
  
  //parents
  SetActiveVisitor activeVisitorParents;
  boost::breadth_first_search(rGraph, vertex, boost::visitor(activeVisitorParents));
  SetHiddenVisitor hideVisitorParents;
  boost::breadth_first_search(rGraph, vertex, boost::visitor(hideVisitorParents));
  //if this is a create feature we don't want to hide the immediate parents.
  if (projectGraph[vertex].feature->getDescriptor() == ftr::Descriptor::Create)
  {
    GraphReversed::adjacency_iterator it, itEnd;
    boost::tie(it, itEnd) = boost::adjacent_vertices(vertex, rGraph);
    for (; it != itEnd; ++it)
    {
      projectGraph[*it].feature->setActive();
      projectGraph[*it].feature->show3D();
    }
  }
  
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
  indexVerticesEdges(); //redundent for setCurrentLeaf call.
  
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
    else
    {
      //if all children are of the type 'create' then it is also considered leaf.
      boost::graph_traits<FilteredGraphType>::adjacency_iterator vItNested, vItEndNested;
      boost::tie(vItNested, vItEndNested) = boost::adjacent_vertices(*vIt, filteredGraph);
      bool allCreate = true;
      for (; vItNested != vItEndNested; ++vItNested)
      {
        if (filteredGraph[*vItNested].feature->getDescriptor() != ftr::Descriptor::Create)
        {
        allCreate = false;
        break;
        }
      }
      if (allCreate)
        filteredGraph[*vIt].feature->setLeaf();
    }
  }
}

void Project::connect(const boost::uuids::uuid& parentIn, const boost::uuids::uuid& childIn, ftr::InputTypes type)
{
  Vertex parent = map.at(parentIn);
  Vertex child = map.at(childIn);
  Edge edge = connectVertices(parent, child, type);
  projectGraph[edge].inputType = type;
  
  msg::Message postMessage(msg::Response | msg::Post | msg::Add | msg::Connection);
  prj::Message pMessage;
  pMessage.featureId = parentIn;
  pMessage.featureId2 = childIn; 
  pMessage.inputType = type;
  postMessage.payload = pMessage;
  observer->out(postMessage);
}

void Project::setupDispatcher()
{
  msg::Mask mask;
  
  mask = msg::Request | msg::SetCurrentLeaf;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Project::setCurrentLeafDispatched, this, _1)));
  
  mask = msg::Request | msg::Remove | msg::Feature;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Project::removeFeatureDispatched, this, _1)));
  
  mask = msg::Request | msg::Update;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Project::updateDispatched, this, _1)));
  
  mask = msg::Request | msg::ForceUpdate;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Project::forceUpdateDispatched, this, _1)));
  
  mask = msg::Request | msg::UpdateModel;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Project::updateModelDispatched, this, _1)));
  
  mask = msg::Request | msg::UpdateVisual;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Project::updateVisualDispatched, this, _1)));
  
  mask = msg::Request | msg::SaveProject;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Project::saveProjectRequestDispatched, this, _1)));
  
  mask = msg::Request | msg::CheckShapeIds;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Project::checkShapeIdsDispatched, this, _1)));
  
  mask = msg::Response | msg::Feature | msg::Status;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Project::featureStateChangedDispatched, this, _1)));
  
  mask = msg::Request | msg::DebugDumpProjectGraph;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Project::dumpProjectGraphDispatched, this, _1)));
}

void Project::featureStateChangedDispatched(const msg::Message &messageIn)
{
  
  ftr::Message fMessage = boost::get<ftr::Message>(messageIn.payload);
  if
  (
    (fMessage.stateOffset != ftr::StateOffset::ModelDirty) ||
    (fMessage.freshValue != true)
  )
    return;
    
  //this code blocks all incoming messages to the project while it
  //executes. This prevents the cycles from setting a dependent dirty.
  indexVerticesEdges();
  
  auto block = observer->createBlocker();
  
  prg::Vertex vertex = findVertex(fMessage.featureId);
  SetDirtyVisitor visitor;
  boost::breadth_first_search(projectGraph, vertex, boost::visitor(visitor));
}

void Project::setCurrentLeafDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  prj::Message message = boost::get<prj::Message>(messageIn.payload);
  //send response signal out 'pre set current feature'.
    setCurrentLeaf(message.featureId);
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

void Project::updateDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  updateModel();
  updateVisual();
}

void Project::forceUpdateDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  BGL_FORALL_VERTICES(currentVertex, projectGraph, Graph)
  {
    projectGraph[currentVertex].feature->setModelDirty();
  }
  
  updateModel();
  updateVisual();
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

void Project::saveProjectRequestDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  save();
}

void Project::checkShapeIdsDispatched(const msg::Message&)
{
  /* initially shapeIds were being copied from parent feature to child feature.
   * decided to make each shape unique to feature and use evolution container
   * to map shapes between related features. This command visits graph and checks 
   * for duplicated ids, which we shouldn't have anymore.
   */
  
  using boost::uuids::uuid;
  
  typedef std::vector<uuid> FeaturesIds;
  typedef std::map<uuid, FeaturesIds> IdMap;
  IdMap idMap;
  
  BGL_FORALL_VERTICES(currentVertex, projectGraph, Graph)
  {
    const ftr::Base *feature = projectGraph[currentVertex].feature.get();
    if (!feature->hasSeerShape())
      continue;
    for (const auto &id : feature->getSeerShape().getAllShapeIds())
    {
      IdMap::iterator it = idMap.find(id);
      if (it == idMap.end())
      {
        std::vector<uuid> freshFeatureIds;
        freshFeatureIds.push_back(feature->getId());
        idMap.insert(std::make_pair(id, freshFeatureIds));
      }
      else
      {
        it->second.push_back(id);
      }
    }
  }
  
  std::cout << std::endl << std::endl << "Check shape ids:";
  
  bool foundDuplicate = false;
  for (const auto &entry : idMap)
  {
    if (entry.second.size() < 2)
      continue;
    foundDuplicate = true;
    std::cout << std::endl << "shape id of: " << gu::idToString(entry.first) << "    is in feature id of:";
    for (const auto &it : entry.second)
    {
      std::cout << " " << gu::idToString(it);
    }
  }
  
  if (!foundDuplicate)
    std::cout << std::endl << "No duplicate ids found. Test passed" << std::endl;
}

void Project::dumpProjectGraphDispatched(const msg::Message &)
{
  indexVerticesEdges();
  
  QString fileName = static_cast<app::Application *>(qApp)->getApplicationDirectory().path();
  fileName += QDir::separator();
  fileName += "project.dot";
  writeGraphViz(fileName.toStdString().c_str());
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
  
  msg::Message postMessage(msg::Response | msg::Post | msg::Add | msg::Connection);
  prj::Message pMessage;
  pMessage.featureId = projectGraph[parentIn].feature->getId();
  pMessage.featureId2 = projectGraph[childIn].feature->getId(); 
  pMessage.inputType = type;
  postMessage.payload = pMessage;
  observer->out(postMessage);
  
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

void Project::setColor(const boost::uuids::uuid &featureIdIn, const osg::Vec4 &colorIn)
{
  //the following is a good example of accumulating history of 1 object.
  
  //first remove any non 'target' edges. this will limit the following searches.
  TargetEdgeFilter<Graph> edgeFilter(projectGraph);
  typedef boost::filtered_graph<Graph, TargetEdgeFilter<Graph>, boost::keep_all> TargetFilteredGraph;
  TargetFilteredGraph tFilteredGraph(projectGraph, edgeFilter, boost::keep_all());
  
  //find forward connected vertices.
  Vertex baseVertex = findVertex(featureIdIn);
  std::vector<Vertex> vertexes; //note: name 'vertices' clashes with forall_vertices macro.
  gu::BFSLimitVisitor<Vertex> vis(vertexes);
  boost::breadth_first_search(tFilteredGraph, baseVertex, visitor(vis));
  
  //find reverse connected vertices.
  typedef boost::reverse_graph<TargetFilteredGraph, TargetFilteredGraph&> TFReversedGraph;
  TFReversedGraph rGraph = boost::make_reverse_graph(tFilteredGraph);
  gu::BFSLimitVisitor<VertexReversed> rVis(vertexes);
  boost::breadth_first_search(rGraph, baseVertex, visitor(rVis));
  
  //filter on the accumulated vertexes.
  gu::SubsetFilter<Graph> vertexFilter(projectGraph, vertexes);
  typedef boost::filtered_graph<Graph, boost::keep_all, gu::SubsetFilter<Graph> > FilteredGraph;
  FilteredGraph filteredGraph(projectGraph, boost::keep_all(), vertexFilter);
  
  //set color of all objects.
  BGL_FORALL_VERTICES(currentVertex, filteredGraph, FilteredGraph)
  {
    projectGraph[currentVertex].feature->setColor(colorIn);
    //this is a hack. Currently, in order for this color change to be serialized
    //at next update we would have to mark the feature dirty. Marking the feature dirty
    //and causing models to be recalculated seems excessive for such a minor change as
    //object color. So here we just serialize the changed features to 'sneak' the
    //color change into the git commit.
    projectGraph[currentVertex].feature->serialWrite(QDir(QString::fromStdString(saveDirectory)));
  }
  
  //log action to git.
  std::ostringstream gitMessage;
  gitMessage << QObject::tr("Changing color of feature: ").toStdString()
    << findFeature(featureIdIn)->getName().toStdString()
    << "    Id: "
    << gu::idToString(featureIdIn);
  gitManager->appendGitMessage(gitMessage.str());
}

std::vector<boost::uuids::uuid> Project::getAllFeatureIds() const
{
  std::vector<uuid> out;
  BGL_FORALL_VERTICES(currentVertex, projectGraph, Graph)
  {
    out.push_back(projectGraph[currentVertex].feature->getId());
  }
  
  return out;
}

Project::VertexEdgePairs Project::getParents(prg::Vertex vertexIn) const
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

Project::VertexEdgePairs Project::getChildren(prg::Vertex vertexIn) const
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

void Project::setSaveDirectory(const std::string& directoryIn)
{
  saveDirectory = directoryIn;
}

void Project::serialWrite()
{
  //we accumulate all feature occt shapes into 1 master compound and write
  //that to disk. We do this in an effort for occt to keep it's implicit
  //sharing between saves and opens.
  TopoDS_Compound compound;
  std::size_t offset = 0;
  BRep_Builder builder;
  builder.MakeCompound(compound);
  
  prj::srl::Features features;
  BGL_FORALL_VERTICES(currentVertex, projectGraph, Graph)
  {
    ftr::Base *f = projectGraph[currentVertex].feature.get();
    
    //we can't add a null shape to the compound.
    //so we check for null and add 1 vertex as a place holder.
    TopoDS_Shape shapeOut = f->getShape();
    if (shapeOut.IsNull())
      shapeOut = BRepBuilderAPI_MakeVertex(gp_Pnt(0.0, 0.0, 0.0)).Vertex();
    
    features.feature().push_back(prj::srl::Feature(gu::idToString(f->getId()), f->getTypeString(), offset));
    
    //add to master compound
    builder.Add(compound, shapeOut);
    
    offset++;
  }
  
  //write out master compound
  std::ostringstream masterName;
  masterName << saveDirectory << QDir::separator().toLatin1() << "project.brep";
  BRepTools::Write(compound, masterName.str().c_str());
  
  prj::srl::Connections connections;
  BGL_FORALL_EDGES(currentEdge, projectGraph, Graph)
  {
    std::string inputTypeString = ftr::getInputTypeString(projectGraph[currentEdge].inputType);
    ftr::Base *s = projectGraph[boost::source(currentEdge, projectGraph)].feature.get();
    ftr::Base *t = projectGraph[boost::target(currentEdge, projectGraph)].feature.get();
    connections.connection().push_back(prj::srl::Connection(gu::idToString(s->getId()), gu::idToString(t->getId()), inputTypeString));
  }
  
  expr::StringTranslator sTranslator(*expressionManager);
  prj::srl::Expressions expressions;
  std::vector<uuid> formulaIds = expressionManager->getAllFormulaIdsSorted();
  for (const auto& fId : formulaIds)
  {
    std::string eString = sTranslator.buildStringAll(fId);
    prj::srl::Expression expression(gu::idToString(fId), eString);
    expressions.array().push_back(expression);
  }
  
  prj::srl::ExpressionLinks eLinks;
  for (const auto& link : expressionManager->getLinkContainer())
  {
    prj::srl::ExpressionLink sLink(gu::idToString(link.parameterId), gu::idToString(link.formulaId));
    eLinks.array().push_back(sLink);
  }
  
  prj::srl::ExpressionGroups eGroups;
  for (const auto &group : expressionManager->userDefinedGroups)
  {
    prj::srl::ExpressionGroupEntries entries;
    for (const auto &eId : group.formulaIds)
      entries.array().push_back(gu::idToString(eId));
    prj::srl::ExpressionGroup eGroup(gu::idToString(group.id), group.name, entries);
    eGroups.array().push_back(eGroup);
  }
  
  prj::srl::AppVersion version(0, 0, 0);
  std::string projectPath = saveDirectory + QDir::separator().toLatin1() + "project.prjt";
  srl::Project p(version, 0, features, connections, expressions);
  if (!eLinks.array().empty())
    p.expressionLinks().set(eLinks);
  if (!eGroups.array().empty())
    p.expressionGroups().set(eGroups);
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(projectPath.c_str());
  srl::project(stream, p, infoMap);
}

void Project::save()
{
  observer->out(msg::Message(msg::Response | msg::Pre | msg::SaveProject));
  
  gitManager->save();
  
  observer->out(msg::Message(msg::Response | msg::Post | msg::SaveProject));
}

void Project::initializeNew()
{
  serialWrite();
  gitManager->create(saveDirectory);
}

void Project::open()
{
  isLoading = true;
  gitManager->appendGitMessage("Project Open");
  observer->out(msg::Message(msg::Request | msg::Git | msg::Freeze));
  
  std::string projectPath = saveDirectory + QDir::separator().toLatin1() + "project.prjt";
  std::string shapePath = saveDirectory + QDir::separator().toLatin1() + "project.brep";
  
  gitManager->open(saveDirectory + QDir::separator().toLatin1() +".git");
  
  try
  {
    //read master shape.
    TopoDS_Shape masterShape;
    BRep_Builder junk;
    std::fstream file(shapePath.c_str());
    BRepTools::Read(masterShape, file, junk);
    
    auto project = srl::project(projectPath.c_str(), ::xml_schema::Flags::dont_validate);
    FeatureLoad fLoader(saveDirectory + QDir::separator().toLatin1(), masterShape);
    for (const auto &feature : project->features().feature())
    {
      std::shared_ptr<ftr::Base> featurePtr = fLoader.load(feature.id(), feature.type(), feature.shapeOffset());
      if (featurePtr)
        addFeature(featurePtr);
    }
    
    for (const auto &fConnection : project->connections().connection())
    {
      uuid source = gu::stringToId(fConnection.sourceId());
      uuid target = gu::stringToId(fConnection.targetId());
      ftr::InputTypes inputType = ftr::getInputFromString(fConnection.inputType());
      connect(source, target, inputType);
    }
    
    expr::StringTranslator sTranslator(*expressionManager);
    for (const auto &sExpression : project->expressions().array())
    {
      if (sTranslator.parseString(sExpression.stringForm()) != expr::StringTranslator::ParseSucceeded)
      {
        std::cout << "failed expression parse on load:   " << sExpression.stringForm() << std::endl;
        continue;
      }
      expressionManager->setFormulaId(sTranslator.getFormulaOutId(), gu::stringToId(sExpression.id()));
    }
    
    expressionManager->update(); //addFormulaLink requires that everything be up to date.
    if (project->expressionLinks().present())
    {
      for (const auto &sLink : project->expressionLinks().get().array())
      {
        ftr::Parameter *parameter = findParameter(gu::stringToId(sLink.parameterId()));
        expressionManager->addLink(parameter, gu::stringToId(sLink.expressionId()));
      }
    }
    
    if (project->expressionGroups().present())
    {
      for (const auto &sGroup : project->expressionGroups().get().array())
      {
        expr::Group eGroup;
        eGroup.id = gu::stringToId(sGroup.id());
        eGroup.name = sGroup.name();
        for (const auto &entry : sGroup.entries().array())
        {
          eGroup.formulaIds.push_back(gu::stringToId(entry));
        }
        expressionManager->userDefinedGroups.push_back(eGroup);
      }
    }
    
    updateModel();
    
    //hide all non-leaf feature geometry and hide all overlay. before visual update for speed.
    BGL_FORALL_VERTICES(currentVertex, projectGraph, Graph)
    {
        ftr::Base *f = projectGraph[currentVertex].feature.get();
        if (f->isNonLeaf())
            f->hide3D();
        f->hideOverlay();
    }
    updateVisual();
  }
  catch (const xsd::cxx::xml::invalid_utf16_string&)
  {
    std::cerr << "invalid UTF-16 text in DOM model" << std::endl;
  }
  catch (const xsd::cxx::xml::invalid_utf8_string&)
  {
    std::cerr << "invalid UTF-8 text in object model" << std::endl;
  }
  catch (const xml_schema::Exception& e)
  {
    std::cerr << e << std::endl;
  }
  
  observer->out(msg::Message(msg::Request | msg::Git | msg::Thaw));
  isLoading = false;
}

/* id of selected items is always the 'out' id of the evolution record. this affects
 * the processing of the first graph vertex. When tracking down we don't want to process
 * this vertex but when tracking up we do want to process the first graph vertex
 */
template < typename VertexT>
class ShapeTrackVisitorUp : public boost::default_dfs_visitor
{
  public:
    ShapeTrackVisitorUp(VertexT &startVertexIn, const boost::uuids::uuid &shapeId,
                        std::stack<std::vector<boost::uuids::uuid> >&idsStackIn, std::ostringstream &streamIn) :
      startVertex(startVertexIn), idsStack(idsStackIn), stream(streamIn)
    {
      std::vector<boost::uuids::uuid> temp;
      temp.push_back(shapeId);
      idsStack.push(temp);
    }
  
    template <typename GraphT >
    void discover_vertex(VertexT vertexIn, const GraphT & graph) const
    {
      stream
        << "feature: " << std::setw(15) << std::left << graph[vertexIn].feature->getName().toStdString()
        << "    feature id: " << gu::idToString(graph[vertexIn].feature->getId()) << std::endl;
      if (!graph[vertexIn].feature->hasSeerShape())
      {
        stream << " BREAK: no seer shape" << std::endl;
        std::vector<boost::uuids::uuid> junkIds;
        idsStack.push(junkIds);
        return;
      }
      const ftr::SeerShape &shape = graph[vertexIn].feature->getSeerShape();
      std::vector<boost::uuids::uuid> freshIds;
      const std::vector<boost::uuids::uuid> &currentIds = idsStack.top();
      for (const auto &currentId : currentIds)
      {
        stream << "    shape id out: " << gu::idToString(currentId);
        if (!shape.hasEvolveRecordOut(currentId))
        {
            stream << "    BREAK: no evolve record out" << std::endl;
            continue;
        }
        stream <<  "    ids in: ";
        auto ids = shape.devolve(currentId);
        for (const auto &id : ids)
        {
            stream << gu::idToString(id) << " ";
            freshIds.push_back(id);
        }
        stream << std::endl;
      }
      idsStack.push(freshIds);
    }
    
    template <typename GraphT >
    void finish_vertex(VertexT, const GraphT &) const
    {
      idsStack.pop();
    }
    
  private:
    
    VertexT &startVertex;
    std::stack<std::vector<boost::uuids::uuid> >&idsStack;
    std::ostringstream &stream;
};

template < typename VertexT>
class ShapeTrackVisitorDown : public boost::default_dfs_visitor
{
  public:
    ShapeTrackVisitorDown(VertexT &startVertexIn, const boost::uuids::uuid &shapeId,
                          std::stack<std::vector<boost::uuids::uuid> >&idsStackIn, std::ostringstream &streamIn) :
      startVertex(startVertexIn), idsStack(idsStackIn), stream(streamIn)
    {
      std::vector<boost::uuids::uuid> temp;
      temp.push_back(shapeId);
      idsStack.push(temp);
    }
  
    template <typename GraphT >
    void discover_vertex(VertexT vertexIn, const GraphT & graph) const
    {
      stream
        << "feature: " << std::setw(15) << std::left << graph[vertexIn].feature->getName().toStdString()
        << "    feature id: " << gu::idToString(graph[vertexIn].feature->getId()) << std::endl;
      if (vertexIn == startVertex) //note constructor pushes to idstack so we are in sync with finish vertex.
        return;

      if (!graph[vertexIn].feature->hasSeerShape())
      {
        stream << " BREAK: no seer shape" << std::endl;
        std::vector<boost::uuids::uuid> junkIds;
        idsStack.push(junkIds);
        return;
      }
      const ftr::SeerShape &shape = graph[vertexIn].feature->getSeerShape();
      std::vector<boost::uuids::uuid> freshIds;
      const std::vector<boost::uuids::uuid> &currentIds = idsStack.top();
      for (const auto &currentId : currentIds)
      {
        stream << "    shape id in: " << gu::idToString(currentId);
        if (!shape.hasEvolveRecordIn(currentId))
        {
            stream << "    BREAK: no evolve record in" << std::endl;
            continue;
        }
        stream <<  "    ids out: ";
        auto ids = shape.evolve(currentId);
        for (const auto &id : ids)
        {
            stream << gu::idToString(id) << " ";
            freshIds.push_back(id);
        }
        stream << std::endl;
      }
      idsStack.push(freshIds);
    }
    
    template <typename GraphT >
    void finish_vertex(VertexT, const GraphT &) const
    {
      idsStack.pop();
    }
    
  private:
    
    VertexT &startVertex;
    std::stack<std::vector<boost::uuids::uuid> >&idsStack;
    std::ostringstream &stream;
};

void Project::shapeTrackUp(const uuid& featureIdIn, const uuid& shapeId)
{
  IdVertexMap::const_iterator it = map.find(featureIdIn);
  assert(it != map.end());
  prg::Vertex startVertex = it->second;
  
  if (!projectGraph[startVertex].feature->hasSeerShape())
    return;
  
  assert(projectGraph[startVertex].feature->getSeerShape().hasShapeIdRecord(shapeId));
  assert(projectGraph[startVertex].feature->getSeerShape().hasEvolveRecordOut(shapeId));
  
  GraphReversed rGraph = boost::make_reverse_graph(projectGraph);
  
  std::vector<VertexReversed> limitVertices;
  gu::BFSLimitVisitor<VertexReversed>limitVisitor(limitVertices);
  boost::breadth_first_search(rGraph, startVertex, visitor(limitVisitor));
  
  gu::SubsetFilter<GraphReversed> filter(rGraph, limitVertices);
  typedef boost::filtered_graph<GraphReversed, boost::keep_all, gu::SubsetFilter<GraphReversed> > FilteredGraph;
  typedef boost::graph_traits<FilteredGraph>::vertex_descriptor FilteredVertex;
  FilteredGraph filteredGraph(rGraph, boost::keep_all(), filter);
  
  std::ostringstream stream;
  stream << "Track shape up:" << std::endl;
  std::stack<std::vector<boost::uuids::uuid> >idsStack;
  ShapeTrackVisitorUp<FilteredVertex> shapeVisitor(startVertex, shapeId, idsStack, stream);
  boost::depth_first_search(filteredGraph, visitor(shapeVisitor).root_vertex(startVertex));
  
  msg::Message viewInfoMessage(msg::Request | msg::Info | msg::Text);
  app::Message appMessage;
  appMessage.infoMessage = QString::fromStdString(stream.str());
  viewInfoMessage.payload = appMessage;
  observer->out(viewInfoMessage);
}

void Project::shapeTrackDown(const uuid& featureIdIn, const uuid& shapeId)
{
  IdVertexMap::const_iterator it = map.find(featureIdIn);
  assert(it != map.end());
  prg::Vertex startVertex = it->second;
  
  if (!projectGraph[startVertex].feature->hasSeerShape())
    return;
  
  assert(projectGraph[startVertex].feature->getSeerShape().hasShapeIdRecord(shapeId));
  
  std::vector<Vertex> limitVertices;
  gu::BFSLimitVisitor<Vertex>limitVisitor(limitVertices);
  boost::breadth_first_search(projectGraph, startVertex, visitor(limitVisitor));
  
  gu::SubsetFilter<Graph> filter(projectGraph, limitVertices);
  typedef boost::filtered_graph<Graph, boost::keep_all, gu::SubsetFilter<Graph> > FilteredGraph;
  typedef boost::graph_traits<FilteredGraph>::vertex_descriptor FilteredVertex;
  FilteredGraph filteredGraph(projectGraph, boost::keep_all(), filter);
  
  std::ostringstream stream;
  stream << "Track shape down:" << std::endl;
  std::stack<std::vector<boost::uuids::uuid> >idsStack;
  ShapeTrackVisitorDown<FilteredVertex> shapeVisitor(startVertex, shapeId, idsStack, stream);
  boost::depth_first_search(filteredGraph, visitor(shapeVisitor).root_vertex(startVertex));
  
  msg::Message viewInfoMessage(msg::Request | msg::Info | msg::Text);
  app::Message appMessage;
  appMessage.infoMessage = QString::fromStdString(stream.str());
  viewInfoMessage.payload = appMessage;
  observer->out(viewInfoMessage);
}

ftr::EditMap Project::getParentMap(const boost::uuids::uuid &idIn) const
{
  ftr::EditMap updateMap;
  for (const auto &pair : getParents(findVertex(idIn)))
  {
    auto temp = std::make_pair(projectGraph[pair.second].inputType, projectGraph[pair.first].feature.get());
    updateMap.insert(temp);
  }
  
  return updateMap;
}

template <typename VertexT>
class LeafChildrenVisitor : public boost::default_dfs_visitor
{
public:
  LeafChildrenVisitor(std::vector<VertexT> &leafChildrenIn) :
    leafChildren(leafChildrenIn)
  {
  }
  
  template <typename GraphT >
  void start_vertex(VertexT vertexIn, const GraphT&) const
  {
    startVertex = vertexIn;
  }
  
  template <typename GraphT >
  void discover_vertex(VertexT vertexIn, const GraphT & graphIn) const
  {
    if (foundLeaf)
      return;
    if (vertexIn == startVertex)
      return;
    if (graphIn[vertexIn].feature->isLeaf())
    {
      foundLeaf = true;
      leafChildren.push_back(vertexIn);
    }
  }
  
  template <typename GraphT >
  void finish_vertex(VertexT vertexIn, const GraphT &graphIn) const
  {
    if (graphIn[vertexIn].feature->isActive())
      foundLeaf = false;
  }
  
private:
  std::vector<VertexT> &leafChildren;
  mutable bool foundLeaf = false;
  mutable VertexT startVertex;
};

/*! editing a feature uses setCurrentLeaf to 'rewind' to state
 * when feature was created. When editing is done we want to return
 * leaf states to previous state before editing. this is where this
 * function comes in. It gets leaf status of each path so it can be
 * 'reset' after editing'
 */
std::vector<uuid> Project::getLeafChildren(const uuid &parentIn) const
{
  IdVertexMap::const_iterator it = map.find(parentIn);
  assert(it != map.end());
  prg::Vertex startVertex = it->second;
  
  std::vector<Vertex> limitVertices;
  gu::BFSLimitVisitor<Vertex>limitVisitor(limitVertices);
  boost::breadth_first_search(projectGraph, startVertex, visitor(limitVisitor));
  
  gu::SubsetFilter<Graph> filter(projectGraph, limitVertices);
  typedef boost::filtered_graph<Graph, boost::keep_all, gu::SubsetFilter<Graph> > FilteredGraph;
  typedef boost::graph_traits<FilteredGraph>::vertex_descriptor FilteredVertex;
  FilteredGraph filteredGraph(projectGraph, boost::keep_all(), filter);
  
  std::vector<Vertex> leafChildren;
  LeafChildrenVisitor<FilteredVertex> leafVisitor(leafChildren);
  boost::depth_first_search(filteredGraph, visitor(leafVisitor));
  
  std::vector<uuid> out;
  for (const auto &v : leafChildren)
    out.push_back(projectGraph[v].feature->getId());
  
  return out;
}

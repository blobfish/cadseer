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
#include <QUrl>
#include <QDesktopServices>

#include <boost/graph/topological_sort.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/signals2/shared_connection_block.hpp>
#include <boost/filesystem.hpp>

#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>

#include <osg/ValueObject>

#include <application/application.h>
#include <globalutilities.h>
#include <tools/idtools.h>
#include <feature/base.h>
#include <annex/seershape.h>
#include <feature/inert.h>
#include <feature/shapehistory.h>
#include <expressions/manager.h>
#include <expressions/formulalink.h>
#include <expressions/stringtranslator.h> //for serialize.
#include <project/message.h>
#include <message/message.h>
#include <message/dispatch.h>
#include <message/observer.h>
#include <project/gitmanager.h>
#include <tools/graphtools.h>
#include <project/featureload.h>
#include <project/serial/xsdcxxoutput/project.h>
#include <project/stow.h>
#include <project/project.h>

using namespace prj;
using boost::uuids::uuid;

Project::Project() :
gitManager(new GitManager()),
expressionManager(new expr::Manager()),
shapeHistory(new ftr::ShapeHistory()),
observer(new msg::Observer()),
stow(new Stow())
{
  observer->name = "prj::Project";
  setupDispatcher();
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
  observer->out(msg::Message(msg::Response | msg::Pre | msg::Update | msg::Model));
  
  expressionManager->update();
  
  Vertices sorted;
  try
  {
//     indexVerticesEdges();
    boost::topological_sort(stow->graph, std::back_inserter(sorted));
  }
  catch(const boost::not_a_dag &)
  {
    std::cout << std::endl << "Graph is not a dag exception in Project::updateModel()" << std::endl << std::endl;
    return;
  }
  
  //the update of a feature will trigger a state change signal.
  //we don't want to handle that state change here in the project
  //so we block.
  auto block = observer->createBlocker();
  
  shapeHistory->clear(); //reset history structure.
  
  //loop through and update each feature.
  for (auto it = sorted.rbegin(); it != sorted.rend(); ++it)
  {
    Vertex currentVertex = *it;
    ftr::Base *cFeature = stow->graph[currentVertex].feature.get();
    
    if (!stow->graph[currentVertex].alive)
      continue;
    
    if
    (
      (cFeature->isModelClean()) ||
      (stow->isFeatureInactive(currentVertex))
    )
    {
      cFeature->fillInHistory(*shapeHistory);
      continue;
    }
    
    std::ostringstream messageStream;
    messageStream << "Updating: " << cFeature->getName().toStdString() << "    Id: " << gu::idToString(cFeature->getId());
    observer->out(msg::buildStatusMessage(messageStream.str()));
    qApp->processEvents(); //need this or we won't see messages.
    
    RemovedGraph removedGraph = buildRemovedGraph(stow->graph);
    ReversedGraph reversedGraph = boost::make_reverse_graph(removedGraph);
    ftr::UpdatePayload::UpdateMap updateMap =
    buildAjacentUpdateMap
    <
      ReversedGraph,
      boost::graph_traits<ReversedGraph>::vertex_descriptor
    >(reversedGraph, currentVertex);
    
    ftr::UpdatePayload payload(updateMap, *shapeHistory);
    cFeature->updateModel(payload);
    cFeature->serialWrite(QDir(QString::fromStdString(saveDirectory)));
    cFeature->fillInHistory(*shapeHistory);
  }
  
  updateLeafStatus();
  serialWrite();
  /* this is here to give others a chance to make changes before git makes a commit.
   * if this causes problems, then we will want git manager to use more messaging.
   */
  observer->out(msg::Message(msg::Response | msg::Post | msg::Update | msg::Model));
  gitManager->update();
  
//   shapeHistory->writeGraphViz("/home/tanderson/.CadSeer/ShapeHistory.dot");
  
  observer->out(msg::buildStatusMessage("Update Complete"));
}

void Project::updateVisual()
{
  //if we have selection and then destroy the geometry when the
  //the visual updates, things get out of sync. so clear the selection.
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
  observer->out(msg::Message(msg::Response | msg::Pre | msg::Update | msg::Visual));
  
  auto block = observer->createBlocker();
  
  //don't think we need to topo sort for visual.
  for(auto its = boost::vertices(stow->graph); its.first != its.second; ++its.first)
  {
    if (!stow->graph[*its.first].alive)
      continue;
    auto feature = stow->graph[*its.first].feature;
    if
    (
      feature->isModelClean() &&
      feature->isVisible3D() &&
      stow->isFeatureActive(*its.first) &&
//       feature->isSuccess() && //regenerate from parent shape on failure.
      feature->isVisualDirty()
    )
      feature->updateVisual();
  }
  
  observer->out(msg::Message(msg::Response | msg::Post | msg::Update | msg::Visual));
}

void Project::writeGraphViz(const std::string& fileName)
{
  stow->writeGraphViz(fileName);
}

void Project::readOCC(const std::string &fileName)
{
  TopoDS_Shape base;
  BRep_Builder junk;
  std::fstream file(fileName.c_str());
  BRepTools::Read(base, file, junk);
  
  boost::filesystem::path p = fileName;
  addOCCShape(base, p.filename().string());
}

bool Project::hasFeature(const boost::uuids::uuid &idIn) const
{
  return stow->hasFeature(idIn);
}

ftr::Base* Project::findFeature(const uuid &idIn) const
{
  return stow->findFeature(idIn);
}

ftr::prm::Parameter* Project::findParameter(const uuid &idIn) const
{
  return stow->findParameter(idIn);
}

void Project::addOCCShape(const TopoDS_Shape &shapeIn, std::string name)
{
  std::shared_ptr<ftr::Inert> inert(new ftr::Inert(shapeIn));
  addFeature(inert);
  if (!name.empty())
    inert->setName(QString::fromStdString(name));
}

void Project::addFeature(std::shared_ptr<ftr::Base> feature)
{
  //no pre message.
  stow->addFeature(feature);
  
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
  Vertex vertex = stow->findVertex(idIn);
  std::shared_ptr<ftr::Base> feature = stow->graph[vertex].feature;
  
  //don't block before this dirty call or this won't trigger the dirty children.
  feature->setModelDirty(); //this will make all children dirty.
  
  //shouldn't need anymore messages into project for this function call.
  auto block = observer->createBlocker();
  
  RemovedGraph removedGraph = buildRemovedGraph(stow->graph); //children
  ReversedGraph reversedGraph = boost::make_reverse_graph(removedGraph); //parents
  
  //for now all children get connected to target parent.
  if (boost::in_degree(vertex, stow->graph) > 0)
  {
    //default to first parent.
    Vertex targetParent = NullVertex();
    for (auto its = boost::adjacent_vertices(vertex, reversedGraph); its.first != its.second; ++its.first)
    {
      if (targetParent == NullVertex())
        targetParent = *its.first;
      auto ce = boost::edge(vertex, *its.first, reversedGraph);
      assert(ce.second);
      if (reversedGraph[ce.first].inputType.has(ftr::InputType::target))
      {
        targetParent = *its.first;
        break;
      }
    }
    for (auto its = boost::adjacent_vertices(vertex, removedGraph); its.first != its.second; ++its.first)
    {
      auto ce = boost::edge(vertex, *its.first, removedGraph);
      assert(ce.second);
      
      stow->connect(targetParent, *its.first, removedGraph[ce.first].inputType);
      sendConnectMessage(removedGraph[targetParent].feature->getId(), removedGraph[*its.first].feature->getId(), removedGraph[ce.first].inputType);
      
      if (!feature->hasAnnex(ann::Type::SeerShape))
        continue;
      //update ids.
      for (const uuid &staleId : feature->getAnnex<ann::SeerShape>(ann::Type::SeerShape).getAllShapeIds())
      {
        uuid freshId = shapeHistory->devolve(stow->graph[targetParent].feature->getId(), staleId);
        if (freshId.is_nil())
          continue;
        stow->graph[*its.first].feature->replaceId(staleId, freshId, *shapeHistory);
      }
    }
  }
  
  for (auto its = boost::adjacent_vertices(vertex, reversedGraph); its.first != its.second; ++its.first)
  {
    auto ce = boost::edge(vertex, *its.first, reversedGraph);
    assert(ce.second);
    
    msg::Message preMessage(msg::Response | msg::Pre | msg::Remove | msg::Connection);
    prj::Message pMessage;
    pMessage.featureId = reversedGraph[*its.first].feature->getId();
    pMessage.featureId2 = idIn;
    pMessage.inputType = reversedGraph[ce.first].inputType;
    preMessage.payload = pMessage;
    observer->out(preMessage);
    
    //make parents have same visible state as the feature being removed.
    if (feature->isVisible3D())
      observer->outBlocked(msg::buildShowThreeD(reversedGraph[*its.first].feature->getId()));
    else
      observer->outBlocked(msg::buildHideThreeD(reversedGraph[*its.first].feature->getId()));
  }
  
  for (auto its = boost::adjacent_vertices(vertex, removedGraph); its.first != its.second; ++its.first)
  {
    auto ce = boost::edge(vertex, *its.first, removedGraph);
    assert(ce.second);
    
    msg::Message preMessage(msg::Response | msg::Pre | msg::Remove | msg::Connection);
    prj::Message pMessage;
    pMessage.featureId = idIn;
    pMessage.featureId2 = removedGraph[*its.first].feature->getId();
    pMessage.inputType = removedGraph[ce.first].inputType;
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
  
  boost::clear_vertex(vertex, stow->graph);
  boost::remove_vertex(vertex, stow->graph);
  
  //log action to git.
  std::ostringstream gitMessage;
  gitMessage << QObject::tr("Removing feature ").toStdString() << feature->getTypeString();
  gitManager->appendGitMessage(gitMessage.str());
  
  //no post message.
}

void Project::setCurrentLeaf(const uuid& idIn)
{
//   indexVerticesEdges();
  
  //sometimes the visual of a feature is dirty and doesn't get updated 
  //until we try to show it. However it might be selected meaning that
  //we will destroy the old geometry that is highlighted. So clear the seleciton.
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  //the visitor will be setting features to an inactive state which
  //triggers the signal and we would end up back into this->stateChangedSlot.
  //so we block all the connections to avoid this.
  auto block = observer->createBlocker();
  
  auto vertex = stow->findVertex(idIn);
  RemovedGraph removedGraph = buildRemovedGraph(stow->graph); //children
  ReversedGraph reversedGraph = boost::make_reverse_graph(removedGraph); //parents
  
//   GraphReversed rGraph = boost::make_reverse_graph(projectGraph);
  
  //parents
  Vertices aVertices;
  AccrueVisitor<Vertex> activeVisitor(aVertices);
  boost::breadth_first_search(reversedGraph, vertex, boost::visitor(activeVisitor));
  for (const auto &v : aVertices)
  {
    stow->setFeatureActive(v);
    observer->out(msg::buildHideThreeD(reversedGraph[v].feature->getId()));
  }
  //if this is a create feature we don't want to hide the immediate parents.
  if (reversedGraph[vertex].feature->getDescriptor() == ftr::Descriptor::Create)
  {
    for (auto its = boost::adjacent_vertices(vertex, reversedGraph); its.first != its.second; ++its.first)
    {
      stow->setFeatureActive(*its.first);
      observer->out(msg::buildShowThreeD(reversedGraph[*its.first].feature->getId()));
    }
  }
  
  //children
  Vertices iVertices;
  AccrueVisitor<Vertex> inactiveVisitor(iVertices);
  boost::breadth_first_search(removedGraph, vertex, boost::visitor(inactiveVisitor));
  for (const auto &v : iVertices)
  {
    stow->setFeatureInactive(v);
    observer->out(msg::buildHideThreeD(removedGraph[v].feature->getId()));
  }
  
  //now we don't want the actual feature inactive just it's children so we
  //turn it back on because the BFS and visitor turned it off.
  stow->setFeatureActive(vertex);
  observer->out(msg::buildShowThreeD(stow->graph[vertex].feature->getId()));
  
  updateLeafStatus();
}

void Project::updateLeafStatus()
{
  //we end up in here twice when set current leaf from dag view.
  //once when make the change and once when we call an update.
//   indexVerticesEdges(); //redundent for setCurrentLeaf call.
  
  RemovedGraph removedGraph = buildRemovedGraph(stow->graph);
  
  //first set all features to non leaf.
  for (auto its = boost::vertices(removedGraph); its.first != its.second; ++its.first)
    stow->setFeatureNonLeaf(*its.first);
  
  ActiveFilter<RemovedGraph> activeFilter(removedGraph);
  
  typedef boost::filtered_graph<RemovedGraph, boost::keep_all, ActiveFilter<RemovedGraph> > FilteredGraphType;
  FilteredGraphType filteredGraph(removedGraph, boost::keep_all(), activeFilter);
  
  for (auto its = boost::vertices(filteredGraph); its.first != its.second; ++its.first)
  {
    if (boost::out_degree(*its.first, filteredGraph) == 0)
      stow->setFeatureLeaf(*its.first);
    else
    {
      //if all children are of the type 'create' then it is also considered leaf.
      bool allCreate = true;
      for (auto nits = boost::adjacent_vertices(*its.first, filteredGraph); nits.first != nits.second; ++nits.first)
      {
        if (filteredGraph[*nits.first].feature->getDescriptor() != ftr::Descriptor::Create)
        {
          allCreate = false;
          break;
        }
      }
      if (allCreate)
        stow->setFeatureLeaf(*its.first);
    }
  }
}

void Project::connect(const boost::uuids::uuid& parentIn, const boost::uuids::uuid& childIn, const ftr::InputType &type)
{
  Vertex parent = stow->findVertex(parentIn);
  Vertex child = stow->findVertex(childIn);
  stow->connect(parent, child, type);
  
  sendConnectMessage(parentIn, childIn, type);
}

void Project::sendConnectMessage(const uuid &parentIn, const uuid &childIn, const ftr::InputType &type)
{
  msg::Message postMessage(msg::Response | msg::Post | msg::Add | msg::Connection);
  prj::Message pMessage;
  pMessage.featureId = parentIn;
  pMessage.featureId2 = childIn; 
  pMessage.inputType = type;
  postMessage.payload = pMessage;
  observer->out(postMessage);
}

void Project::removeParentTag(const uuid &targetIn, const std::string &tagIn)
{
  RemovedGraph removedGraph = buildRemovedGraph(stow->graph); //children
  ReversedGraph reversedGraph = boost::make_reverse_graph(removedGraph); //parents
  
  Vertex vertex = stow->findVertex(targetIn);
  for (auto its = boost::adjacent_vertices(vertex, reversedGraph); its.first != its.second; ++its.first)
  {
    //edge references original graph. 'forward'. remove_edge wouldn't work on reversed graph.
    auto ce = boost::edge(*its.first, vertex, stow->graph);
    assert(ce.second);
    stow->graph[ce.first].inputType.tags.erase(tagIn);
    if (stow->graph[ce.first].inputType.tags.empty())
    {
      msg::Message preMessage(msg::Response | msg::Pre | msg::Remove | msg::Connection);
      prj::Message pMessage;
      pMessage.featureId = reversedGraph[*its.first].feature->getId();
      pMessage.featureId2 = targetIn;
      pMessage.inputType = ftr::InputType({tagIn});
      preMessage.payload = pMessage;
      observer->out(preMessage);
      
      boost::remove_edge(ce.first, stow->graph);
    }
  }
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
  
  mask = msg::Request | msg::Update | msg::Model;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Project::updateModelDispatched, this, _1)));
  
  mask = msg::Request | msg::Update | msg::Visual;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Project::updateVisualDispatched, this, _1)));
  
  mask = msg::Request | msg::Save | msg::Project;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Project::saveProjectRequestDispatched, this, _1)));
  
  mask = msg::Request | msg::CheckShapeIds;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Project::checkShapeIdsDispatched, this, _1)));
  
  mask = msg::Response | msg::Feature | msg::Status;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Project::featureStateChangedDispatched, this, _1)));
  
  mask = msg::Request | msg::DebugDumpProjectGraph;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Project::dumpProjectGraphDispatched, this, _1)));
  
  mask = msg::Response | msg::View | msg::Show | msg::ThreeD;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Project::shownThreeDDispatched, this, _1)));
}

void Project::featureStateChangedDispatched(const msg::Message &messageIn)
{
  ftr::Message fMessage = boost::get<ftr::Message>(messageIn.payload);
  //this could be a performance problem. we are visiting children and setting model
  //dirty when ever a state change happens with a model dirty feature. we only need to
  //do this when the model dirty changes not all the other state changes.
  if (!fMessage.state.test(ftr::StateOffset::ModelDirty))
    return;
    
//   indexVerticesEdges();
  
  //this code blocks all incoming messages to the project while it
  //executes. This prevents the cycles from setting a dependent dirty.
  auto block = observer->createBlocker();
  
  Vertex vertex = stow->findVertex(fMessage.featureId);
  Vertices vertices;
  AccrueVisitor<Vertex> visitor(vertices);
  boost::breadth_first_search(stow->graph, vertex, boost::visitor(visitor));
  for (const auto &v : vertices)
    stow->graph[v].feature->setModelDirty();
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
  
  app::WaitCursor waitCursor;
  updateModel();
  updateVisual();
}

void Project::forceUpdateDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  for (auto its = boost::vertices(stow->graph); its.first != its.second; ++its.first)
  {
    stow->graph[*its.first].feature->setModelDirty();
  }
  
  app::WaitCursor waitCursor;
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
  
  
  // this has not been updated to the new graph.
  std::cout << "command is out of date" << std::endl;
  
  /*
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
  
  */
}

void Project::dumpProjectGraphDispatched(const msg::Message &)
{
//   indexVerticesEdges();
  
  QString fileName = static_cast<app::Application *>(qApp)->getApplicationDirectory().path();
  fileName += QDir::separator();
  fileName += "project.dot";
  stow->writeGraphViz(fileName.toStdString().c_str());
  
  QDesktopServices::openUrl(QUrl(fileName));
}

void Project::shownThreeDDispatched(const msg::Message &mIn)
{
  uuid id = boost::get<vwr::Message>(mIn.payload).featureId;
  auto feature = stow->findFeature(id);
  if
  (
    feature->isModelClean() &&
    feature->isVisible3D() &&
    isFeatureActive(id) &&
    feature->isVisualDirty()
  )
    feature->updateVisual();
}

void Project::setAllVisualDirty()
{
  for (auto its = boost::vertices(stow->graph); its.first != its.second; ++its.first)
  {
    stow->graph[*its.first].feature->setVisualDirty();
  }
}

void Project::setColor(const boost::uuids::uuid &featureIdIn, const osg::Vec4 &colorIn)
{
  //the following is a good example of accumulating history of 1 object.
  
  //first remove any non 'target' edges. this will limit the following searches.
  TargetEdgeFilter<Graph> edgeFilter(stow->graph);
  typedef boost::filtered_graph<Graph, TargetEdgeFilter<Graph>, boost::keep_all> TargetFilteredGraph;
  TargetFilteredGraph tFilteredGraph(stow->graph, edgeFilter, boost::keep_all());
  
  //find forward connected vertices.
  Vertex baseVertex = stow->findVertex(featureIdIn);
  Vertices vertexes; //note: name 'vertices' clashes with forall_vertices macro.
  gu::BFSLimitVisitor<Vertex> vis(vertexes);
  boost::breadth_first_search(tFilteredGraph, baseVertex, visitor(vis));
  
  //find reverse connected vertices.
  typedef boost::reverse_graph<TargetFilteredGraph, TargetFilteredGraph&> TFReversedGraph;
  TFReversedGraph rGraph = boost::make_reverse_graph(tFilteredGraph);
  gu::BFSLimitVisitor<Vertex> rVis(vertexes);
  boost::breadth_first_search(rGraph, baseVertex, visitor(rVis));
  
  //filter on the accumulated vertexes.
  gu::SubsetFilter<Graph> vertexFilter(stow->graph, vertexes);
  typedef boost::filtered_graph<Graph, boost::keep_all, gu::SubsetFilter<Graph> > FilteredGraph;
  FilteredGraph filteredGraph(stow->graph, boost::keep_all(), vertexFilter);
  
  //set color of all objects.
  for (auto its = boost::vertices(filteredGraph); its.first != its.second; ++its.first)
  {
    stow->graph[*its.first].feature->setColor(colorIn);
    //this is a hack. Currently, in order for this color change to be serialized
    //at next update we would have to mark the feature dirty. Marking the feature dirty
    //and causing models to be recalculated seems excessive for such a minor change as
    //object color. So here we just serialize the changed features to 'sneak' the
    //color change into the git commit.
    stow->graph[*its.first].feature->serialWrite(QDir(QString::fromStdString(saveDirectory)));
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
  
  for (auto its = boost::vertices(stow->graph); its.first != its.second; ++its.first)
  {
    if (!stow->graph[*its.first].alive)
      continue;
    out.push_back(stow->graph[*its.first].feature->getId());
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
  
  RemovedGraph removedGraph = buildRemovedGraph(stow->graph);
  
  prj::srl::Features features;
  prj::srl::FeatureStates states;
  for (auto its = boost::vertices(removedGraph); its.first != its.second; ++its.first)
  {
    ftr::Base *f = removedGraph[*its.first].feature.get();
    
    //save the state
    states.array().push_back(prj::srl::FeatureState(gu::idToString(f->getId()), removedGraph[*its.first].state.to_string()));
    
    //we can't add a null shape to the compound.
    //so we check for null and add 1 vertex as a place holder.
    TopoDS_Shape shapeOut;
    if (f->hasAnnex(ann::Type::SeerShape))
    {
      const ann::SeerShape &sShape = f->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
      if (sShape.isNull() || sShape.getRootOCCTShape().IsNull())
        std::cout << "WARNING: trying to write out null shape in: Project::serialWrite" << std::endl;
      else
        shapeOut = sShape.getRootOCCTShape();
    }
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
  for (auto its = boost::edges(removedGraph); its.first != its.second; ++its.first)
  {
    prj::srl::InputTypes inputTypes;
    for (const auto &tag : removedGraph[*its.first].inputType.tags)
      inputTypes.array().push_back(tag);
    ftr::Base *s = removedGraph[boost::source(*its.first, removedGraph)].feature.get();
    ftr::Base *t = removedGraph[boost::target(*its.first, removedGraph)].feature.get();
    connections.connection().push_back(prj::srl::Connection(gu::idToString(s->getId()), gu::idToString(t->getId()), inputTypes));
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
  for (const auto& link : expressionManager->getLinkContainer().container)
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
  srl::Project p(version, 0, features, states, connections, expressions);
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
  observer->out(msg::Message(msg::Response | msg::Pre | msg::Save | msg::Project));
  
  gitManager->save();
  
  observer->out(msg::Message(msg::Response | msg::Post | msg::Save | msg::Project));
}

void Project::initializeNew()
{
  serialWrite();
  gitManager->create(saveDirectory);
}

void Project::open()
{
  app::WaitCursor waitCursor;
  
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
      std::ostringstream messageStream;
      messageStream << "Loading: " << feature.type() << "    Id: " << feature.id();
      observer->outBlocked(msg::buildStatusMessage(messageStream.str()));
      qApp->processEvents(); //need this or we won't see messages.
      
      std::shared_ptr<ftr::Base> featurePtr = fLoader.load(feature.id(), feature.type(), feature.shapeOffset());
      if (featurePtr)
      {
        addFeature(featurePtr);
        
        //send state message
        ftr::Message fMessage(featurePtr->getId(), featurePtr->getState());
        msg::Message mMessage(msg::Response | msg::Feature | msg::Status);
        mMessage.payload = fMessage;
        observer->outBlocked(mMessage);
      }
    }
    
    for (const auto &state : project->states().array())
    {
      uuid fId = gu::stringToId(state.id());
      ftr::State fState(state.state());
      stow->graph[stow->findVertex(fId)].state = fState;
      
      //send state message
      ftr::Message fMessage(fId, fState);
      msg::Message mMessage(msg::Response | msg::Project | msg::Feature | msg::Status);
      mMessage.payload = fMessage;
      observer->outBlocked(mMessage);
    }
    
    for (const auto &fConnection : project->connections().connection())
    {
      uuid source = gu::stringToId(fConnection.sourceId());
      uuid target = gu::stringToId(fConnection.targetId());
      
      ftr::InputType inputType;
      for (const auto &sInputType : fConnection.inputType().array())
        inputType.insert(sInputType);
      connect(source, target, inputType);
    }
    
    expr::StringTranslator sTranslator(*expressionManager);
    for (const auto &sExpression : project->expressions().array())
    {
      if (sTranslator.parseString(sExpression.stringForm()) != expr::StringTranslator::ParseSucceeded)
      {
        std::cout << "failed expression parse on load: \"" << sExpression.stringForm()
          << "\". Error message: " << sTranslator.getFailureMessage() << std::endl;
        continue;
      }
      expressionManager->setFormulaId(sTranslator.getFormulaOutId(), gu::stringToId(sExpression.id()));
      
      /* scenario:
       * We now have multiple formula types. scalar, vector etc....
       * The types for the formula nodes are not set until update/calculate are called.
       * The string parser checks input strings for compatable types.
       * 
       * problem:
       * When we read multiple expressions from the project file that has expressions
       * linked to each other, the parser fails. It fails because the types of the 
       * formula nodes are not known yet.
       * 
       * To avoid this we are call update after each expression. I don't think this will
       * be significant with respect to performance.
       */
      expressionManager->update();
    }
    
    if (project->expressionLinks().present())
    {
      for (const auto &sLink : project->expressionLinks().get().array())
      {
        ftr::prm::Parameter *parameter = findParameter(gu::stringToId(sLink.parameterId()));
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
    
    observer->outBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
    observer->outBlocked(msg::buildStatusMessage("Project Opened"));
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

void Project::shapeTrackUp(const uuid& shapeId) const
{
  //testing shape history.
  ftr::ShapeHistory devolve = shapeHistory->createDevolveHistory(shapeId);
  devolve.writeGraphViz("/home/tanderson/.CadSeer/devolveHistory.dot");
}

void Project::shapeTrackDown(const uuid& shapeId) const
{
  //testing shape history.
  ftr::ShapeHistory evolve = shapeHistory->createEvolveHistory(shapeId);
  evolve.writeGraphViz("/home/tanderson/.CadSeer/evolveHistory.dot");
}

ftr::UpdatePayload::UpdateMap Project::getParentMap(const boost::uuids::uuid &idIn) const
{
  ftr::UpdatePayload::UpdateMap updateMap;
  
  auto vertex = stow->findVertex(idIn);
  RemovedGraph removedGraph = buildRemovedGraph(stow->graph); //children
  ReversedGraph reversedGraph = boost::make_reverse_graph(removedGraph); //parents
  for (auto its = boost::adjacent_vertices(vertex, reversedGraph); its.first != its.second; ++its.first)
  {
    auto e = boost::edge(vertex, *its.first, reversedGraph);
    assert(e.second);
    for (const auto &tag : reversedGraph[e.first].inputType.tags)
    {
      auto temp = std::make_pair(tag, reversedGraph[*its.first].feature.get());
      updateMap.insert(temp);
    }
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
    if (!graphIn[vertexIn].state.test(ftr::StateOffset::NonLeaf))
    {
      foundLeaf = true;
      leafChildren.push_back(vertexIn);
    }
  }
  
  template <typename GraphT >
  void finish_vertex(VertexT vertexIn, const GraphT &graphIn) const
  {
    if (!graphIn[vertexIn].state.test(ftr::StateOffset::Inactive))
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
  Vertex startVertex = stow->findVertex(parentIn);
  
  Vertices limitVertices;
  gu::BFSLimitVisitor<Vertex>limitVisitor(limitVertices);
  boost::breadth_first_search(stow->graph, startVertex, visitor(limitVisitor));
  
  gu::SubsetFilter<Graph> filter(stow->graph, limitVertices);
  typedef boost::filtered_graph<Graph, boost::keep_all, gu::SubsetFilter<Graph> > FilteredGraph;
  typedef boost::graph_traits<FilteredGraph>::vertex_descriptor FilteredVertex;
  FilteredGraph filteredGraph(stow->graph, boost::keep_all(), filter);
  
  std::vector<Vertex> leafChildren;
  LeafChildrenVisitor<FilteredVertex> leafVisitor(leafChildren);
  boost::depth_first_search(filteredGraph, visitor(leafVisitor));
  
  std::vector<uuid> out;
  for (const auto &v : leafChildren)
    out.push_back(stow->graph[v].feature->getId());
  
  return out;
}

void Project::setFeatureActive(const boost::uuids::uuid &idIn)
{
  stow->setFeatureActive(stow->findVertex(idIn));
}

void Project::setFeatureInactive(const boost::uuids::uuid &idIn)
{
  stow->setFeatureInactive(stow->findVertex(idIn));
}

bool Project::isFeatureActive(const boost::uuids::uuid &idIn)
{
  return stow->isFeatureActive(stow->findVertex(idIn));
}

bool Project::isFeatureInactive(const boost::uuids::uuid &idIn)
{
  return stow->isFeatureInactive(stow->findVertex(idIn));
}

void Project::setFeatureLeaf(const boost::uuids::uuid &idIn)
{
  stow->setFeatureLeaf(stow->findVertex(idIn));
}

void Project::setFeatureNonLeaf(const boost::uuids::uuid &idIn)
{
  stow->setFeatureNonLeaf(stow->findVertex(idIn));
}

bool Project::isFeatureLeaf(const boost::uuids::uuid &idIn)
{
  return stow->isFeatureLeaf(stow->findVertex(idIn));
}

bool Project::isFeatureNonLeaf(const boost::uuids::uuid &idIn)
{
  return stow->isFeatureNonLeaf(stow->findVertex(idIn));
}

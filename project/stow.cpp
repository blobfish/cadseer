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

#include <boost/graph/graphviz.hpp>
#include <boost/variant/variant.hpp>

#include <osg/Node> //yuck

#include <tools/idtools.h>
#include <feature/base.h>
#include <message/message.h>
#include <message/observer.h>
#include <project/stow.h>

using namespace prj;

Stow::Stow() : observer(new msg::Observer())
{

}

Stow::~Stow()
{

}

Vertex Stow::addFeature(std::shared_ptr<ftr::Base> feature)
{
  Vertex newVertex = boost::add_vertex(graph);
  graph[newVertex].feature = feature;
  return newVertex;
}

Edge Stow::connect(const Vertex &parentIn, const Vertex &childIn, const ftr::InputType &type)
{
  assert(graph[parentIn].alive);
  assert(graph[childIn].alive);
  
  bool results;
  Edge newEdge;
  boost::tie(newEdge, results) = boost::edge(parentIn, childIn, graph);
  if (!results)
    boost::tie(newEdge, results) = boost::add_edge(parentIn, childIn, graph);
  assert(results);
  if (!results)
  {
    std::cout << "warning: couldn't add edge in prj::Stow::connect" << std::endl;
    return newEdge;
  }
  graph[newEdge].inputType += type;
  sendConnectMessage(parentIn, childIn, type);
  return newEdge;
}

void Stow::sendConnectMessage(const Vertex &parentIn, const Vertex &childIn, const ftr::InputType &type) const
{
  msg::Message postMessage(msg::Response | msg::Post | msg::Add | msg::Connection);
  prj::Message pMessage;
  pMessage.featureIds.push_back(graph[parentIn].feature->getId());
  pMessage.featureIds.push_back(graph[childIn].feature->getId()); 
  pMessage.inputType = type;
  postMessage.payload = pMessage;
  observer->outBlocked(postMessage);
}

void Stow::disconnect(const Edge &eIn)
{
  sendDisconnectMessage(boost::source(eIn, graph), boost::target(eIn, graph), graph[eIn].inputType);
  boost::remove_edge(eIn, graph);
}

void Stow::sendDisconnectMessage(const Vertex &parentIn, const Vertex &childIn, const ftr::InputType &type) const
{
  msg::Message preMessage(msg::Response | msg::Pre | msg::Remove | msg::Connection);
  prj::Message pMessage;
  pMessage.featureIds.push_back(graph[parentIn].feature->getId());
  pMessage.featureIds.push_back(graph[childIn].feature->getId());
  pMessage.inputType = type;
  preMessage.payload = pMessage;
  observer->out(preMessage);
}

void Stow::removeEdges(Edges esIn)
{
  //don't invalidate.
  gu::uniquefy(esIn);
  for (auto it = esIn.rbegin(); it != esIn.rend(); ++it)
    disconnect(*it);
}

bool Stow::hasFeature(const boost::uuids::uuid &idIn) const
{
  return findVertex(idIn) != NullVertex();
}

ftr::Base* Stow::findFeature(const boost::uuids::uuid &idIn) const
{
  Vertex v = findVertex(idIn);
  if (v != NullVertex())
    return graph[v].feature.get();
  assert(0); //no feature with id in prj::Stow::findFeature
  std::cout << "warning: no feature with id in prj::Stow::findFeature" << std::endl;
  return nullptr;
}

Vertex Stow::findVertex(const boost::uuids::uuid &idIn) const
{
  for (auto its = boost::vertices(graph); its.first != its.second; ++its.first)
  {
    if (graph[*its.first].feature->getId() == idIn)
    {
      assert(graph[*its.first].alive);
      return *its.first;
    }
  }
  assert(0); //no vertex with id in prj::Stow::findVertex
  std::cout << "warning: no vertex with id in prj::Stow::findVertex" << std::endl;
  return NullVertex();
}

std::vector<boost::uuids::uuid> Stow::getAllFeatureIds() const
{
  std::vector<boost::uuids::uuid> out;
  
  for (auto its = boost::vertices(graph); its.first != its.second; ++its.first)
  {
    if (graph[*its.first].alive)
      out.push_back(graph[*its.first].feature->getId());
  }
  
  return out;
}

ftr::prm::Parameter* Stow::findParameter(const boost::uuids::uuid &idIn) const
{
  for (auto its = boost::vertices(graph); its.first != its.second; ++its.first)
  {
    if (!graph[*its.first].feature->hasParameter(idIn))
      continue;
    return graph[*its.first].feature->getParameter(idIn);
  }

  assert(0); //no such parameter.
  std::cout << "warning: no such parameter in prj::Stow::findParameter" << std::endl;
  return nullptr;
}

void Stow::setFeatureActive(Vertex vIn)
{
  if (isFeatureActive(vIn))
    return; //already active.
  graph[vIn].state.set(ftr::StateOffset::Inactive, false);
  sendStateMessage(vIn, ftr::StateOffset::Inactive);
}

void Stow::setFeatureInactive(Vertex vIn)
{
  if (isFeatureInactive(vIn))
    return;
  graph[vIn].state.set(ftr::StateOffset::Inactive, true);
  sendStateMessage(vIn, ftr::StateOffset::Inactive);
}

bool Stow::isFeatureActive(Vertex vIn)
{
  return !graph[vIn].state.test(ftr::StateOffset::Inactive);
}

bool Stow::isFeatureInactive(Vertex vIn)
{
  return graph[vIn].state.test(ftr::StateOffset::Inactive);
}

void Stow::setFeatureLeaf(Vertex vIn)
{
  if (isFeatureLeaf(vIn))
    return;
  graph[vIn].state.set(ftr::StateOffset::NonLeaf, false);
  sendStateMessage(vIn, ftr::StateOffset::NonLeaf);
}

void Stow::setFeatureNonLeaf(Vertex vIn)
{
  if (isFeatureNonLeaf(vIn))
    return;
  graph[vIn].state.set(ftr::StateOffset::NonLeaf, true);
  sendStateMessage(vIn, ftr::StateOffset::NonLeaf);
}

bool Stow::isFeatureLeaf(Vertex vIn)
{
  return !graph[vIn].state.test(ftr::StateOffset::NonLeaf);
}

bool Stow::isFeatureNonLeaf(Vertex vIn)
{
  return graph[vIn].state.test(ftr::StateOffset::NonLeaf);
}

void Stow::sendStateMessage(const Vertex &v, std::size_t stateOffset)
{
  ftr::Message fMessage(graph[v].feature->getId(), graph[v].state, stateOffset);
  msg::Message mMessage(msg::Response | msg::Project | msg::Feature | msg::Status);
  mMessage.payload = fMessage;
  observer->outBlocked(mMessage);
}


template <class GraphEW>
class Edge_writer {
public:
  Edge_writer(const GraphEW &graphEWIn) : graphEW(graphEWIn) {}
  template <class EdgeW>
  void operator()(std::ostream& out, const EdgeW& edgeW) const
  {
    out << "[label=\"";
    for (const auto &input : graphEW[edgeW].inputType.tags)
      out << input << "\\n";
    out << "\"]";
  }
private:
  const GraphEW &graphEW;
};

template <class GraphVW>
class Vertex_writer {
public:
  Vertex_writer(const GraphVW &graphVWIn) : graphVW(graphVWIn) {}
  template <class VertexW>
  void operator()(std::ostream& out, const VertexW& vertexW) const
  {
    out << 
        "[label=\"" <<
        graphVW[vertexW].feature->getName().toUtf8().data() << "\\n" <<
        gu::idToString(graphVW[vertexW].feature->getId()) << "\\n" <<
        "Descriptor: " << ftr::getDescriptorString(graphVW[vertexW].feature->getDescriptor()) << 
        "\"]";
  }
private:
  const GraphVW &graphVW;
};

template <class GraphIn>
void outputGraphviz(const GraphIn &graphIn, const std::string &filePath)
{
  std::ofstream file(filePath.c_str());
  boost::write_graphviz(file, graphIn, Vertex_writer<GraphIn>(graphIn),
                        Edge_writer<GraphIn>(graphIn));
}

void Stow::writeGraphViz(const std::string &fileName)
{
  outputGraphviz(graph, fileName);
}

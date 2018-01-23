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

#include <boost/uuid/uuid.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/reverse_graph.hpp>
#include <boost/graph/transpose_graph.hpp>
#include <boost/graph/copy.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>

#include <globalutilities.h>
#include <tools/idtools.h>
#include <tools/graphtools.h>
#include <project/serial/xsdcxxoutput/featurebase.h>
#include <feature/shapehistory.h>

using namespace ftr;

using boost::uuids::uuid;

struct VertexProperty
{
  VertexProperty() :
  featureId(gu::createNilId()),
  shapeId(gu::createNilId())
  {}
  
  VertexProperty(const uuid &featureIdIn, const uuid &shapeIdIn) :
  featureId(featureIdIn),
  shapeId(shapeIdIn)
  {}
  
  uuid featureId;
  uuid shapeId;
};
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, VertexProperty> Graph;
typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
typedef boost::graph_traits<Graph>::edge_descriptor Edge;
typedef boost::graph_traits<Graph>::vertex_iterator VertexIterator;
typedef boost::graph_traits<Graph>::edge_iterator EdgeIterator;
typedef boost::reverse_graph<Graph, Graph&> GraphReversed;
typedef boost::graph_traits<GraphReversed>::vertex_descriptor VertexReversed;

class VertexWriter {
public:
  VertexWriter(const Graph &graphIn) : graph(graphIn) {}
  template <class VertexW>
  void operator()(std::ostream& out, const VertexW& vertexW) const
  {
    out << 
    "[label=\"" <<
    "shapeId: " << gu::idToString(graph[vertexW].shapeId) << "\\n" <<
    "featureId: " << gu::idToString(graph[vertexW].featureId) << 
    "\"]";
  }
private:
  const Graph &graph;
};


//indexed map into graph
struct ShapeIdRecord
{
  uuid shapeId;
  Vertex graphVertex;
  
  ShapeIdRecord() :
  shapeId(gu::createNilId()),
  graphVertex(boost::graph_traits<Graph>::null_vertex())
  {}
  
  ShapeIdRecord(const uuid &shapeIdIn, const Vertex &vertexIn) :
  shapeId(shapeIdIn),
  graphVertex(vertexIn)
  {}
  
  //@{
  //! for tags
  struct ByShapeId{};
  //@}
};

using boost::multi_index_container;
using boost::multi_index::indexed_by;
using boost::multi_index::ordered_unique;
using boost::multi_index::tag;
using boost::multi_index::member;
typedef multi_index_container
<
  ShapeIdRecord,
  indexed_by
  <
    ordered_unique
    <
      tag<ShapeIdRecord::ByShapeId>,
      member<ShapeIdRecord, uuid, &ShapeIdRecord::shapeId>
    >
  >
> IdMap;

// std::ostream& operator<<(std::ostream&, const ShapeIdRecord&);
// std::ostream& operator<<(std::ostream&, const IdMap&);

namespace ftr
{
  class ShapeHistoryStow
  {
  public:
    Graph graph;
    IdMap idMap;
    
    bool hasShape(const boost::uuids::uuid &shapeIdIn) const
    {
      typedef IdMap::index<ShapeIdRecord::ByShapeId>::type List;
      const List &list = idMap.get<ShapeIdRecord::ByShapeId>();
      List::const_iterator it = list.find(shapeIdIn);
      return (it != list.end());
    }
    
    Vertex findVertex(const uuid &shapeIdIn) const
    {
      typedef IdMap::index<ShapeIdRecord::ByShapeId>::type List;
      const List &list = idMap.get<ShapeIdRecord::ByShapeId>();
      List::const_iterator it = list.find(shapeIdIn);
      return it->graphVertex;
    }
    
    void dumpIdMap() const
    {
      std::cout << std::endl << std::endl << "Shape history id map:" << std::endl;
      for (const auto &entry : idMap)
      {
        std::cout << "shape id: " << gu::idToString(entry.shapeId) << "      vertex: " << entry.graphVertex << std::endl;
      }
    }
    
    //Should I really be sorting this? wouldn't BFS be better if sorting at all? 
    std::vector<uuid> getAllIds() const
    {
      std::vector<Vertex> vertices;
      std::vector<uuid> out;
      
      boost::topological_sort(graph, std::back_inserter(vertices));
      
      for (auto rIt = vertices.rbegin(); rIt != vertices.rend(); ++rIt)
        out.push_back(graph[*rIt].shapeId);
      
      return out;
    }
  };
}

ShapeHistory::ShapeHistory() :
shapeHistoryStow(new ShapeHistoryStow())
{
}
ShapeHistory::ShapeHistory(std::shared_ptr<ShapeHistoryStow> stowIn) :
shapeHistoryStow(stowIn)
{
}

ShapeHistory::~ShapeHistory() {}

void ShapeHistory::clear()
{
  shapeHistoryStow->graph.clear();
  shapeHistoryStow->idMap.clear();
}

void ShapeHistory::writeGraphViz(const std::string &fileName) const
{
  std::ofstream file(fileName.c_str());
  boost::write_graphviz(file, shapeHistoryStow->graph, VertexWriter(shapeHistoryStow->graph));
  
  shapeHistoryStow->dumpIdMap();
}

void ShapeHistory::addShape(const uuid &featureIdIn, const uuid &shapeIdIn)
{
  Vertex v = boost::add_vertex(shapeHistoryStow->graph);
  shapeHistoryStow->graph[v] = VertexProperty(featureIdIn, shapeIdIn);
  
  shapeHistoryStow->idMap.insert(ShapeIdRecord(shapeIdIn, v));
}

void ShapeHistory::addConnection(const uuid &sourceShapeIdIn, const uuid &targetShapeIdIn)
{
  assert(shapeHistoryStow->hasShape(sourceShapeIdIn));
  assert(shapeHistoryStow->hasShape(targetShapeIdIn));
  
  boost::add_edge
  (
    shapeHistoryStow->findVertex(sourceShapeIdIn),
    shapeHistoryStow->findVertex(targetShapeIdIn),
    shapeHistoryStow->graph
  );
}

bool ShapeHistory::hasShape(const uuid &shapeIdIn) const
{
  return shapeHistoryStow->hasShape(shapeIdIn);
}

uuid ShapeHistory::devolve(const uuid &featureIdIn, const uuid &shapeIdIn) const
{
  assert(shapeHistoryStow->hasShape(shapeIdIn));
  
  Vertex v = shapeHistoryStow->findVertex(shapeIdIn);
  
  std::vector<Vertex> vertices;
  gu::BFSLimitVisitor<Vertex> lVisitor(vertices);
  boost::breadth_first_search(shapeHistoryStow->graph, v, boost::visitor(lVisitor));
  
  for (const auto &cVertex : vertices)
  {
    if (shapeHistoryStow->graph[cVertex].featureId == featureIdIn)
      return shapeHistoryStow->graph[cVertex].shapeId;
  }
  
  return gu::createNilId();
}

uuid ShapeHistory::evolve(const uuid &featureIdIn, const uuid &shapeIdIn) const
{
  assert(shapeHistoryStow->hasShape(shapeIdIn));
  
  Vertex v = shapeHistoryStow->findVertex(shapeIdIn);
  
  //find reverse connected vertices.
  std::vector<Vertex> vertices;
  GraphReversed rGraph = boost::make_reverse_graph(shapeHistoryStow->graph);
  gu::BFSLimitVisitor<VertexReversed> rVis(vertices);
  boost::breadth_first_search(rGraph, v, visitor(rVis));
  
  for (const auto &cVertex : vertices)
  {
    if (rGraph[cVertex].featureId == featureIdIn)
      return rGraph[cVertex].shapeId;
  }
  
  return gu::createNilId();
}


std::vector<boost::uuids::uuid> ShapeHistory::resolveHistories
(
  const ShapeHistory &pick,
  const boost::uuids::uuid &featureId
) const
{
  std::vector<boost::uuids::uuid> out;
  
  //find root of pick.
  Vertex v = boost::graph_traits<Graph>::null_vertex();
  for (auto its = boost::vertices(pick.shapeHistoryStow->graph); its.first != its.second; ++its.first)
  {
    if (boost::in_degree(*its.first, pick.shapeHistoryStow->graph) == 0)
    {
      v = *its.first;
      break;
    }
  }
  assert(v != boost::graph_traits<Graph>::null_vertex());
  if (v == boost::graph_traits<Graph>::null_vertex())
  {
    std::cerr << "WARNING: didn't find shape history pick root in ShapeHistory::resolveHistories" << std::endl;
    return out;
  }
  
  std::vector<Vertex> pickVertices;
  gu::BFSLimitVisitor<Vertex> lVisitor(pickVertices);
  boost::breadth_first_search(pick.shapeHistoryStow->graph, v, boost::visitor(lVisitor));
  assert(!pickVertices.empty());
  if (pickVertices.empty())
  {
    std::cerr << "WARNING: didn't find any pick vertices in ShapeHistory::resolveHistories" << std::endl;
    return out;
  }
  
  //anchorId is the first id in the pick history that is also in 'this' graph.
  uuid anchorId = gu::createNilId();
  for (const auto &pv : pickVertices)
  {
    uuid pid = pick.shapeHistoryStow->graph[pv].shapeId;
    if (!shapeHistoryStow->hasShape(pid))
      continue;
    anchorId = pid;
    break;
  }
  if (anchorId.is_nil())
    return out;
  
  Vertex anchorVertex = shapeHistoryStow->findVertex(anchorId);
  std::vector<Vertex> allRelatives;
  gu::BFSLimitVisitor<Vertex> aVisitor(allRelatives); //acendant visitor
  boost::breadth_first_search(shapeHistoryStow->graph, anchorVertex, boost::visitor(aVisitor));
  
  GraphReversed rGraph = boost::make_reverse_graph(shapeHistoryStow->graph);
  gu::BFSLimitVisitor<VertexReversed> dVisitor(allRelatives); //descendant visitor
  boost::breadth_first_search(rGraph, anchorVertex, boost::visitor(dVisitor));
  
  gu::uniquefy(allRelatives);
  
  for (const auto &vertex : allRelatives)
  {
    if (shapeHistoryStow->graph[vertex].featureId == featureId)
      out.push_back(shapeHistoryStow->graph[vertex].shapeId);
  }
  
  return out;
}



ShapeHistory ShapeHistory::createEvolveHistory(const uuid &shapeIdIn) const
{
  assert(shapeHistoryStow->hasShape(shapeIdIn));
  
  Vertex v = shapeHistoryStow->findVertex(shapeIdIn);
  
  //was having trouble copying the reversed and filtered graph so..
  //just use the reverse graph to accumulate vertices through BFS.
  GraphReversed rGraph = boost::make_reverse_graph(shapeHistoryStow->graph);
  std::vector<Vertex> vertices;
  gu::BFSLimitVisitor<Vertex> vis(vertices);
  boost::breadth_first_search(rGraph, v, visitor(vis));
  
  //filter the ORIGINAL graph on the accumulated vertexes.
  gu::SubsetFilter<Graph> vertexFilter(shapeHistoryStow->graph, vertices);
  typedef boost::filtered_graph<Graph, boost::keep_all, gu::SubsetFilter<Graph> > FilteredGraph;
  FilteredGraph filteredGraph(shapeHistoryStow->graph, boost::keep_all(), vertexFilter);
  
  std::shared_ptr<ShapeHistoryStow> stowOut(new ShapeHistoryStow());
  //need to transpose because we discard the reverse graph.
  boost::transpose_graph(filteredGraph, stowOut->graph);
  
  VertexIterator it, itEnd;
  std::tie(it, itEnd) = boost::vertices(stowOut->graph);
  for (; it != itEnd; ++it)
    stowOut->idMap.insert(ShapeIdRecord(stowOut->graph[*it].shapeId, *it));
  
  return ShapeHistory(stowOut);
}


ShapeHistory ShapeHistory::createDevolveHistory(const uuid &shapeIdIn) const
{
  assert(shapeHistoryStow->hasShape(shapeIdIn));
  
  Vertex v = shapeHistoryStow->findVertex(shapeIdIn);
  
  std::vector<Vertex> vertices;
  gu::BFSLimitVisitor<Vertex> vis(vertices);
  boost::breadth_first_search(shapeHistoryStow->graph, v, visitor(vis));
  
  //filter on the accumulated vertexes.
  gu::SubsetFilter<Graph> vertexFilter(shapeHistoryStow->graph, vertices);
  typedef boost::filtered_graph<Graph, boost::keep_all, gu::SubsetFilter<Graph> > FilteredGraph;
  FilteredGraph filteredGraph(shapeHistoryStow->graph, boost::keep_all(), vertexFilter);
  
  std::shared_ptr<ShapeHistoryStow> stowOut(new ShapeHistoryStow());
  boost::copy_graph(filteredGraph, stowOut->graph);
  VertexIterator it, itEnd;
  std::tie(it, itEnd) = boost::vertices(stowOut->graph);
  for (; it != itEnd; ++it)
    stowOut->idMap.insert(ShapeIdRecord(stowOut->graph[*it].shapeId, *it));
  
  return ShapeHistory(stowOut);
}

std::vector<boost::uuids::uuid> ShapeHistory::getAllIds() const
{
  return shapeHistoryStow->getAllIds();
}

prj::srl::ShapeHistory ShapeHistory::serialOut() const
{
  prj::srl::HistoryVertices vertsOut;
  VertexIterator it, itEnd;
  std::tie(it, itEnd) = boost::vertices(shapeHistoryStow->graph);
  for (; it != itEnd; ++it)
  {
    prj::srl::HistoryVertex vOut
    (
      gu::idToString(shapeHistoryStow->graph[*it].featureId),
      gu::idToString(shapeHistoryStow->graph[*it].shapeId)
    );
    vertsOut.array().push_back(vOut);
  }
  
  prj::srl::HistoryEdges edgesOut;
  EdgeIterator eIt, eItEnd;
  std::tie(eIt, eItEnd) = boost::edges(shapeHistoryStow->graph);
  for (; eIt != eItEnd; ++eIt)
  {
    prj::srl::HistoryEdge eOut
    (
      gu::idToString(shapeHistoryStow->graph[boost::source(*eIt, shapeHistoryStow->graph)].shapeId),
      gu::idToString(shapeHistoryStow->graph[boost::target(*eIt, shapeHistoryStow->graph)].shapeId)
    );
    edgesOut.array().push_back(eOut);
  }
  
  //idmap doesn't need serial support.
  
  return prj::srl::ShapeHistory
  (
    vertsOut,
    edgesOut
  );
}

void ShapeHistory::serialIn(const prj::srl::ShapeHistory &historyIn)
{
  for (const auto &sv : historyIn.vertices().array())
  {
    Vertex v = boost::add_vertex(shapeHistoryStow->graph);
    shapeHistoryStow->graph[v].featureId = gu::stringToId(sv.featureId());
    shapeHistoryStow->graph[v].shapeId = gu::stringToId(sv.shapeId());
    shapeHistoryStow->idMap.insert(ShapeIdRecord(shapeHistoryStow->graph[v].shapeId, v));
  }
  
  for (const auto &se : historyIn.edges().array())
  {
    Vertex source = shapeHistoryStow->findVertex(gu::stringToId(se.sourceShapeId()));
    Vertex target = shapeHistoryStow->findVertex(gu::stringToId(se.targetShapeId()));
    boost::add_edge(source, target, shapeHistoryStow->graph);
  }
}

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

#ifndef PRJ_STOW_H
#define PRJ_STOW_H

#include <memory>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/reverse_graph.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/uuid/uuid.hpp>

#include <feature/states.h>
#include <feature/inputtype.h>

namespace msg{class Message; class Observer;}
namespace ftr{class Base; namespace prm{class Parameter;}}

namespace prj
{
  struct VertexProperty
  {
    bool alive = true;
    std::shared_ptr<ftr::Base> feature;
    ftr::State state;
  };
  struct EdgeProperty
  {
    ftr::InputType inputType;
  };
  typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, VertexProperty, EdgeProperty> Graph;
  typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
  typedef boost::graph_traits<Graph>::edge_descriptor Edge;
  typedef boost::graph_traits<Graph>::vertex_iterator VertexIterator;
  typedef boost::graph_traits<Graph>::edge_iterator EdgeIterator;
  typedef std::vector<Vertex> Vertices;
  inline Vertex NullVertex()
  {
    return boost::graph_traits<Graph>::null_vertex();
  }
  
  class Stow
  {
  public:
    Stow();
    ~Stow();
    
    Vertex addFeature(std::shared_ptr<ftr::Base> feature);
    Edge connect(Vertex parentIn, Vertex childIn, const ftr::InputType &type);
    
    bool hasFeature(const boost::uuids::uuid &idIn) const;
    ftr::Base* findFeature(const boost::uuids::uuid &) const;
    Vertex findVertex(const boost::uuids::uuid&) const;
    std::vector<boost::uuids::uuid> getAllFeatureIds() const;
    ftr::prm::Parameter* findParameter(const boost::uuids::uuid &idIn) const;
    
    void writeGraphViz(const std::string &fileName);
    
    void setFeatureActive(Vertex);
    void setFeatureInactive(Vertex);
    bool isFeatureActive(Vertex);
    bool isFeatureInactive(Vertex);
    void setFeatureLeaf(Vertex);
    void setFeatureNonLeaf(Vertex);
    bool isFeatureLeaf(Vertex);
    bool isFeatureNonLeaf(Vertex);
    
    Graph graph;
  private:
    std::unique_ptr<msg::Observer> observer;
    void sendStateMessage(const Vertex&);
  };
  
  
  
  
  //works with straight adjacency iterator. if parent map wanted, pass in reversed graph.
  //dont forget to filter out removed features.
  template<typename G, typename V>
  ftr::UpdatePayload::UpdateMap buildAjacentUpdateMap(const G &gIn, const V &vIn)
  {
    ftr::UpdatePayload::UpdateMap out;
    for (auto its = boost::adjacent_vertices(vIn, gIn); its.first != its.second; ++its.first)
    {
      auto e = boost::edge(vIn, *its.first, gIn);
      assert(e.second);
      for (const auto &tag : gIn[e.first].inputType.tags)
        out.insert(std::make_pair(tag, gIn[*its.first].feature.get()));
    }
    return out;
  }
  
  
  
  template <typename G>
  struct RemovedFilter
  {
    RemovedFilter() : g(nullptr) { }
    RemovedFilter(const G &gIn) : g(&gIn) { }
    template <typename V>
    bool operator()(const V &vIn) const
    {
      if(!g)
        return false;
      return (*g)[vIn].alive;
    }
    const G *g = nullptr;
  };
  typedef boost::filtered_graph<Graph, boost::keep_all, RemovedFilter<Graph>>RemovedGraph;
  inline RemovedGraph buildRemovedGraph(Graph& gIn)
  {
    RemovedFilter<Graph> f(gIn);
    return RemovedGraph(gIn, boost::keep_all(), f);
  }
  
  typedef boost::reverse_graph<RemovedGraph, RemovedGraph&> ReversedGraph;
  
  
  //! simply collect connected vertices.
  template <typename VertexTypeIn>
  class AccrueVisitor : public boost::default_bfs_visitor
  {
  public:
    AccrueVisitor() = delete;
    AccrueVisitor(std::vector<VertexTypeIn> &vsIn) : vertices(vsIn){}
    template <typename GraphTypeIn>
    void discover_vertex(VertexTypeIn vertexIn, const GraphTypeIn &graphIn) const
    {
      vertices.push_back(vertexIn);
    }
    
    std::vector<VertexTypeIn> &vertices;
  };
  
  template <typename GraphTypeIn>
  struct ActiveFilter
  {
    ActiveFilter() : graph(nullptr) { }
    ActiveFilter(const GraphTypeIn &graphIn) : graph(&graphIn) { }
    template <typename VertexType>
    bool operator()(const VertexType& vertexIn) const
    {
      if(!graph)
        return false;
      return !(*graph)[vertexIn].state.test(ftr::StateOffset::Inactive);
    }
    const GraphTypeIn *graph;
  };
  
  template <typename GraphTypeIn>
  struct TargetEdgeFilter
  {
    TargetEdgeFilter() : graph(nullptr) {}
    TargetEdgeFilter(const GraphTypeIn &graphIn) : graph(&graphIn) {}
    template <typename EdgeType>
    bool operator()(const EdgeType& edgeIn) const
    {
      if (!graph)
        return false;
      return (*graph)[edgeIn].inputType.has(ftr::InputType::target);
    }
    const GraphTypeIn *graph;
  };
}

#endif // PRJ_STOW_H

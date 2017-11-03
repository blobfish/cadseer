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

#ifndef PROJECTGRAPH_H
#define PROJECTGRAPH_H

#include <vector>
#include <memory>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/reverse_graph.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/graphviz.hpp>

#include <tools/idtools.h>
#include <feature/inputtype.h>
#include <feature/base.h>

namespace prg
{
  struct VertexProperty
  {
    //boost graph will call destructor when expanding vector
    //so we need to keep from deleting when this happens.
    std::shared_ptr<ftr::Base> feature;
    ftr::State state;
  };
  /*! @brief needed to create an internal index for vertex. needed for listS.*/
  typedef boost::property<boost::vertex_index_t, std::size_t, VertexProperty> vertex_prop;

  struct EdgeProperty
  {
    ftr::InputType inputType;
  };
  /*! @brief needed to create an internal index for graph edges. needed for setS. Not needed upon implementation*/
  typedef boost::property<boost::edge_index_t, std::size_t, EdgeProperty> edge_prop;

  typedef boost::adjacency_list<boost::setS, boost::listS, boost::bidirectionalS, vertex_prop, edge_prop> Graph;
  typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
  typedef boost::graph_traits<Graph>::edge_descriptor Edge;
  typedef boost::graph_traits<Graph>::vertex_iterator VertexIterator;
  typedef boost::graph_traits<Graph>::edge_iterator EdgeIterator;
  typedef boost::graph_traits<Graph>::in_edge_iterator InEdgeIterator;
  typedef boost::graph_traits<Graph>::out_edge_iterator OutEdgeIterator;
  typedef boost::graph_traits<Graph>::adjacency_iterator VertexAdjacencyIterator;
  typedef boost::reverse_graph<Graph, Graph&> GraphReversed;
  typedef boost::graph_traits<GraphReversed>::vertex_descriptor VertexReversed;
  typedef std::vector<Vertex> Path;
  
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
    struct ActiveFilter {
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

#endif // PROJECTGRAPH_H

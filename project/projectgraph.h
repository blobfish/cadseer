#ifndef PROJECTGRAPH_H
#define PROJECTGRAPH_H

#include <vector>
#include <memory>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/reverse_graph.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/graphviz.hpp>

#include "../feature/inputtypes.h"
#include "../feature/base.h"

namespace ProjectGraph
{
  struct VertexProperty
  {
    //boost graph will call destructor when expanding vector
    //so we need to keep from deleting when this happens.
    std::shared_ptr<Feature::Base> feature;
  };

  struct EdgeProperty
  {
    Feature::InputTypes inputType;
  };

  typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, VertexProperty, EdgeProperty> Graph;
  typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
  typedef boost::graph_traits<Graph>::edge_descriptor Edge;
  typedef boost::graph_traits<Graph>::vertex_iterator VertexIterator;
  // typedef boost::graph_traits<Graph>::edge_iterator EdgeIterator;
  typedef boost::graph_traits<Graph>::in_edge_iterator InEdgeIterator;
  // typedef boost::graph_traits<Graph>::out_edge_iterator OutEdgeIterator;
  typedef boost::graph_traits<Graph>::adjacency_iterator VertexAdjacencyIterator;
  // typedef boost::reverse_graph<Graph, Graph&> GraphReversed;
  typedef std::vector<Vertex> Path;
  
  template <class GraphEW>
    class Edge_writer {
    public:
      Edge_writer(const GraphEW &graphEWIn) : graphEW(graphEWIn) {}
      template <class EdgeW>
      void operator()(std::ostream& out, const EdgeW& edgeW) const
      {
        out << "[label=\"";
        out << Feature::getInputTypeString(graphEW[edgeW].inputType);
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
        out << "[label=\"";
        out << graphVW[vertexW].feature->getTypeString().c_str(); 
        out << "\"]";
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

}

#endif // PROJECTGRAPH_H

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

#ifndef DAG_CONTROLLEDDAGDFS_H
#define DAG_CONTROLLEDDAGDFS_H

#include <tuple>
#include <vector>
#include <stack>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/iteration_macros.hpp>

namespace dag
{
  //! get all the roots of the templated graph.
  template <class GraphT>
  class DigRoots
  {
    typedef typename boost::graph_traits<GraphT>::vertex_descriptor VertexT;
    typedef typename std::vector<VertexT> VerticesT;
  public:
    DigRoots(const GraphT &graphIn) : graph(graphIn) {}
    VerticesT operator()() const
    {
      VerticesT out;
      BGL_FORALL_VERTICES_T(currentVertex, graph, GraphT)
      {
        if (boost::in_degree(currentVertex, graph) == 0)
          out.push_back(currentVertex);
      }
      return out;
    }
  private:
    const GraphT &graph;
  };
  
  template<class GraphT, class VisitorT>
  class ControlledDFS
  {
  public:
    typedef typename boost::graph_traits<GraphT>::vertex_descriptor VertexT;
    typedef typename boost::graph_traits<GraphT>::edge_descriptor EdgeT;
    typedef typename boost::graph_traits<GraphT>::adjacency_iterator AdjacencyIteratorT;
    typedef typename boost::graph_traits<GraphT>::in_edge_iterator InEdgeIteratorT;
    typedef typename std::vector<VertexT> VerticesT;
    // 0 = parent, 1 = vector of children, 2 = next child to process.
    typedef typename std::tuple<VertexT, VerticesT, std::size_t> IterationRecord;
    
    ControlledDFS(VisitorT &visitorIn) : graph(visitorIn.getGraph()), visitor(visitorIn) 
    {
      initialize();
      
      VerticesT startVertices = visitor.getStartVertices();
      if (startVertices.empty())
        startVertices = DigRoots<GraphT>(graph)();
      visitor.sortVertices(startVertices);
      for (auto currentVertex : startVertices)
      {
        VerticesT children = getSortedChildren(currentVertex);
        recordStack.push(std::make_tuple(currentVertex, children, 0));
        colorMap.at(currentVertex) = Color::Gray;
        visitor.discoverVertex(currentVertex);
        
        execute();
      }
    }
  private:
    GraphT &graph;
    VisitorT &visitor;
    typedef std::stack<IterationRecord> RecordStack;
    RecordStack recordStack;
    enum class Color
    {
      White,
      Gray,
      Black
    };
    typedef std::map<VertexT, Color> ColorMap;
    ColorMap colorMap;
    
    void execute()
    {
      while(!recordStack.empty())
      {
        auto currentParent = std::get<0>(recordStack.top());
        auto currentChildren = std::get<1>(recordStack.top());
        auto currentChildIt = std::get<2>(recordStack.top());
        recordStack.pop();
        
        while (currentChildIt < currentChildren.size())
        {
          if (colorMap.at(currentChildren.at(currentChildIt)) == Color::White)
          {
            auto currentChild = currentChildren.at(currentChildIt);
            currentChildIt++;
            
            if (!parentScan(currentChild)) //has parents not visited yet. don't continue;
              continue;
            
            recordStack.push(std::make_tuple(currentParent, currentChildren, currentChildIt));
            
            colorMap.at(currentChild) = Color::Gray;
            visitor.discoverVertex(currentChild);
            currentParent = currentChild;
            currentChildren = getSortedChildren(currentChild);
            currentChildIt = 0;
          }
          else //gray or black, we just skip.
          {
            currentChildIt++;
          }
        }
        colorMap.at(currentParent) = Color::Black;
      }
    }
    
    bool parentScan(VertexT childIn)
    {
      //if we are to continue no parent should be white.
      InEdgeIteratorT it, itEnd;
      std::tie(it, itEnd) = boost::in_edges(childIn, graph);
      for (; it != itEnd; ++it)
      {
        VertexT currentParent = boost::source(*it, graph);
        if (colorMap.at(currentParent) == Color::White)
          return false;
      }
      return true;
    }
    
    VerticesT getSortedChildren(VertexT parentIn)
    {
      VerticesT children;
      AdjacencyIteratorT it, itEnd;
      std::tie(it, itEnd) = boost::adjacent_vertices(parentIn, graph);
      for (; it != itEnd; ++it)
        children.push_back(*it);
      visitor.sortVertices(children);
      return children;
    }
    
    void initialize()
    {
      BGL_FORALL_VERTICES_T(currentVertex, graph, GraphT)
      {
        colorMap.insert(std::make_pair(currentVertex, Color::White));
      }
    }
  };
  
  template<class GraphT>
  class TopoSortVisitor
  {
  public:
    typedef typename boost::graph_traits<GraphT>::vertex_descriptor VertexT;
    typedef typename boost::graph_traits<GraphT>::edge_descriptor EdgeT;
    typedef typename std::vector<VertexT> VerticesT;
    typedef typename std::vector<VertexT>::iterator VerticesTIterator;
    typedef typename std::tuple<VerticesT, VerticesTIterator, VerticesTIterator> IterationRecord;
    
    TopoSortVisitor(GraphT &graphIn) : graph(graphIn){}
    GraphT& getGraph(){return graph;}
    
    /*! @brief vertices to initiate search
     * 
     * if empty, ControlledDFS will "dig roots".
     * sortVertices will be called.
     */
    VerticesT getStartVertices()
    {
      return VerticesT();
    }
    
    VerticesT getResults(){return results;}
    
    /*! @brief vertex sort
     * 
     * called from dfs to give the visitor a chance for an ordered
     * iteration over child vertices.
     */
    void sortVertices(VerticesT &)
    {
      //nothing for now.
    }
    
    void discoverVertex(VertexT &vertexIn)
    {
      results.push_back(vertexIn);
    }
    
  private:
    GraphT &graph;
    VerticesT results;
    
  };
}

#endif // DAG_CONTROLLEDDAGDFS_H

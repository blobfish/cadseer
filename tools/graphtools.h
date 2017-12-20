/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef GU_GRAPHTOOLS_H
#define GU_GRAPHTOOLS_H

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/visitors.hpp>


namespace gu
{
  //! Combined with a BFS to accumulate related vertices
  template <typename VertexTypeIn>
  class BFSLimitVisitor : public boost::default_bfs_visitor
  {
  public:
    BFSLimitVisitor(std::vector<VertexTypeIn> &verticesIn): vertices(verticesIn) {}
    template <typename GraphTypeIn>
    void discover_vertex(VertexTypeIn vertexIn, const GraphTypeIn &) const
    {
      vertices.push_back(vertexIn);
    }
  private:
    std::vector<VertexTypeIn> &vertices;
  };

  template <typename GraphTypeIn>
  struct SubsetFilter {
    typedef typename boost::graph_traits<GraphTypeIn>::vertex_descriptor VertexTypeIn;
    SubsetFilter() : graph(nullptr), vertices(nullptr){ }
    SubsetFilter(const GraphTypeIn &graphIn, std::vector<VertexTypeIn> &verticesIn) :
    graph(&graphIn), vertices(&verticesIn){ }
    template <typename VertexType>
    bool operator()(const VertexType& vertexIn) const
    {
      if(!graph)
        return false;
      if (std::find(vertices->begin(), vertices->end(), vertexIn) == vertices->end())
        return false;
      return true;
    }
    const GraphTypeIn *graph;
    const std::vector<VertexTypeIn> *vertices;
  };
  
  /*! @brief Cached data for each vertex of search.
  * 
  */
  template <typename G>
  class Node
  {
  public:
    typedef typename boost::graph_traits<G>::vertex_descriptor V;
    typedef typename boost::graph_traits<G>::edge_descriptor E;
    
    Node() = delete;
    Node(V vIn, const G &gIn) : g(gIn), v(vIn), ei(0)
    {
      for (auto pairIt = boost::out_edges(v, g); pairIt.first != pairIt.second; ++pairIt.first)
        oes.push_back(*pairIt.first);
    }
    
    V getVertex() const {return v;}
    E getCurrentEdge() const {assert(isValid()); return oes.at(ei);}
    V getCurrentTarget() const{assert(isValid()); return boost::target(oes.at(ei), g);}
    
    bool isValid() const {return ei < oes.size();} //!< currently pointing to a valid edge.
    void operator ++(int){ei++;} //!< bump to next out edge.
    
  private:
    const G &g;
    V v;
    std::size_t ei; //!< edge index into vector. Not using iterators as this structure will be copied around.
    std::vector<E> oes; //!< out edges
  };

  /*! @brief Accumulate all paths between 1 root and 1 vertex.
  * 
  * use filtered graphs if needed.
  * 
  */
  template <typename G>
  class PathSearch
  {
  public:
    typedef typename boost::graph_traits<G>::vertex_descriptor V;
    typedef typename boost::graph_traits<G>::edge_descriptor E;
    
    PathSearch() = delete;
    PathSearch(const G &gIn) : g(gIn)
    {
      std::vector<V> roots;
      std::vector<V> leaves;
      for (auto vits = boost::vertices(g); vits.first != vits.second; ++vits.first)
      {
        if (boost::in_degree(*vits.first, g) == 0)
          roots.push_back(*vits.first);
        if (boost::out_degree(*vits.first, g) == 0)
          leaves.push_back(*vits.first);
      }
      
      //we only support 1 root and 1 leave. assert in debug, do nothing in release.
      assert(roots.size() == 1);
      assert(leaves.size() == 1);
      if (roots.size() != 1 || leaves.size() != 1)
        return;
      
      root = roots.front();
      leaf = leaves.front();
      assert(root != leaf); //can't be the same vertex.
      if (root == leaf)
        return;
      Node<G> rn(root, g);
      stack.push_back(rn);
      
      while (!stack.empty())
      {
        if (!stack.back().isValid()) //done with this node
        {
          stack.pop_back();
          if (!stack.empty())
            stack.back()++; //increment parent.
          continue;
        }
        V cv = stack.back().getCurrentTarget();
        Node<G> cn(cv, g);
        stack.push_back(cn);
        if (cv == leaf)
        {
          std::vector<V> tp; //temp path.
          for (const auto &n : stack)
            tp.push_back(n.getVertex());
          paths.push_back(tp);
        }
      }
    }
    
    const std::vector<std::vector<V>>& getPaths(){return paths;}
    
  private:
    const G &g;
    V root;
    V leaf;
    
    std::vector<Node<G>> stack;
    std::vector<std::vector<V>> paths;
  };
}

#endif //GU_GRAPHTOOLS_H

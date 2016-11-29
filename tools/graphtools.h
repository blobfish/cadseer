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
}

#endif //GU_GRAPHTOOLS_H

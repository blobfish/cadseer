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

#ifndef CONNECTORGRAPH_H
#define CONNECTORGRAPH_H

#include <map>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/reverse_graph.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/uuid/uuid.hpp>

#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS_Shape.hxx>

namespace ConnectorGraph
{
struct VertexProperty
{
    boost::uuids::uuid id;
    TopAbs_ShapeEnum shapeType;
    TopoDS_Shape shape;
};

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, VertexProperty> Graph;
typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
typedef boost::graph_traits<Graph>::edge_descriptor Edge;
typedef boost::graph_traits<Graph>::vertex_iterator VertexIterator;
typedef boost::graph_traits<Graph>::edge_iterator EdgeIterator;
typedef boost::graph_traits<Graph>::in_edge_iterator InEdgeIterator;
typedef boost::graph_traits<Graph>::out_edge_iterator OutEdgeIterator;
typedef boost::graph_traits<Graph>::adjacency_iterator VertexAdjacencyIterator;
typedef boost::reverse_graph<Graph, Graph&> GraphReversed;

typedef std::map<boost::uuids::uuid, Vertex> IdVertexMap;

class TypeCollectionVisitor : public boost::default_bfs_visitor
{
public:
    TypeCollectionVisitor(const TopAbs_ShapeEnum &shapeTypeIn, std::vector<Vertex> &vertIn) : shapeType(shapeTypeIn),
        typedVertices(vertIn){}
    template <typename VisitorVertex, typename VisitorGraph>
    void discover_vertex(VisitorVertex u, const VisitorGraph &g)
    {
        if (g[u].shapeType == this->shapeType)
            typedVertices.push_back(u);
    }

private:
    TopAbs_ShapeEnum shapeType;
    std::vector<Vertex> &typedVertices;
};

static const std::vector<std::string> shapeStrings
({
     "Compound",
     "Compound Solid",
     "Solid",
     "Shell",
     "Face",
     "Wire",
     "Edge",
     "Vertex",
     "Shape",
 });


template <class GraphTypeIn>
class Node_writer {
public:
  Node_writer(GraphTypeIn graphIn) : graph(graphIn) {}
  template <class NodeW>
  void operator()(std::ostream& out, const NodeW& v) const {
    out << 
      "[label=\"" <<
      shapeStrings.at(static_cast<int>(graph[v].shapeType)) << "\\n" <<
      graph[v].id <<
      "\"]";
  }
private:
  const GraphTypeIn &graph;
};

}

#endif // CONNECTORGRAPH_H

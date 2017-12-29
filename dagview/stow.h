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

#ifndef DAG_STOW_H
#define DAG_STOW_H

#include <memory>
#include <bitset>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/reverse_graph.hpp>
#include <boost/uuid/uuid.hpp>

#include <feature/inputtype.h>
#include <dagview/rectitem.h>

namespace dag
{
  namespace qtd
  {
    const static int key =              0;
    const static int rectangle =        1;
    const static int point =            2;
    const static int visibleIcon =      3;
    const static int overlayIcon =      4;
    const static int stateIcon =        5;
    const static int featureIcon =      6;
    const static int text =             7;
    const static int connector =        8;
  }
  
  //limit of column width? boost::dynamic_bitset?
  //did a trial run with this set at 4096, not much difference.
  //going to leave a big number by default and see how it goes.
  typedef std::bitset<1024> ColumnMask;
  
  /*! @brief Graph vertex information
  *
  * My data stored for each vertex;
  */
  struct VertexProperty
  {
    VertexProperty();
    boost::uuids::uuid featureId;  //? possibly for multi_index.
    ColumnMask columnMask; //!< column number containing the point.
    std::size_t row;
    std::size_t sortedIndex; //for index into toposorted list.
    ftr::State state;
    bool dagVisible; //!< should entry be visible in the DAG view.
    bool alive;
    bool hasSeerShape; //!< to show check geometry context menu entry.
    
    std::shared_ptr<RectItem> rectShared;
    std::shared_ptr<QGraphicsEllipseItem> pointShared;
    std::shared_ptr<QGraphicsPixmapItem> visibleIconShared;
    std::shared_ptr<QGraphicsPixmapItem> overlayIconShared;
    std::shared_ptr<QGraphicsPixmapItem> stateIconShared;
    std::shared_ptr<QGraphicsPixmapItem> featureIconShared;
    std::shared_ptr<QGraphicsTextItem> textShared;
  };

  
  /*! @brief Graph edge information
  *
  * My data stored for each edge;
  */
  struct EdgeProperty
  {
    EdgeProperty();
    std::shared_ptr <QGraphicsPathItem> connector; //!< line representing link between nodes.
    ftr::InputType inputType;
  };
  
  typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, VertexProperty, EdgeProperty> Graph;
  typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
  typedef boost::graph_traits<Graph>::edge_descriptor Edge;
  typedef boost::graph_traits<Graph>::vertex_iterator VertexIterator;
  typedef boost::graph_traits<Graph>::edge_iterator EdgeIterator;
  typedef boost::graph_traits<Graph>::in_edge_iterator InEdgeIterator;
  typedef boost::graph_traits<Graph>::out_edge_iterator OutEdgeIterator;
  typedef boost::graph_traits<Graph>::adjacency_iterator VertexAdjacencyIterator;
  typedef boost::reverse_graph<Graph, Graph&> GraphReversed;
  typedef std::vector<Vertex> Path; //!< a path or any array of vertices
  inline Vertex NullVertex(){return boost::graph_traits<Graph>::null_vertex();}
  
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
        graphVW[vertexW].textShared->toPlainText().toUtf8().data() << "\\n" <<
        gu::idToString(graphVW[vertexW].featureId) <<
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
  
  class Stow
  {
  public:
    Stow();
    ~Stow();
    
    void writeGraphViz(const std::string &fileName){outputGraphviz<Graph>(graph, fileName);}
    Vertex findVertex(const boost::uuids::uuid&);
    Vertex findVertex(const RectItem*);
    Vertex findVertex(const QGraphicsEllipseItem*);
    Vertex findVisibleVertex(const QGraphicsPixmapItem*);
    Vertex findOverlayVertex(const QGraphicsPixmapItem*);
    Edge findEdge(const QGraphicsPathItem *);
    
    std::vector<Vertex> getAllSelected();
    std::vector<QGraphicsItem*> getAllSceneItems(Vertex);
    float connectionOffset(Vertex, Edge); //!< parameterized between [-1.0, 1.0]. zero means center of point.
    void highlightConnectorOn(Edge, const QColor&);
    void highlightConnectorOn(QGraphicsPathItem *, const QColor&);
    void highlightConnectorOff(Edge);
    void highlightConnectorOff(QGraphicsPathItem *);
    std::vector<Edge> getAllEdges(Vertex); //!< all edges connected to a vertex, both in an out.
    
    
    std::pair<std::vector<Vertex>, std::vector<Edge>> getDropAccepted(Vertex); //!< find connected vertices that vertex can be dropped upon.
    
    Graph graph;
  };
  
  
  
//   
//   //! get all the leaves of the templated graph. Not used right now.
//   template <class GraphIn>
//   class RakeLeaves
//   {
//     typedef boost::graph_traits<Graph>::vertex_descriptor GraphInVertex;
//     typedef std::vector<GraphInVertex> GraphInVertices;
//   public:
//     RakeLeaves(const GraphIn &graphIn) : graph(graphIn) {}
//     GraphInVertices operator()() const
//     {
//       GraphInVertices out;
//       BGL_FORALL_VERTICES_T(currentVertex, graph, GraphIn)
//       {
//         if (boost::out_degree(currentVertex, graph) == 0)
//           out.push_back(currentVertex);
//       }
//       return out;
//     }
//   private:
//     const GraphIn &graph;
//   };
//   

//   
//   /*! @brief Get connected components.
//   */
//   class ConnectionVisitor : public boost::default_bfs_visitor
//   {
//   public:
//     ConnectionVisitor(std::vector<Vertex> &verticesIn) : vertices(verticesIn){}
//     
//     template<typename TVertex, typename TGraph>
//     void discover_vertex(TVertex vertex, TGraph &graph)
//     {
//       vertices.push_back(vertex);
//     }
//   private:
//     std::vector<Vertex> &vertices;
//   };
}

#endif // DAG_STOW_H

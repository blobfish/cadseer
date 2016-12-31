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

#ifndef DAGMODELGRAPH_H
#define DAGMODELGRAPH_H

#include <memory>
#include <bitset>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/reverse_graph.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>

#include <tools/idtools.h>
#include <feature/base.h>
#include <feature/inputtypes.h>
#include <dagview/dagrectitem.h>

using boost::uuids::uuid;

namespace dag
{
  namespace qtd
  {
    const static int key =              0;
    const static int rectangle =        1;
    const static int point =            2;
    const static int visibleIcon =      3;
    const static int stateIcon =        4;
    const static int featureIcon =      5;
    const static int text =             6;
    const static int connector =        7;
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
    std::weak_ptr<ftr::Base> feature;
    uuid featureId;  //? possibly for multi_index.
    ColumnMask columnMask; //!< column number containing the point.
    std::size_t row;
    std::size_t sortedIndex; //for index into toposorted list.
    bool dagVisible; //!< should entry be visible in the DAG view.
    
    //keeping raw pointer for indexing.
    std::shared_ptr<RectItem> rectShared;
    RectItem *rectRaw;
    std::shared_ptr<QGraphicsEllipseItem> pointShared;
    QGraphicsEllipseItem *pointRaw;
    std::shared_ptr<QGraphicsPixmapItem> visibleIconShared;
    QGraphicsPixmapItem *visibleIconRaw;
    std::shared_ptr<QGraphicsPixmapItem> stateIconShared;
    QGraphicsPixmapItem *stateIconRaw;
    std::shared_ptr<QGraphicsPixmapItem> featureIconShared;
    QGraphicsPixmapItem *featureIconRaw;
    std::shared_ptr<QGraphicsTextItem> textShared;
    QGraphicsTextItem *textRaw;
    
    
    //@{
    //! used as tags. All indexes on raw pointers
    struct ByFeatureId{};
    struct ByRect{}; 
    struct ByPoint{}; 
    struct ByVisibleIcon{};
    struct ByStateIcon{};
    struct ByFeatureIcon{};
    struct ByText{};
    //@}
    
  };
  /*! @brief needed to create an internal index for vertex. needed for listS.*/
  typedef boost::property<boost::vertex_index_t, std::size_t, VertexProperty> vertex_prop;
  
  /*! @brief Graph edge information
  *
  * My data stored for each edge;
  */
  struct EdgeProperty
  {
    EdgeProperty();
    std::shared_ptr <QGraphicsPathItem> connector; //!< line representing link between nodes.
    ftr::InputTypes inputType;
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
  typedef std::vector<Vertex> Path; //!< a path or any array of vertices
  
  template <class GraphEW>
  class Edge_writer {
  public:
    Edge_writer(const GraphEW &graphEWIn) : graphEW(graphEWIn) {}
    template <class EdgeW>
    void operator()(std::ostream& out, const EdgeW& edgeW) const
    {
      out << 
        "[label=\"" <<
        getInputTypeString(graphEW[edgeW].inputType) <<
        "\"]";
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
        graphVW[vertexW].textRaw->toPlainText().toAscii().data() << "\\n" <<
        gu::idToString(graphVW[vertexW].feature.lock()->getId()) <<
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
  
  namespace BMI = boost::multi_index;
  typedef boost::multi_index_container
  <
    VertexProperty,
    BMI::indexed_by
    <
      BMI::ordered_unique
      <
        BMI::tag<VertexProperty::ByFeatureId>,
        BMI::member<VertexProperty, uuid, &VertexProperty::featureId>
      >,
      BMI::ordered_unique
      <
        BMI::tag<VertexProperty::ByRect>,
        BMI::member<VertexProperty, RectItem *, &VertexProperty::rectRaw>
      >,
      BMI::ordered_unique
      <
        BMI::tag<VertexProperty::ByPoint>,
        BMI::member<VertexProperty, QGraphicsEllipseItem*, &VertexProperty::pointRaw>
      >,
      BMI::ordered_unique
      <
        BMI::tag<VertexProperty::ByVisibleIcon>,
        BMI::member<VertexProperty, QGraphicsPixmapItem*, &VertexProperty::visibleIconRaw>
      >,
      BMI::ordered_unique
      <
        BMI::tag<VertexProperty::ByStateIcon>,
        BMI::member<VertexProperty, QGraphicsPixmapItem*, &VertexProperty::stateIconRaw>
      >,
      BMI::ordered_unique
      <
        BMI::tag<VertexProperty::ByFeatureIcon>,
        BMI::member<VertexProperty, QGraphicsPixmapItem*, &VertexProperty::featureIconRaw>
      >,
      BMI::ordered_unique
      <
        BMI::tag<VertexProperty::ByText>,
        BMI::member<VertexProperty, QGraphicsTextItem*, &VertexProperty::textRaw>
      >
    >
  > GraphLinkContainer;
  
  const VertexProperty& findRecordByVisible(const GraphLinkContainer &containerIn, QGraphicsPixmapItem *itemIn);
  const VertexProperty& findRecord(const GraphLinkContainer &containerIn, const uuid &idIn);
  void eraseRecord(GraphLinkContainer &containerIn, const uuid &idIn);
  const VertexProperty& findRecord(const GraphLinkContainer &containerIn, RectItem* rectIn);
  void clear(GraphLinkContainer &containerIn);
  
  struct VertexIdRecord
  {
    uuid featureId;
    Vertex vertex;
    
    //@{
    //! used as tags. All indexes on raw pointers
    struct ByFeatureId{};
    struct ByVertex{};
    //@}
  };
  
  typedef boost::multi_index_container
  <
    VertexIdRecord,
    BMI::indexed_by
    <
      BMI::ordered_unique
      <
        BMI::tag<VertexIdRecord::ByFeatureId>,
        BMI::member<VertexIdRecord, uuid, &VertexIdRecord::featureId>
      >,
      BMI::ordered_unique
      <
        BMI::tag<VertexIdRecord::ByVertex>,
        BMI::member<VertexIdRecord, Vertex, &VertexIdRecord::vertex>
      >
    >
  > VertexIdContainer;
  
  inline const VertexIdRecord& findRecord(const VertexIdContainer &containerIn, const uuid &featureIdIn)
  {
    typedef VertexIdContainer::index<VertexIdRecord::ByFeatureId>::type List;
    const List &list = containerIn.get<VertexIdRecord::ByFeatureId>();
    List::const_iterator it = list.find(featureIdIn);
    assert(it != list.end());
    return *it;
  }
  
  inline const VertexIdRecord& findRecord(const VertexIdContainer &containerIn, const Vertex &featureIdIn)
  {
    typedef VertexIdContainer::index<VertexIdRecord::ByVertex>::type List;
    const List &list = containerIn.get<VertexIdRecord::ByVertex>();
    List::const_iterator it = list.find(featureIdIn);
    assert(it != list.end());
    return *it;
  }
  
  inline void eraseRecord(VertexIdContainer &containerIn, const uuid &featureIdIn)
  {
    typedef VertexIdContainer::index<VertexIdRecord::ByFeatureId>::type List;
    List &list = containerIn.get<VertexIdRecord::ByFeatureId>();
    List::const_iterator it = list.find(featureIdIn);
    assert(it != list.end());
    list.erase(it);
  }
  
  inline void clear(VertexIdContainer &containerIn)
  {
    typedef VertexIdContainer::index<VertexIdRecord::ByFeatureId>::type List;
    List &list = containerIn.get<VertexIdRecord::ByFeatureId>();
    list.clear();
  }
}

#endif // DAGMODELGRAPH_H

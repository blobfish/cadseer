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

#ifndef EXPRESSIONGRAPH_H
#define EXPRESSIONGRAPH_H

#include <memory>
#include <stack>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/breadth_first_search.hpp>

#include <expressions/abstractnode.h>
#include <expressions/expressionedgeproperty.h>

namespace expr{

//! @brief Used for graph definiton.
typedef std::unique_ptr<AbstractNode> VertexProperty;

typedef boost::property<boost::vertex_index_t, std::size_t, VertexProperty> vertex_prop;
typedef boost::property<boost::edge_index_t, std::size_t, EdgeProperty::EdgePropertyType> edge_prop;

/*! @brief The graph definition for containment of all abstract node decendents.
 * 
 * This is the data structure at the heart of the expression evaluations.
 * @see VertexProperty @see EdgeProperty::EdgePropertyType
 */
typedef boost::adjacency_list<boost::multisetS, boost::listS, boost::bidirectionalS, vertex_prop, edge_prop> Graph;
typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
typedef boost::graph_traits<Graph>::edge_descriptor Edge;
typedef boost::graph_traits<Graph>::edge_iterator EdgeIterator;
typedef boost::graph_traits<Graph>::vertex_iterator VertexIterator;
typedef boost::graph_traits<Graph>::in_edge_iterator InEdgeIterator;
typedef boost::graph_traits<Graph>::out_edge_iterator OutEdgeIterator;
typedef boost::graph_traits<Graph>::adjacency_iterator VertexAdjacencyIterator;

/*! @brief Container for the boost graph.
 * 
 * This object contains and interfaces with the graph. This allows us to forward declare
 * and isolate the graph definition for build speed.
 * 
 * There are 3 ways to 
 * refer to a formula node: a boost::graph vertex, a std::string name, and a boost uuid.
 * 
 * vertex is part of the graph defintion is kept close as possible to the graph to help isolate graph definition.
 * Name and Ids are transformed into a vertex.
 * 
 * Name is used as little as possible and only for user interface needs.
 * 
 * boost uuid is used outside the graph to reference the formulas. This is used in groups and for value change notifications.
 * and anywhere else we want to reference a specific formula but not pull in the entire graph definition. Using
 * the name for this would be to volatile.
 */
class GraphWrapper
{
public:
  //! @brief Default constructor.
  GraphWrapper();
  ~GraphWrapper();
  //! The actual boost graph object.
  Graph graph;
  
  //! Write out a graphviz(.dot) file.
  void writeOutGraph(const std::string &pathName);
  //! Dependency sort and recompute update nodes.
  void update();
  
  /*! @brief index the vertices for algorithms
   */
  void indexVerticesAndEdges();
  
  /*! @brief Construct EdgePropertiesMap from the graph.
   * 
   * Passed into each nodes calculate override.
   */
  EdgePropertiesMap buildEdgePropertiesMap(const Vertex &source) const;
  
  //@{
  //! Testing if a formula exists.
  bool hasFormula(const std::string &nameIn) const;
  bool hasFormula(const boost::uuids::uuid &idIn) const;
  //@}
  
  //@{
  /*!* @brief Get graph vertex.
   * 
   * Formula must exists or an assert. @see hasFormula
   */
  Vertex getFormulaVertex(const std::string &nameIn) const;
  Vertex getFormulaVertex(const boost::uuids::uuid &idIn) const;
  //@}
  
  /*! @brief Get id from vertex. */
  boost::uuids::uuid getId(const Vertex &vertexIn) const;
  /*! @brief Get formula id from name. */
  boost::uuids::uuid getFormulaId(const std::string &nameIn) const;

  //@{
  /*! @brief Clean formula.
   * 
   * Removes all dependent, non-formula nodes and edges. Leaves the base formula node.
   */
  void cleanFormula(const boost::uuids::uuid &idIn);
  void cleanFormula(const Vertex &vertexIn);
  //@}
  
  //@{
  /*! @brief Remove formula.
   * 
   * Any dependent formula will be replaced by a constant node of formulas current value.
   * Don't call these directly. Go through the manager so groups will be updated. @see ExpressionManager::removeFormula
   */
  void removeFormula(const boost::uuids::uuid &idIn);
  void removeFormula(const Vertex &vertexIn);
  //@}
  
  //@{
  //! @brief Get formula name.
  std::string getFormulaName(const Vertex &vIn) const;
  std::string getFormulaName(const boost::uuids::uuid &idIn) const;
  //@}
  
  //! @brief Set the formula name with the id
  void setFormulaName(const boost::uuids::uuid &idIn, const std::string &nameIn);
  
  //@{
  //! @brief Get the formula value.
  double getFormulaValue(const Vertex &vIn) const;
  double getFormulaValue(const boost::uuids::uuid &idIn) const;
  //@}
  
  /*! @brief Get a vector of all the formula names.
   * 
   * Used to populate any gui items with a list of available formulas.
   */
  std::vector<std::string> getAllFormulaNames() const;
  
  //! @brief Get all the formula ids.
  std::vector<boost::uuids::uuid> getAllFormulaIds() const;
  
  /*! @brief Get all formula ids sorted.
   * 
   * The sorting is determined by the graph dependency. This is used for
   * exporting the formulas. Thus upon import the creation order will ensure
   * that dependent formulas will exist.
   */
  std::vector<boost::uuids::uuid> getAllFormulaIdsSorted();
  
  /*! @brief Get a vector of dependent ids.
   * 
   * Out vector will contain the passed in parent.
   */
  std::vector<boost::uuids::uuid> getDependentFormulaIds(const boost::uuids::uuid &parentIn);
  
  //@{
  //! @brief Set dependent nodes dirty.
  void setFormulaDependentsDirty(const boost::uuids::uuid &idIn);
  void setFormulaDependentsDirty(const Vertex &vertexIn);
  //@}
  
  //! @brief Set all the graph nodes dirty.
  void setAllDirty();
  
  
  //@{
  //! @brief Build typed nodes.
  template<typename T> Vertex buildNode()
  {
    Vertex vertex = boost::add_vertex(graph);
    graph[vertex] = std::move(std::unique_ptr<T>(new T()));
    return vertex;
  }
  Vertex buildConstantNode(const double &constantIn);
  Vertex buildFormulaNode(const std::string& stringIn);
  Vertex buildAdditionNode(){return buildNode<AdditionNode>();}
  Vertex buildSubractionNode(){return buildNode<SubtractionNode>();}
  Vertex buildMultiplicationNode(){return buildNode<MultiplicationNode>();}
  Vertex buildDivisionNode(){return buildNode<DivisionNode>();}
  Vertex buildParenthesesNode(){return buildNode<ParenthesesNode>();}
  Vertex buildSinNode(){return buildNode<SinNode>();}
  Vertex buildCosNode(){return buildNode<CosNode>();}
  Vertex buildTanNode(){return buildNode<TanNode>();}
  Vertex buildAsinNode(){return buildNode<AsinNode>();}
  Vertex buildAcosNode(){return buildNode<AcosNode>();}
  Vertex buildAtanNode(){return buildNode<AtanNode>();}
  Vertex buildAtan2Node(){return buildNode<Atan2Node>();}
  Vertex buildPowNode(){return buildNode<PowNode>();}
  Vertex buildAbsNode(){return buildNode<AbsNode>();}
  Vertex buildMinNode(){return buildNode<MinNode>();}
  Vertex buildMaxNode(){return buildNode<MaxNode>();}
  Vertex buildFloorNode(){return buildNode<FloorNode>();}
  Vertex buildCeilNode(){return buildNode<CeilNode>();}
  Vertex buildRoundNode(){return buildNode<RoundNode>();}
  Vertex buildRadToDegNode(){return buildNode<RadToDegNode>();}
  Vertex buildDegToRadNode(){return buildNode<DegToRadNode>();}
  Vertex buildLogNode(){return buildNode<LogNode>();}
  Vertex buildExpNode(){return buildNode<ExpNode>();}
  Vertex buildSqrtNode(){return buildNode<SqrtNode>();}
  Vertex buildHypotNode(){return buildNode<HypotNode>();}
  Vertex buildConditionalNode(){return buildNode<ConditionalNode>();}
  //@}
  
  //! @brief Build a graph edge.
  Edge buildEdge(const Vertex &sourceIn, const Vertex &targetIn);
  
  /*! @brief detects a cycle in the graph
   * 
   * @param idIn id of the formula to start from
   * @param nameOut name of the formula that branches from idIn responsible
   * for creating the cyle.
   */
  bool hasCycle(const boost::uuids::uuid &idIn, std::string &nameOut);
};

//! @cond
template <class GraphEW>
class Edge_writer {
public:
  Edge_writer(const GraphEW &graphEWIn) : graphEW(graphEWIn) {}
  template <class EdgeW>
  void operator()(std::ostream& out, const EdgeW& edgeW) const {
    out << "[label=\"";
    out << getEdgePropertyString(graphEW[edgeW]) << "\\n";
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
  void operator()(std::ostream& out, const VertexW& vertexW) const {
    out << "[label=\"";
    if (graphVW[vertexW]->getType() == NodeType::Formula)
    {
      FormulaNode *fNode = static_cast<FormulaNode *>(graphVW[vertexW].get());
      out << fNode->name;
    }
    else
      out << graphVW[vertexW]->className();
    out  << "   " << graphVW[vertexW]->getValue() << "\\n";
    out << "\"";
    if (graphVW[vertexW]->getType() == NodeType::Formula)
    {
      out << "color=aquamarine style=filled";
    }
    out << "]";
  }
private:
  const GraphVW &graphVW;
};
//! @endcond

/*! @brief Depth first search
 * 
 * Traverses child vertices of a formulaNode. Will visit dependent formulaNodes,
 * but not their children. This is used to erase non-formula, dependent nodes and edges.
 * Original formula node will be left. This is used in update of expression.
 */
class FormulaCleanVisitor : public boost::default_dfs_visitor
{
public:
    FormulaCleanVisitor(std::vector<Vertex> &verticesIn, std::vector<Edge> &edgesIn) :
      completedTest(false), verticesOut(verticesIn), edgesOut(edgesIn)
    {

    }
    
    template<typename FindVertex, typename FindGraph>
    void start_vertex(FindVertex vertex, FindGraph &graph)
    {
      /* from the doc: "This is invoked on the source vertex once before the start of the search."
       * this is misleading. What I am seeing here is this is invoked on the every tree source node.
       * for example having 3 independent formulas will invoke this 3 times. this is discussed here:
       * http://www.boost.org/doc/libs/1_54_0/libs/graph/doc/graph_theory_review.html#sec:dfs-algorithm
       */
      startVertex = vertex;
    }
    template<typename FindVertex, typename FindGraph>
    void discover_vertex(FindVertex vertex, FindGraph &graph)
    {
      if (completedTest || startVertex == vertex)
        return;
      if (graph[vertex]->getType() == NodeType::Formula)
        subFormulaVertices.push(vertex);
      if (!subFormulaVertices.empty())
        return;
      verticesOut.push_back(vertex);
    }
    
    template<typename FindEdge, typename FindGraph>
    void examine_edge(FindEdge edge, FindGraph &graph)
    {
      if (completedTest)
        return;
      if (!subFormulaVertices.empty())
        return;
      edgesOut.push_back(edge);
    }
    
    template<typename FindVertex, typename FindGraph>
    void finish_vertex(FindVertex vertex, FindGraph &graph)
    {
      if (completedTest)
        return;
      if (vertex == startVertex)
      {
        completedTest = true;
        return;
      }
      if (graph[vertex]->getType() == NodeType::Formula)
        subFormulaVertices.pop();
    }
    
private:
  Vertex startVertex;
  bool completedTest;
  std::stack<Vertex> subFormulaVertices;
  std::vector<Vertex> &verticesOut;
  std::vector<Edge> &edgesOut;
};


/*! @brief Depth first search for cycle detection
 * 
 * Scans a graph for cycles in a newly edited formula. We assume that the graph is non-cyclic before submitting changes.
 * Meaning that only one newly edited formula is responsible for the cycle. This is member CycleVisitor::baseVertex.
 * CycleVisitor::branchVertex is the next formula after base vertex in the cycle, thus the cause of the cycle.
 * That is what we are looking for.
 * reference: <a href="http://www.boost.org/doc/libs/1_54_0/libs/graph/doc/file_dependency_example.html#sec:cycles">boost cycle example</a>
 */
class CycleVisitor : public boost::default_dfs_visitor
{
public:
  CycleVisitor(bool &hasCycleIn, Vertex &baseVertexIn, Vertex &branchVertexIn) : hasCycle(hasCycleIn),
    baseVertex(baseVertexIn), branchVertex(branchVertexIn)
  {
    hasCycle = false;
  }
  
  template<typename Vertex, typename Graph>
  void start_vertex(Vertex vertex, Graph &graph)
  {
    if (hasCycle)
      return;
    fStack = std::stack<Vertex>();
  }
  
  template <class Edge, class Graph>
  void back_edge(Edge edge, Graph& graph)
  {
    if (hasCycle)
      return;
    hasCycle = true;
    assert(!fStack.empty());
    Vertex current = fStack.top();
    fStack.pop();
    if (fStack.empty())
    {
      //formula references itself.
      branchVertex = current;
      return;
    }

    bool found = false;
    while(!fStack.empty())
    {
      if (fStack.top() == baseVertex)
      {
        found = true;
        break;
      }
      current = fStack.top();
      fStack.pop();
    }
    assert(found);
    branchVertex = current;
  }
  
  template <class Vertex, class Graph>
  void discover_vertex(Vertex vertex, Graph& graph)
  {
    if (hasCycle)
      return;
    if(graph[vertex]->getType() != NodeType::Formula)
      return;
    fStack.push(vertex);
  }
  
  template<typename Vertex, typename Graph>
  void finish_vertex(Vertex vertex, Graph &graph)
  {
    if (hasCycle)
      return;
    if(graph[vertex]->getType() != NodeType::Formula)
      return;
    if (!fStack.empty())
      fStack.pop();
  }
    
protected:
  bool &hasCycle;
  Vertex &baseVertex;
  Vertex &branchVertex;
  std::stack <Vertex> fStack;
};

/*! @brief Sets dependent nodes dirty. 
 * A breadth first visitor starting at vertex and setting nodes dirty.
 * Used for graph computation.
 */
class DependentDirtyVisitor : public boost::default_bfs_visitor
{
public:
  DependentDirtyVisitor(){}
  
  template<typename FindVertex, typename FindGraph>
  void discover_vertex(FindVertex vertex, FindGraph &graph)
  {
    graph[vertex]->setDirty();
  }
};

/*! @brief Get dependent formulas.
 * A breadth first visitor starting at vertex. This is used
 * for exporting in correct order.
 */
class DependentFormulaCollectionVisitor : public boost::default_bfs_visitor
{
public:
  DependentFormulaCollectionVisitor(std::vector<Vertex> &verticesIn) : verticesOut(verticesIn){}
  
  template<typename FindVertex, typename FindGraph>
  void discover_vertex(FindVertex vertex, FindGraph &graph)
  {
    if (graph[vertex]->getType() == NodeType::Formula)
      verticesOut.push_back(vertex);
  }
private:
  std::vector<Vertex> &verticesOut;
};

}
#endif // EXPRESSIONGRAPH_H

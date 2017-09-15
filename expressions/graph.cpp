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

#include <assert.h>

#include <boost/graph/reverse_graph.hpp>
#include <boost/graph/copy.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/graph/graphviz.hpp>

#include <expressions/graph.h>

using namespace expr;
using boost::uuids::uuid;

GraphWrapper::GraphWrapper()
{

}

GraphWrapper::~GraphWrapper()
{

}

void GraphWrapper::indexVerticesAndEdges()
{
  //index vertices.
  VertexIterator it, itEnd;
  std::size_t index = 0;
  for(boost::tie(it, itEnd) = boost::vertices(graph); it != itEnd; ++it)
  {
    boost::put(boost::vertex_index, graph, *it, index);
    index++;
  }
  
  //index edges.
  EdgeIterator eit, eitEnd;
  index = 0;
  for(boost::tie(eit, eitEnd) = boost::edges(graph); eit != eitEnd; ++eit)
  {
    boost::put(boost::edge_index, graph, *eit, index);
    index++;
  }
}

void GraphWrapper::writeOutGraph(const std::string& pathName)
{
  indexVerticesAndEdges();
  std::ofstream file(pathName.c_str());
  boost::write_graphviz(file, graph, Vertex_writer<Graph>(graph),
                        Edge_writer<Graph>(graph), boost::default_writer());
}

void GraphWrapper::update()
{
  indexVerticesAndEdges();
  
  std::vector<Vertex> vertices;
  try
  {
    boost::topological_sort(graph, std::back_inserter(vertices));
  }
  catch(const boost::not_a_dag &)
  {
    std::cout << "not a dag exception in recompute" << std::endl;
    return;
  }
  
  for (std::vector<Vertex>::iterator it = vertices.begin(); it != vertices.end(); ++it)
  {
    AbstractNode *currentNode = graph[*it].get();
    assert(currentNode);
    if (currentNode->isClean())
      continue;
    currentNode->calculate(buildEdgePropertiesMap(*it));
  }
}

EdgePropertiesMap GraphWrapper::buildEdgePropertiesMap(const Vertex& source) const
{
  EdgePropertiesMap out;
  OutEdgeIterator it, itEnd;
  boost::tie(it, itEnd) = boost::out_edges(source, graph);
  for (;it != itEnd; ++it)
  {
    Vertex target = boost::target(*it, graph);
    out.insert(std::make_pair(graph[*it], graph[target].get()));
  }
  return out;
}

bool GraphWrapper::hasFormula(const std::string& nameIn) const
{
  VertexIterator it, itEnd;
  boost::tie(it, itEnd) = boost::vertices(graph);
  for (;it != itEnd; ++it)
  {
    if (graph[*it]->getType() != NodeType::Formula)
      continue;
    FormulaNode *node = dynamic_cast<FormulaNode *>(graph[*it].get());
    assert(node);
    if (node->name == nameIn)
      return true;
  }
  
  return false;
}

Vertex GraphWrapper::getFormulaVertex(const std::string& nameIn) const
{
  VertexIterator it, itEnd;
  boost::tie(it, itEnd) = boost::vertices(graph);
  for (;it != itEnd; ++it)
  {
    if (graph[*it]->getType() != NodeType::Formula)
      continue;
    FormulaNode *node = dynamic_cast<FormulaNode *>(graph[*it].get());
    assert(node);
    if (node->name == nameIn)
      return *it;
  }
  
  assert(0);
  throw std::runtime_error("couldn't find graph vertex in GraphWrapper::getFormulaVertex");
}

void GraphWrapper::setFormulaName(const boost::uuids::uuid &idIn, const std::string& nameIn)
{
  assert(hasFormula(idIn));
  Vertex fVertex = getFormulaVertex(idIn);
  FormulaNode *fNode = dynamic_cast<FormulaNode *>(graph[fVertex].get());
  assert(fNode);
  fNode->name = nameIn;
}

void GraphWrapper::setFormulaId(const boost::uuids::uuid &oldIdIn, const boost::uuids::uuid &newIdIn)
{
  assert(hasFormula(oldIdIn));
  Vertex fVertex = getFormulaVertex(oldIdIn);
  FormulaNode *fNode = dynamic_cast<FormulaNode *>(graph[fVertex].get());
  assert(fNode);
  fNode->setId(newIdIn);
}

std::vector< boost::uuids::uuid > GraphWrapper::getAllFormulaIds() const
{
  std::vector<boost::uuids::uuid> out;
  VertexIterator it, itEnd;
  boost::tie(it, itEnd) = boost::vertices(graph);
  for (;it != itEnd; ++it)
  {
    if (graph[*it]->getType() != NodeType::Formula)
      continue;
    FormulaNode *fNode = static_cast<FormulaNode*>(graph[*it].get());
    out.push_back(fNode->getId());
  }
  return out;
}

std::vector< boost::uuids::uuid > GraphWrapper::getAllFormulaIdsSorted()
{
  indexVerticesAndEdges();
  
  std::vector<Vertex> vertices;
  try
  {
    boost::topological_sort(graph, std::back_inserter(vertices));
  }
  catch(const boost::not_a_dag &)
  {
    assert(0); //not a dag exception in recompute
  }
  
  std::vector<boost::uuids::uuid> out;
  for (std::vector<Vertex>::iterator it = vertices.begin(); it != vertices.end(); ++it)
  {
    if(graph[*it]->getType() == NodeType::Formula)
    {
      FormulaNode *fNode = static_cast<FormulaNode*>(graph[*it].get());
      out.push_back(fNode->getId());
    }
  }
  
  return out;
}

std::vector<std::string> GraphWrapper::getAllFormulaNames() const
{
  std::vector<std::string> out;
  VertexIterator it, itEnd;
  boost::tie(it, itEnd) = boost::vertices(graph);
  for (; it != itEnd; ++it)
  {
    if (graph[*it]->getType() != NodeType::Formula)
      continue;
    out.push_back(static_cast<FormulaNode*>(graph[*it].get())->name);
  }
  return out;
}

bool GraphWrapper::hasFormula(const boost::uuids::uuid& idIn) const
{
  VertexIterator it, itEnd;
  boost::tie(it, itEnd) = boost::vertices(graph);
  for (;it != itEnd; ++it)
  {
    if (graph[*it]->getType() != NodeType::Formula)
      continue;
    FormulaNode *fNode = static_cast<FormulaNode*>(graph[*it].get());
    if (fNode->getId() == idIn)
      return true;
  }
  return false;
}

Vertex GraphWrapper::getFormulaVertex(const boost::uuids::uuid& idIn) const
{
  VertexIterator it, itEnd;
  boost::tie(it, itEnd) = boost::vertices(graph);
  for (;it != itEnd; ++it)
  {
    if (graph[*it]->getType() != NodeType::Formula)
      continue;
    FormulaNode *fNode = static_cast<FormulaNode*>(graph[*it].get());
    if (fNode->getId() == idIn)
      return *it;
  }
  assert(0); //has no formual of id.
  throw std::runtime_error("couldn't find formula with given id in GraphWrapper::getFormulaVertex");
}

boost::uuids::uuid GraphWrapper::getFormulaId(const Vertex &vertexIn) const
{
  assert(graph[vertexIn]->getType() == NodeType::Formula);
  FormulaNode *fNode = static_cast<FormulaNode*>(graph[vertexIn].get());
  return fNode->getId();
}

boost::uuids::uuid GraphWrapper::getFormulaId(const std::string& nameIn) const
{
  assert(hasFormula(nameIn));
  return getFormulaId(getFormulaVertex(nameIn));
}

std::string GraphWrapper::getFormulaName(const Vertex &vIn) const
{
  assert(graph[vIn]->getType() == NodeType::Formula);
  FormulaNode *fNode = dynamic_cast<FormulaNode *>(graph[vIn].get());
  assert(fNode);
  return fNode->name;
}

std::string GraphWrapper::getFormulaName(const boost::uuids::uuid& idIn) const
{
  assert(hasFormula(idIn));
  Vertex fVertex = getFormulaVertex(idIn);
  return getFormulaName(fVertex);
}

Value GraphWrapper::getFormulaValue(const Vertex& vIn) const
{
  assert(graph[vIn]->getType() == NodeType::Formula);
  FormulaNode *fNode = dynamic_cast<FormulaNode *>(graph[vIn].get());
  assert(fNode);
  return fNode->getValue();
}

Value GraphWrapper::getFormulaValue(const boost::uuids::uuid& idIn) const
{
  assert(hasFormula(idIn));
  return getFormulaValue(getFormulaVertex(idIn));
}

ValueType GraphWrapper::getFormulaValueType(const Vertex &vIn) const
{
  assert(graph[vIn]->getType() == NodeType::Formula);
  return graph[vIn]->getOutputType();
}

ValueType GraphWrapper::getFormulaValueType(const uuid &idIn) const
{
  assert(hasFormula(idIn));
  return getFormulaValueType(getFormulaVertex(idIn));
}

Vertex GraphWrapper::buildScalarConstantNode(const double& constantIn)
{
  Vertex cVertex = buildNode<ScalarConstantNode>();
  static_cast<ScalarConstantNode*>(graph[cVertex].get())->setValue(constantIn);
  return cVertex;
}

Vertex GraphWrapper::buildFormulaNode(const std::string& stringIn)
{
  Vertex vertex = buildNode<FormulaNode>();
  static_cast<FormulaNode*>(graph[vertex].get())->name = stringIn;
  return vertex;
}

Edge GraphWrapper::buildEdge(const Vertex& sourceIn, const Vertex& targetIn)
{
  bool results;
  Edge edge;
  boost::tie(edge, results) = boost::add_edge(sourceIn, targetIn, graph);
  assert(results);
  return edge;
}

void GraphWrapper::cleanFormula(const boost::uuids::uuid& idIn)
{
  assert(this->hasFormula(idIn));
  Vertex fVertex = getFormulaVertex(idIn);
  cleanFormula(fVertex);
}

void GraphWrapper::cleanFormula(const Vertex& vertexIn)
{
  indexVerticesAndEdges();
  
  std::vector<Vertex> vertices;
  std::vector<Edge> edges;
  FormulaCleanVisitor visitor(vertices, edges);
  boost::depth_first_search(graph, boost::visitor(visitor).root_vertex(vertexIn));
  
  for (std::vector<Edge>::iterator it = edges.begin(); it != edges.end(); ++it)
    boost::remove_edge(*it, graph);
  for (std::vector<Vertex>::iterator it = vertices.begin(); it != vertices.end(); ++it)
    boost::remove_vertex(*it, graph);
}

void GraphWrapper::removeFormula(const boost::uuids::uuid& idIn)
{
  assert(hasFormula(idIn));
  Vertex fVertex = getFormulaVertex(idIn);
  removeFormula(fVertex);
}

void GraphWrapper::removeFormula(const Vertex& vertexIn)
{
  ValueType valueType = static_cast<FormulaNode *>(graph[vertexIn].get())->getOutputType();
  Value oldValue = static_cast<FormulaNode *>(graph[vertexIn].get())->getValue();
  cleanFormula(vertexIn);
  
  //any dependents will get a constant value equal to the original formula.
  InEdgeIterator it, itEnd;
  boost::tie(it, itEnd) = boost::in_edges(vertexIn, graph);
  for (;it != itEnd; ++it)
  {
    Vertex constant = graph.null_vertex();
    if (valueType == ValueType::Scalar)
    {
      constant = buildScalarConstantNode(boost::get<double>(oldValue));
      graph[constant]->setClean(); //should equal so no need to be the default dirty.
    }
    // address other types and a check for null.
    Vertex dependent = boost::source(*it, graph);
    Edge edge = buildEdge(dependent, constant);
    graph[edge] = graph[*it];
    boost::remove_edge(*it, graph);
  }
  
  boost::remove_vertex(vertexIn, graph);
}

bool GraphWrapper::hasCycle(const boost::uuids::uuid &idIn, std::string &nameOut)
{
  indexVerticesAndEdges();
  
  bool out = false;
  assert(hasFormula(idIn));
  Vertex baseVertex = getFormulaVertex(idIn);
  Vertex branchVertex = Graph::null_vertex();
  CycleVisitor visitor(out, baseVertex, branchVertex);
  boost::depth_first_search(graph, boost::visitor(visitor).root_vertex(baseVertex));
  if (out)
  {
    assert(branchVertex != Graph::null_vertex());
    nameOut = getFormulaName(branchVertex);
  }
  return out;
}

void GraphWrapper::setFormulaDependentsDirty(const Vertex& vertexIn)
{
  assert(graph[vertexIn]->getType() == NodeType::Formula);
  
  indexVerticesAndEdges();
  
  DependentDirtyVisitor visitor;
  boost::breadth_first_search(boost::make_reverse_graph(graph), vertexIn, boost::visitor(visitor));
}

void GraphWrapper::setFormulaDependentsDirty(const boost::uuids::uuid& idIn)
{
  assert(hasFormula(idIn));
  Vertex vertex = getFormulaVertex(idIn);
  setFormulaDependentsDirty(vertex);
}

void GraphWrapper::setAllDirty()
{
  VertexIterator vIt, vItEnd;
  for (boost::tie(vIt, vItEnd) = boost::vertices(graph); vIt != vItEnd; ++vIt)
  {
    graph[*vIt]->setDirty();
  }
}

std::vector< boost::uuids::uuid > GraphWrapper::getDependentFormulaIds(const boost::uuids::uuid& parentIn)
{
  assert(hasFormula(parentIn));
  Vertex root = getFormulaVertex(parentIn);
  
  indexVerticesAndEdges();
  
  std::vector<Vertex> depedentVertices;
  DependentFormulaCollectionVisitor visitor(depedentVertices);
  boost::breadth_first_search(graph, root, boost::visitor(visitor));
  
  std::vector<boost::uuids::uuid> out;
  std::vector<Vertex>::const_iterator it;
  for (it = depedentVertices.begin(); it != depedentVertices.end(); ++it)
    out.push_back(getFormulaId(*it));
  
  return out;
}

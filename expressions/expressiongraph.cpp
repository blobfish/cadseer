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

#include <expressions/expressiongraph.h>

using namespace expr;

GraphWrapper::GraphWrapper()
{

}

GraphWrapper::GraphWrapper(const GraphWrapper &graphWrapperIn)
{
  //not using member function here because we are indexing the graph passed in, not the member graph.
  VertexIndexMap vIndexMap;
  boost::associative_property_map<VertexIndexMap> pMap(vIndexMap);
  VertexIterator vIt, vItEnd;
  int index = 0;
  for (boost::tie(vIt, vItEnd) = boost::vertices(graphWrapperIn.graph); vIt != vItEnd; ++vIt)
    boost::put(pMap, *vIt, index++);
  
  boost::copy_graph(graphWrapperIn.graph, graph, boost::vertex_index_map(pMap));
  
  //probably could use vertex copy function.
  for (boost::tie(vIt, vItEnd) = boost::vertices(graph); vIt != vItEnd; ++vIt)
    graph[*vIt] = boost::shared_ptr<AbstractNode>(graph[*vIt]->clone());
}

GraphWrapper::~GraphWrapper()
{

}

void GraphWrapper::writeOutGraph(const std::string& pathName)
{
  VertexIndexMap vIndexMap;
  boost::associative_property_map<VertexIndexMap> pMap = buildVertexPropertyMap(vIndexMap);
  std::ofstream file(pathName.c_str());
  boost::write_graphviz(file, graph, Vertex_writer<Graph>(graph),
                        Edge_writer<Graph>(graph), boost::default_writer(), pMap);
}

void GraphWrapper::update()
{
  VertexIndexMap vIndexMap;
  boost::associative_property_map<VertexIndexMap> pMap = buildVertexPropertyMap(vIndexMap);
  
  std::vector<Vertex> vertices;
  try
  {
    boost::topological_sort(graph, std::back_inserter(vertices), boost::vertex_index_map(pMap));
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
    out.insert(std::make_pair(graph[*it], graph[target]->getValue()));
  }
  return out;
}

VertexPropertyMap GraphWrapper::buildVertexPropertyMap(VertexIndexMap &mapIn) const
{
  boost::associative_property_map<VertexIndexMap> pMap(mapIn);
  VertexIterator vIt, vItEnd;
  int index = 0;
  for (boost::tie(vIt, vItEnd) = boost::vertices(graph); vIt != vItEnd; ++vIt)
    boost::put(pMap, *vIt, index++);
  return pMap;
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
}

void GraphWrapper::setFormulaName(const boost::uuids::uuid &idIn, const std::string& nameIn)
{
  assert(hasFormula(idIn));
  Vertex fVertex = getFormulaVertex(idIn);
  FormulaNode *fNode = dynamic_cast<FormulaNode *>(graph[fVertex].get());
  assert(fNode);
  fNode->name = nameIn;
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
    out.push_back(graph[*it]->getId());
  }
  return out;
}

std::vector< boost::uuids::uuid > GraphWrapper::getAllFormulaIdsSorted() const
{
  VertexIndexMap vIndexMap;
  boost::associative_property_map<VertexIndexMap> pMap = buildVertexPropertyMap(vIndexMap);
  
  std::vector<Vertex> vertices;
  try
  {
    boost::topological_sort(graph, std::back_inserter(vertices), boost::vertex_index_map(pMap));
  }
  catch(const boost::not_a_dag &)
  {
    assert(0); //not a dag exception in recompute
  }
  
  std::vector<boost::uuids::uuid> out;
  for (std::vector<Vertex>::iterator it = vertices.begin(); it != vertices.end(); ++it)
  {
    if(graph[*it]->getType() == NodeType::Formula)
      out.push_back(graph[*it]->getId());
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
    if (graph[*it]->getId() == idIn)
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
    if (graph[*it]->getId() == idIn)
      return *it;
  }
  assert(0); //has no formual of id.
}

boost::uuids::uuid GraphWrapper::getId(const Vertex &vertexIn) const
{
  return graph[vertexIn]->getId();
}

boost::uuids::uuid GraphWrapper::getFormulaId(const std::string& nameIn) const
{
  assert(hasFormula(nameIn));
  return getId(getFormulaVertex(nameIn));
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

double GraphWrapper::getFormulaValue(const Vertex& vIn) const
{
  assert(graph[vIn]->getType() == NodeType::Formula);
  FormulaNode *fNode = dynamic_cast<FormulaNode *>(graph[vIn].get());
  assert(fNode);
  return fNode->getValue();
}

double GraphWrapper::getFormulaValue(const boost::uuids::uuid& idIn) const
{
  assert(hasFormula(idIn));
  return getFormulaValue(getFormulaVertex(idIn));
}

Vertex GraphWrapper::buildConstantNode(const double& constantIn)
{
  ConstantNode *cNode = new ConstantNode();
  cNode->setValue(constantIn);
  Vertex cVertex = boost::add_vertex(graph);
  graph[cVertex] = boost::shared_ptr<ConstantNode>(cNode);
  return cVertex;
}

Vertex GraphWrapper::buildFormulaNode(const std::string& stringIn)
{
  Vertex vertex = boost::add_vertex(graph);
  FormulaNode *fNode = new FormulaNode();
  fNode->name = stringIn;
  graph[vertex] = boost::shared_ptr<FormulaNode>(fNode);
  return vertex;
}

Vertex GraphWrapper::buildAdditionNode()
{
  Vertex vertex = boost::add_vertex(graph);
  AdditionNode *aNode = new AdditionNode();
  graph[vertex] = boost::shared_ptr<AdditionNode>(aNode);
  return vertex;

}

Vertex GraphWrapper::buildSubractionNode()
{
  Vertex vertex = boost::add_vertex(graph);
  SubtractionNode *sNode = new SubtractionNode();
  graph[vertex] = boost::shared_ptr<SubtractionNode>(sNode);
  return vertex;
}

Vertex GraphWrapper::buildMultiplicationNode()
{
  Vertex vertex = boost::add_vertex(graph);
  MultiplicationNode *mNode = new MultiplicationNode();
  graph[vertex] = boost::shared_ptr<MultiplicationNode>(mNode);
  return vertex;
}

Vertex GraphWrapper::buildDivisionNode()
{
  Vertex vertex = boost::add_vertex(graph);
  DivisionNode *dNode = new DivisionNode();
  graph[vertex] = boost::shared_ptr<DivisionNode>(dNode);
  return vertex;
}

Vertex GraphWrapper::buildParenthesesNode()
{
    Vertex vertex = boost::add_vertex(graph);
    ParenthesesNode *pNode = new ParenthesesNode();
    graph[vertex] = boost::shared_ptr<ParenthesesNode>(pNode);
    return vertex;
}

Vertex GraphWrapper::buildSinNode()
{
    Vertex vertex = boost::add_vertex(graph);
    SinNode *sNode = new SinNode();
    graph[vertex] = boost::shared_ptr<SinNode>(sNode);
    return vertex;
}

Vertex GraphWrapper::buildCosNode()
{
    Vertex vertex = boost::add_vertex(graph);
    CosNode *cNode = new CosNode();
    graph[vertex] = boost::shared_ptr<CosNode>(cNode);
    return vertex;
}

Vertex GraphWrapper::buildTanNode()
{
    Vertex vertex = boost::add_vertex(graph);
    TanNode *sNode = new TanNode();
    graph[vertex] = boost::shared_ptr<TanNode>(sNode);
    return vertex;
}

Vertex GraphWrapper::buildAsinNode()
{
    Vertex vertex = boost::add_vertex(graph);
    AsinNode *node = new AsinNode();
    graph[vertex] = boost::shared_ptr<AsinNode>(node);
    return vertex;
}

Vertex GraphWrapper::buildAcosNode()
{
    Vertex vertex = boost::add_vertex(graph);
    AcosNode *node = new AcosNode();
    graph[vertex] = boost::shared_ptr<AcosNode>(node);
    return vertex;
}

Vertex GraphWrapper::buildAtanNode()
{
    Vertex vertex = boost::add_vertex(graph);
    AtanNode *node = new AtanNode();
    graph[vertex] = boost::shared_ptr<AtanNode>(node);
    return vertex;
}

Vertex GraphWrapper::buildAtan2Node()
{
    Vertex vertex = boost::add_vertex(graph);
    Atan2Node *node = new Atan2Node();
    graph[vertex] = boost::shared_ptr<Atan2Node>(node);
    return vertex;
}

Vertex GraphWrapper::buildPowNode()
{
    Vertex vertex = boost::add_vertex(graph);
    PowNode *node = new PowNode();
    graph[vertex] = boost::shared_ptr<PowNode>(node);
    return vertex;
}

Vertex GraphWrapper::buildAbsNode()
{
    Vertex vertex = boost::add_vertex(graph);
    AbsNode *node = new AbsNode();
    graph[vertex] = boost::shared_ptr<AbsNode>(node);
    return vertex;
}

Vertex GraphWrapper::buildMinNode()
{
  Vertex vertex = boost::add_vertex(graph);
  MinNode *node = new MinNode();
  graph[vertex] = boost::shared_ptr<MinNode>(node);
  return vertex;
}

Vertex GraphWrapper::buildMaxNode()
{
  Vertex vertex = boost::add_vertex(graph);
  MaxNode *node = new MaxNode();
  graph[vertex] = boost::shared_ptr<MaxNode>(node);
  return vertex;
}

Vertex GraphWrapper::buildFloorNode()
{
  Vertex vertex = boost::add_vertex(graph);
  FloorNode *node = new FloorNode();
  graph[vertex] = boost::shared_ptr<FloorNode>(node);
  return vertex;
}

Vertex GraphWrapper::buildCeilNode()
{
  Vertex vertex = boost::add_vertex(graph);
  CeilNode *node = new CeilNode();
  graph[vertex] = boost::shared_ptr<CeilNode>(node);
  return vertex;
}

Vertex GraphWrapper::buildRoundNode()
{
  Vertex vertex = boost::add_vertex(graph);
  RoundNode *node = new RoundNode();
  graph[vertex] = boost::shared_ptr<RoundNode>(node);
  return vertex;
}

Vertex GraphWrapper::buildRadToDegNode()
{
  Vertex vertex = boost::add_vertex(graph);
  RadToDegNode *node = new RadToDegNode();
  graph[vertex] = boost::shared_ptr<RadToDegNode>(node);
  return vertex;
}

Vertex GraphWrapper::buildDegToRadNode()
{
  Vertex vertex = boost::add_vertex(graph);
  DegToRadNode *node = new DegToRadNode();
  graph[vertex] = boost::shared_ptr<DegToRadNode>(node);
  return vertex;
}

Vertex GraphWrapper::buildLogNode()
{
  Vertex vertex = boost::add_vertex(graph);
  LogNode *node = new LogNode();
  graph[vertex] = boost::shared_ptr<LogNode>(node);
  return vertex;
}

Vertex GraphWrapper::buildExpNode()
{
  Vertex vertex = boost::add_vertex(graph);
  ExpNode *node = new ExpNode();
  graph[vertex] = boost::shared_ptr<ExpNode>(node);
  return vertex;
}

Vertex GraphWrapper::buildSqrtNode()
{
  Vertex vertex = boost::add_vertex(graph);
  SqrtNode *node = new SqrtNode();
  graph[vertex] = boost::shared_ptr<SqrtNode>(node);
  return vertex;
}

Vertex GraphWrapper::buildHypotNode()
{
  Vertex vertex = boost::add_vertex(graph);
  HypotNode *node = new HypotNode();
  graph[vertex] = boost::shared_ptr<HypotNode>(node);
  return vertex;
}

Vertex GraphWrapper::buildConditionalNode()
{
  Vertex vertex = boost::add_vertex(graph);
  ConditionalNode *node = new ConditionalNode();
  graph[vertex] = boost::shared_ptr<ConditionalNode>(node);
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
  VertexIndexMap vIndexMap;
  boost::associative_property_map<VertexIndexMap> pMap = buildVertexPropertyMap(vIndexMap);
  
  std::vector<Vertex> vertices;
  std::vector<Edge> edges;
  FormulaCleanVisitor visitor(vertices, edges);
  boost::depth_first_search(graph, boost::visitor(visitor).root_vertex(vertexIn).vertex_index_map(pMap));
  
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
  double oldValue = static_cast<FormulaNode *>(graph[vertexIn].get())->getValue();
  cleanFormula(vertexIn);
  
  //any dependents will get a constant value equal to the original formula.
  InEdgeIterator it, itEnd;
  boost::tie(it, itEnd) = boost::in_edges(vertexIn, graph);
  for (;it != itEnd; ++it)
  {
    Vertex constant = buildConstantNode(oldValue);
    graph[constant]->setClean(); //should equal so no need to be the default dirty.
    Vertex dependent = boost::source(*it, graph);
    Edge edge = buildEdge(dependent, constant);
    graph[edge] = graph[*it];
    boost::remove_edge(*it, graph);
  }
  
  boost::remove_vertex(vertexIn, graph);
}

bool GraphWrapper::hasCycle(const boost::uuids::uuid &idIn, std::string &nameOut)
{
  VertexIndexMap vIndexMap;
  boost::associative_property_map<VertexIndexMap> pMap = buildVertexPropertyMap(vIndexMap);
  
  bool out = false;
  assert(hasFormula(idIn));
  Vertex baseVertex = getFormulaVertex(idIn);
  Vertex branchVertex = Graph::null_vertex();
  CycleVisitor visitor(out, baseVertex, branchVertex);
  boost::depth_first_search(graph, boost::visitor(visitor).root_vertex(baseVertex).vertex_index_map(pMap));
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
  
  VertexIndexMap vIndexMap;
  boost::associative_property_map<VertexIndexMap> pMap = buildVertexPropertyMap(vIndexMap);
  
  DependentDirtyVisitor visitor;
  boost::breadth_first_search(boost::make_reverse_graph(graph), vertexIn, boost::visitor(visitor).vertex_index_map(pMap));
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
  
  VertexIndexMap vIndexMap;
  boost::associative_property_map<VertexIndexMap> pMap = buildVertexPropertyMap(vIndexMap);
  
  std::vector<Vertex> depedentVertices;
  DependentFormulaCollectionVisitor visitor(depedentVertices);
  boost::breadth_first_search(graph, root, boost::visitor(visitor).vertex_index_map(pMap));
  
  std::vector<boost::uuids::uuid> out;
  std::vector<Vertex>::const_iterator it;
  for (it = depedentVertices.begin(); it != depedentVertices.end(); ++it)
    out.push_back(getId(*it));
  
  return out;
}

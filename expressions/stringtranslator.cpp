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

#include <QString>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <iostream>
#include <string>

#include <expressions/expressiongraph.h>
#include <expressions/stringtranslator.h>

using namespace expr;

StringTranslator::StringTranslator(ExpressionManager &eManagerIn) : failedPosition(-1), eManager(eManagerIn),
  graphWrapper(eManager.getGraphWrapper()), totalState(None)
{
}

StringTranslator::TotalState StringTranslator::parseString(const std::string &formula)
{
  totalState = None;
  vStack = std::stack<Vertex>();
  addedVertices.clear();
  addedEdges.clear();
  failedPosition = -1;
  formulaNodeOut = Graph::null_vertex();
  
  using boost::spirit::qi::phrase_parse;
  using boost::spirit::ascii::space;
  using boost::spirit::utree;
  
  typedef std::string::iterator strIt;
  
  std::string form = formula;
  strIt first = form.begin();
  strIt last = form.end();
  
  ExpressionGrammar <strIt> grammar(*this);
  
  bool r = phrase_parse(
      first,
      last,
      grammar,
      space
  );
  if (!r || first != last)
  {
    failedPosition = std::distance(form.begin(), first);
//     std::cout << "parse failed at: " <<  failedPosition << std::endl;
    cleanFailedParse();
    totalState = ParseFailed;
    return ParseFailed;
  }
//   std::cout << "parse succeeded" << std::endl;
  
  totalState = ParseSucceeded;
  return totalState;
}

Vertex StringTranslator::getFormulaNodeOut()
{
  assert(formulaNodeOut != Graph::null_vertex());
  return formulaNodeOut;
}

void StringTranslator::cleanFailedParse()
{
  for (std::vector<Edge>::iterator it = addedEdges.begin(); it != addedEdges.end(); ++it)
    boost::remove_edge(*it, graphWrapper.graph);
  
  for (std::vector<Vertex>::iterator it = addedVertices.begin(); it != addedVertices.end(); ++it)
  {
    if (graphWrapper.graph[*it]->getType() == NodeType::Formula)
      eManager.removeFormulaFromAllGroup(graphWrapper.getId(*it));
    boost::remove_vertex(*it, graphWrapper.graph);
  }
}

void StringTranslator::buildConstantNode(const double& valueIn)
{
  Vertex cNode = graphWrapper.buildConstantNode(valueIn);
  vStack.push(cNode);
  addedVertices.push_back(cNode);
}

void StringTranslator::buildFormulaNode(const std::string& stringIn, bool &carryOn)
{
  Vertex fVertex = graphWrapper.graph.null_vertex();
  if (graphWrapper.hasFormula(stringIn))
  {
    fVertex = graphWrapper.getFormulaVertex(stringIn);
    if (boost::out_degree(fVertex, graphWrapper.graph) > 0)
    {
      //TODO error message.
      std::cout << "out degree in StringTranslator::buildFormulaNode is: " << boost::out_degree(fVertex, graphWrapper.graph) << std::endl;
      carryOn = false;
      return;
    }
    FormulaNode *fNode = dynamic_cast<FormulaNode *>(graphWrapper.graph[fVertex].get());
    assert(fNode);
    fNode->setDirty();
  }
  else
  {
    fVertex = graphWrapper.buildFormulaNode(stringIn);
    eManager.addFormulaToAllGroup(graphWrapper.getId(fVertex));
    addedVertices.push_back(fVertex);
  }
  
  assert(fVertex != graphWrapper.graph.null_vertex());
  formulaNodeOut = fVertex;
  vStack.push(fVertex);
}

void StringTranslator::buildAdditionNode()
{
//   assert(currentNode != graphWrapper.graph.null_vertex());
  
  Vertex aNode = graphWrapper.buildAdditionNode();
  vStack.push(aNode);
  addedVertices.push_back(aNode);
}

void StringTranslator::buildSubractionNode()
{
  Vertex sNode = graphWrapper.buildSubractionNode();
  vStack.push(sNode);
  addedVertices.push_back(sNode);
}

void StringTranslator::buildMultiplicationNode()
{
  Vertex mNode = graphWrapper.buildMultiplicationNode(); 
  vStack.push(mNode);
  addedVertices.push_back(mNode);
}

void StringTranslator::buildDivisionNode()
{
  Vertex dNode = graphWrapper.buildDivisionNode();
  vStack.push(dNode);
  addedVertices.push_back(dNode);
}

void StringTranslator::buildLinkNode(const std::string &stringIn, bool &carryOn)
{
  if(!graphWrapper.hasFormula(stringIn))
  {
    std::cout << "invalid link node name" << std::endl;
    carryOn = false;
    return;
  }
  Vertex lVertex = graphWrapper.getFormulaVertex(stringIn);
  vStack.push(lVertex);
}

void StringTranslator::startParenthesesNode()
{
  Vertex pNode = graphWrapper.buildParenthesesNode();
  vStack.push(pNode);
  addedVertices.push_back(pNode);
}

void StringTranslator::finishParenthesesNode()
{
  Vertex op = vStack.top();
  vStack.pop();
  Vertex parentheses = vStack.top();
  buildEdgeNone(parentheses, op);
}

void StringTranslator::buildSinNode()
{
  Vertex sNode = graphWrapper.buildSinNode();
  vStack.push(sNode);
  addedVertices.push_back(sNode);
}

void StringTranslator::buildCosNode()
{
  Vertex cNode = graphWrapper.buildCosNode();
  vStack.push(cNode);
  addedVertices.push_back(cNode);
}

void StringTranslator::buildTanNode()
{
  Vertex tNode = graphWrapper.buildTanNode();
  vStack.push(tNode);
  addedVertices.push_back(tNode);
}

void StringTranslator::buildAsinNode()
{
  Vertex node = graphWrapper.buildAsinNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslator::buildAcosNode()
{
  Vertex node = graphWrapper.buildAcosNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslator::buildAtanNode()
{
  Vertex node = graphWrapper.buildAtanNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslator::buildAtan2Node()
{
  Vertex node = graphWrapper.buildAtan2Node();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslator::buildPowNode()
{
  Vertex node = graphWrapper.buildPowNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslator::buildAbsNode()
{
  Vertex node = graphWrapper.buildAbsNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslator::buildMinNode()
{
  Vertex node = graphWrapper.buildMinNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslator::buildMaxNode()
{
  Vertex node = graphWrapper.buildMaxNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslator::buildFloorNode()
{
  Vertex node = graphWrapper.buildFloorNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslator::buildCeilNode()
{
  Vertex node = graphWrapper.buildCeilNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslator::buildRoundNode()
{
  Vertex node = graphWrapper.buildRoundNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslator::buildRadToDegNode()
{
  Vertex node = graphWrapper.buildRadToDegNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslator::buildDegToRadNode()
{
  Vertex node = graphWrapper.buildDegToRadNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslator::buildLogNode()
{
  Vertex node = graphWrapper.buildLogNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslator::buildExpNode()
{
  Vertex node = graphWrapper.buildExpNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslator::buildSqrtNode()
{
  Vertex node = graphWrapper.buildSqrtNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslator::buildHypotNode()
{
  Vertex node = graphWrapper.buildHypotNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslator::buildConditionalNode()
{
  Vertex node = graphWrapper.buildConditionalNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslator::setConditionGreaterThan()
{
  Vertex ifNode = vStack.top();
  ConditionalNode *node = dynamic_cast<ConditionalNode*>(graphWrapper.graph[ifNode].get());
  assert(node);
  node->type = ConditionalNode::GreaterThan;
}

void StringTranslator::setConditionLessThan()
{
  Vertex ifNode = vStack.top();
  ConditionalNode *node = dynamic_cast<ConditionalNode*>(graphWrapper.graph[ifNode].get());
  assert(node);
  node->type = ConditionalNode::LessThan;
}

void StringTranslator::setConditionGreaterThanEqual()
{
  Vertex ifNode = vStack.top();
  ConditionalNode *node = dynamic_cast<ConditionalNode*>(graphWrapper.graph[ifNode].get());
  assert(node);
  node->type = ConditionalNode::GreaterThanEqual;
}

void StringTranslator::setConditionLessThanEqual()
{
  Vertex ifNode = vStack.top();
  ConditionalNode *node = dynamic_cast<ConditionalNode*>(graphWrapper.graph[ifNode].get());
  assert(node);
  node->type = ConditionalNode::LessThanEqual;
}

void StringTranslator::setConditionEqual()
{
  Vertex ifNode = vStack.top();
  ConditionalNode *node = dynamic_cast<ConditionalNode*>(graphWrapper.graph[ifNode].get());
  assert(node);
  node->type = ConditionalNode::Equal;
}

void StringTranslator::setConditionNotEqual()
{
  Vertex ifNode = vStack.top();
  ConditionalNode *node = dynamic_cast<ConditionalNode*>(graphWrapper.graph[ifNode].get());
  assert(node);
  node->type = ConditionalNode::NotEqual;
}

void StringTranslator::setConditionLhs()
{
  Vertex lhs = vStack.top();
  vStack.pop();
  Vertex ifNode = vStack.top();
  Edge edge = buildEdgeCommon(ifNode, lhs);
  graphWrapper.graph[edge] = EdgeProperty::Lhs;
}

void StringTranslator::setConditionRhs()
{
  Vertex rhs = vStack.top();
  vStack.pop();
  Vertex ifNode = vStack.top();
  Edge edge = buildEdgeCommon(ifNode, rhs);
  graphWrapper.graph[edge] = EdgeProperty::Rhs;
}

void StringTranslator::setConditionThen()
{
  Vertex thenNode = vStack.top();
  vStack.pop();
  Vertex ifNode = vStack.top();
  Edge edge = buildEdgeCommon(ifNode, thenNode);
  graphWrapper.graph[edge] = EdgeProperty::Then;
}

void StringTranslator::setConditionElse()
{
  Vertex elseNode = vStack.top();
  vStack.pop();
  Vertex ifNode = vStack.top();
  Edge edge = buildEdgeCommon(ifNode, elseNode);
  graphWrapper.graph[edge] = EdgeProperty::Else;
}

void StringTranslator::setParameter1()
{
  Vertex p1Node = vStack.top();
  vStack.pop();
  Vertex functionNode = vStack.top();
  Edge edge = buildEdgeCommon(functionNode, p1Node);
  graphWrapper.graph[edge] = EdgeProperty::Parameter1;
}

void StringTranslator::setParameter2()
{
  Vertex p2Node = vStack.top();
  vStack.pop();
  Vertex functionNode = vStack.top();
  Edge edge = buildEdgeCommon(functionNode, p2Node);
  graphWrapper.graph[edge] = EdgeProperty::Parameter2;
}

void StringTranslator::finishFunction1()
{
  Vertex child = vStack.top();
  vStack.pop();
  Vertex trig = vStack.top();
  buildEdgeNone(trig, child);
}

void StringTranslator::buildEdgeLHS(const Vertex &source, const Vertex &target)
{
  Edge edgeLhs = buildEdgeCommon(source, target);
  graphWrapper.graph[edgeLhs] = EdgeProperty::Lhs;
}

void StringTranslator::buildEdgeRHS(const Vertex &source, const Vertex &target)
{
  Edge edgeRhs = buildEdgeCommon(source, target);
  graphWrapper.graph[edgeRhs] = EdgeProperty::Rhs;
}

void StringTranslator::buildEdgeNone(const Vertex& source, const Vertex& target)
{
  Edge edgeNone = buildEdgeCommon(source, target);
  graphWrapper.graph[edgeNone] = EdgeProperty::None;
}

Edge StringTranslator::buildEdgeCommon(const Vertex &source, const Vertex &target)
{
  assert(source != graphWrapper.graph.null_vertex());
  assert(target != graphWrapper.graph.null_vertex());
  
  Edge anEdge = graphWrapper.buildEdge(source, target);
  addedEdges.push_back(anEdge);
  
  return(anEdge);
}

void StringTranslator::makeCurrentRHS()
{
  assert (vStack.size() > 2);
  
  Vertex rhs = vStack.top();
  vStack.pop();
  Vertex op = vStack.top();
  vStack.pop();
  Vertex lhs = vStack.top();
  vStack.pop();
  
  buildEdgeLHS(op, lhs);
  buildEdgeRHS(op, rhs);
  
  vStack.push(op);
}

void StringTranslator::finish()
{
//   assert (vStack.size() == 2);
  //no assert. if stack isn't correct, parse should fail naturally.
  if (vStack.size() == 2)
  {
    Vertex rhs = vStack.top();
    vStack.pop();
    Vertex formula = vStack.top();
    vStack.pop();
    buildEdgeNone(formula, rhs);
  }
}

std::string StringTranslator::buildStringRhs(const Vertex &vertexIn) const
{
  std::list<std::string> expression = buildStringCommon(vertexIn);
  assert(!expression.empty());
  std::ostringstream stream;
  //first entry is name + '='. skip
  std::copy(++(expression.begin()), expression.end(), std::ostream_iterator<std::string>(stream));
  return stream.str();
}

std::string StringTranslator::buildStringAll(const Vertex& vertexIn) const
{
  std::list<std::string> expression = buildStringCommon(vertexIn);
  assert(!expression.empty());
  std::ostringstream stream;
  std::copy(expression.begin(), expression.end(), std::ostream_iterator<std::string>(stream));
  return stream.str();
}

std::list< std::string > StringTranslator::buildStringCommon(const Vertex& vertexIn) const
{
  VertexIndexMap vIndexMap;
  boost::associative_property_map<VertexIndexMap> pMap(vIndexMap);
  VertexIterator vIt, vItEnd;
  int index = 0;
  for (boost::tie(vIt, vItEnd) = boost::vertices(graphWrapper.graph); vIt != vItEnd; ++vIt)
    boost::put(pMap, *vIt, index++);
  
  std::list<std::string> expression;
  
  FormulaVisitor visitor(expression);
  boost::depth_first_search(graphWrapper.graph, boost::visitor(visitor).root_vertex(vertexIn).vertex_index_map(pMap));
  
  return expression;
}

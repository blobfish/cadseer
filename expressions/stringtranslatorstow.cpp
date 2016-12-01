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

#include <iostream>
#include <string>
#include <stack>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/filtered_graph.hpp>

#include <expressions/grammer.h>
#include <expressions/stringtranslatorstow.h>
#include <tools/graphtools.h>

using namespace expr;

/*! @brief Depth first search to build a representation string of a formula.
  * 
  * Traverses child vertices of a formulaNode. Will visit dependent formulaNodes,
  * but not their children. This is used to build a string representation of the formulaNode.
  * */
class FormulaVisitor : public boost::default_dfs_visitor
{
public:
    FormulaVisitor(std::list<std::string> &stringIn) : expression(stringIn), currentPosition(expression.begin())
    {
        typeMap.insert(std::make_pair(expr::NodeType::Addition, " + "));
        typeMap.insert(std::make_pair(expr::NodeType::Subtraction, " - "));
        typeMap.insert(std::make_pair(expr::NodeType::Multiplication, " * "));
        typeMap.insert(std::make_pair(expr::NodeType::Division, " / "));
    }
      
    template<typename FindVertex, typename FindGraph>
    void start_vertex(FindVertex vertex, FindGraph &)
    {
        startVertex = vertex;
    }
      
      //watchout for storing iterator == end();
      
    template<typename FindVertex, typename FindGraph>
    void discover_vertex(FindVertex vertex, FindGraph &graph)
    {
        if (graph[vertex]->getType() == expr::NodeType::Formula)
        {
            expr::FormulaNode *fNode = dynamic_cast<expr::FormulaNode *>(graph[vertex].get());
            if (startVertex == vertex)
            {
                //found root formula.
                currentPosition = expression.insert(currentPosition, fNode->name + " = ");
            }
            else
            {
                currentPosition = expression.insert(currentPosition, fNode->name);
            }
            currentPosition++;
            StackEntry temp;
            temp.push_back(currentPosition);
            itStack.push(temp);
        }
        else if (typeMap.count(graph[vertex]->getType()) > 0)
        {
            currentPosition = expression.insert(currentPosition, typeMap.at(graph[vertex]->getType()));
            StackEntry temp;
            temp.push_back(currentPosition);
            currentPosition++;
            temp.push_back(currentPosition);
            itStack.push(temp);
        }
        else if (graph[vertex]->getType() == expr::NodeType::Parentheses)
        {
            currentPosition = expression.insert(currentPosition, "(");
            currentPosition++;
            currentPosition = expression.insert(currentPosition, ")");
            StackEntry temp;
            temp.push_back(currentPosition);
            itStack.push(temp);
        }
        else if (graph[vertex]->getType() == expr::NodeType::Constant)
        {
            std::ostringstream stream;
            stream << graph[vertex]->getValue();
            currentPosition = expression.insert(currentPosition, stream.str());
            StackEntry temp;
            temp.push_back(currentPosition);
            itStack.push(temp);
        }
        else if (graph[vertex]->getType() == expr::NodeType::Sin)
        {
            currentPosition = expression.insert(currentPosition, "sin(");
            currentPosition++;
            currentPosition = expression.insert(currentPosition, ")");
            StackEntry temp;
            temp.push_back(currentPosition);
            itStack.push(temp);
        }
        else if (graph[vertex]->getType() == expr::NodeType::Cos)
        {
            currentPosition = expression.insert(currentPosition, "cos(");
            currentPosition++;
            currentPosition = expression.insert(currentPosition, ")");
            StackEntry temp;
            temp.push_back(currentPosition);
            itStack.push(temp);
        }
        else if (graph[vertex]->getType() == expr::NodeType::Tan)
        {
            currentPosition = expression.insert(currentPosition, "tan(");
            currentPosition++;
            currentPosition = expression.insert(currentPosition, ")");
            StackEntry temp;
            temp.push_back(currentPosition);
            itStack.push(temp);
        }
        else if (graph[vertex]->getType() == expr::NodeType::Asin)
        {
            currentPosition = expression.insert(currentPosition, "asin(");
            currentPosition++;
            currentPosition = expression.insert(currentPosition, ")");
            StackEntry temp;
            temp.push_back(currentPosition);
            itStack.push(temp);
        }
        else if (graph[vertex]->getType() == expr::NodeType::Acos)
        {
            currentPosition = expression.insert(currentPosition, "acos(");
            currentPosition++;
            currentPosition = expression.insert(currentPosition, ")");
            StackEntry temp;
            temp.push_back(currentPosition);
            itStack.push(temp);
        }
        else if (graph[vertex]->getType() == expr::NodeType::Atan)
        {
            currentPosition = expression.insert(currentPosition, "atan(");
            currentPosition++;
            currentPosition = expression.insert(currentPosition, ")");
            StackEntry temp;
            temp.push_back(currentPosition);
            itStack.push(temp);
        }
        else if (graph[vertex]->getType() == expr::NodeType::Pow)
        {
            currentPosition = expression.insert(currentPosition, "pow(");
            currentPosition++;
            currentPosition = expression.insert(currentPosition, ", ");
            StackEntry temp;
            temp.push_back(currentPosition);
            currentPosition++;
            currentPosition = expression.insert(currentPosition, ")");
            temp.push_back(currentPosition);
            itStack.push(temp);
        }
        else if (graph[vertex]->getType() == expr::NodeType::Atan2)
        {
            currentPosition = expression.insert(currentPosition, "atan2(");
            currentPosition++;
            currentPosition = expression.insert(currentPosition, ", ");
            StackEntry temp;
            temp.push_back(currentPosition);
            currentPosition++;
            currentPosition = expression.insert(currentPosition, ")");
            temp.push_back(currentPosition);
            itStack.push(temp);
        }
        else if (graph[vertex]->getType() == expr::NodeType::Abs)
        {
            currentPosition = expression.insert(currentPosition, "abs(");
            currentPosition++;
            currentPosition = expression.insert(currentPosition, ")");
            StackEntry temp;
            temp.push_back(currentPosition);
            itStack.push(temp);
        }
        else if (graph[vertex]->getType() == expr::NodeType::Min)
        {
            currentPosition = expression.insert(currentPosition, "min(");
            currentPosition++;
            currentPosition = expression.insert(currentPosition, ", ");
            StackEntry temp;
            temp.push_back(currentPosition);
            currentPosition++;
            currentPosition = expression.insert(currentPosition, ")");
            temp.push_back(currentPosition);
            itStack.push(temp);
        }
        else if (graph[vertex]->getType() == expr::NodeType::Max)
        {
            currentPosition = expression.insert(currentPosition, "max(");
            currentPosition++;
            currentPosition = expression.insert(currentPosition, ", ");
            StackEntry temp;
            temp.push_back(currentPosition);
            currentPosition++;
            currentPosition = expression.insert(currentPosition, ")");
            temp.push_back(currentPosition);
            itStack.push(temp);
        }
        else if (graph[vertex]->getType() == expr::NodeType::Floor)
        {
            currentPosition = expression.insert(currentPosition, "floor(");
            currentPosition++;
            currentPosition = expression.insert(currentPosition, ", ");
            StackEntry temp;
            temp.push_back(currentPosition);
            currentPosition++;
            currentPosition = expression.insert(currentPosition, ")");
            temp.push_back(currentPosition);
            itStack.push(temp);
        }
        else if (graph[vertex]->getType() == expr::NodeType::Ceil)
        {
            currentPosition = expression.insert(currentPosition, "ceil(");
            currentPosition++;
            currentPosition = expression.insert(currentPosition, ", ");
            StackEntry temp;
            temp.push_back(currentPosition);
            currentPosition++;
            currentPosition = expression.insert(currentPosition, ")");
            temp.push_back(currentPosition);
            itStack.push(temp);
        }
        else if (graph[vertex]->getType() == expr::NodeType::Round)
        {
            currentPosition = expression.insert(currentPosition, "round(");
            currentPosition++;
            currentPosition = expression.insert(currentPosition, ", ");
            StackEntry temp;
            temp.push_back(currentPosition);
            currentPosition++;
            currentPosition = expression.insert(currentPosition, ")");
            temp.push_back(currentPosition);
            itStack.push(temp);
        }
        else if (graph[vertex]->getType() == expr::NodeType::RadToDeg)
        {
            currentPosition = expression.insert(currentPosition, "radtodeg(");
            currentPosition++;
            currentPosition = expression.insert(currentPosition, ")");
            StackEntry temp;
            temp.push_back(currentPosition);
            itStack.push(temp);
        }
        else if (graph[vertex]->getType() == expr::NodeType::DegToRad)
        {
            currentPosition = expression.insert(currentPosition, "degtorad(");
            currentPosition++;
            currentPosition = expression.insert(currentPosition, ")");
            StackEntry temp;
            temp.push_back(currentPosition);
            itStack.push(temp);
        }
        else if (graph[vertex]->getType() == expr::NodeType::Log)
        {
            currentPosition = expression.insert(currentPosition, "log(");
            currentPosition++;
            currentPosition = expression.insert(currentPosition, ")");
            StackEntry temp;
            temp.push_back(currentPosition);
            itStack.push(temp);
        }
        else if (graph[vertex]->getType() == expr::NodeType::Exp)
        {
            currentPosition = expression.insert(currentPosition, "exp(");
            currentPosition++;
            currentPosition = expression.insert(currentPosition, ")");
            StackEntry temp;
            temp.push_back(currentPosition);
            itStack.push(temp);
        }
        else if (graph[vertex]->getType() == expr::NodeType::Sqrt)
        {
            currentPosition = expression.insert(currentPosition, "sqrt(");
            currentPosition++;
            currentPosition = expression.insert(currentPosition, ")");
            StackEntry temp;
            temp.push_back(currentPosition);
            itStack.push(temp);
        }
        else if (graph[vertex]->getType() == expr::NodeType::Hypot)
        {
            currentPosition = expression.insert(currentPosition, "hypot(");
            currentPosition++;
            currentPosition = expression.insert(currentPosition, ", ");
            StackEntry temp;
            temp.push_back(currentPosition);
            currentPosition++;
            currentPosition = expression.insert(currentPosition, ")");
            temp.push_back(currentPosition);
            itStack.push(temp);
        }
        else if (graph[vertex]->getType() == expr::NodeType::Conditional)
        {
            currentPosition = expression.insert(currentPosition, "if(");
            currentPosition++;
            StackEntry temp;

            expr::ConditionalNode *node = dynamic_cast<expr::ConditionalNode*>(graph[vertex].get());
            assert(node);
            if (node->type == expr::ConditionalNode::GreaterThan)
                currentPosition = expression.insert(currentPosition, " > ");
            else if (node->type == expr::ConditionalNode::LessThan)
                currentPosition = expression.insert(currentPosition, " < ");
            else if (node->type == expr::ConditionalNode::GreaterThanEqual)
                currentPosition = expression.insert(currentPosition, " >= ");
            else if (node->type == expr::ConditionalNode::LessThanEqual)
                currentPosition = expression.insert(currentPosition, " <= ");
            else if(node->type == expr::ConditionalNode::Equal)
                currentPosition = expression.insert(currentPosition, " == ");
            else if (node->type == expr::ConditionalNode::NotEqual)
                currentPosition = expression.insert(currentPosition, " != ");

            temp.push_back(currentPosition); //lhs position.
            currentPosition++;

            currentPosition = expression.insert(currentPosition, ")");
            temp.push_back(currentPosition); //rhs position.
            currentPosition++;

            currentPosition = expression.insert(currentPosition, " then(");
            currentPosition++;
            currentPosition = expression.insert(currentPosition, ")");
            temp.push_back(currentPosition); //then position.
            currentPosition++;

            currentPosition = expression.insert(currentPosition, " else(");
            currentPosition++;
            currentPosition = expression.insert(currentPosition, ")");
            temp.push_back(currentPosition); //else position.

            itStack.push(temp);
        }
    }
        
    template<typename FindEdge, typename FindGraph>
    void examine_edge(FindEdge edge, FindGraph &graph)
    {
        if (graph[edge] == expr::EdgeProperty::Lhs || graph[edge] == expr::EdgeProperty::Parameter1 ||
            graph[edge] == expr::EdgeProperty::None)
        {
            currentPosition = itStack.top().at(0);
        }
        else if(graph[edge] == expr::EdgeProperty::Rhs || graph[edge] == expr::EdgeProperty::Parameter2)
        {
            currentPosition = itStack.top().at(1);
        }
        else if(graph[edge] == expr::EdgeProperty::Then)
        {
            currentPosition = itStack.top().at(2);
        }
        else if(graph[edge] == expr::EdgeProperty::Else)
        {
            currentPosition = itStack.top().at(3);
        }
    }
      
    template <typename EdgeT, typename GraphT>
    void forward_or_cross_edge(EdgeT edge, GraphT &graph)
    {
        // only formula vertices should be target of a forward edge.
        assert(graph[boost::target(edge, graph)]->getType() == NodeType::Formula);
        FormulaNode *fNode = static_cast<FormulaNode*>(graph[boost::target(edge, graph)].get());
        currentPosition = expression.insert(currentPosition, fNode->name);
        currentPosition++;
    }
      
    template<typename FindVertex, typename FindGraph>
    void finish_vertex(FindVertex, FindGraph &)
    {
        itStack.pop();
        if (!itStack.empty())
        currentPosition = itStack.top().front();
    }
      
    //!This is the final result for output.
    std::list<std::string> &expression;
      
private:
    //!Type for mapping.
    typedef boost::unordered_map<expr::NodeType::Type, std::string> TypeMap;
    //!Map between operators and string chars. '+' '-' '*' '/'. This might go away.
    TypeMap typeMap;
    //!Current position for insertion into #expression.
    std::list<std::string>::iterator currentPosition;

    /*! @brief Type for entries into stack.
    * 
    * Each node may have multiple positions for string insertion. For example
    * an "if then else" node will have 4 positions for insertion: LHS RHS for the conditional
    * and the "then" and "else". Callbacks #discover_vertex and #finish_vertex will alter #itStack.
    * While edge visitation will move #currentPosition to reflect entries in the current top() of #itStack.
    */
    typedef std::vector<std::list<std::string>::iterator> StackEntry;
    //!Stack of positions(iterators) mirroring the current vertex visitation path.
    std::stack<StackEntry> itStack;
    //!The first vertex visited.
    expr::Vertex startVertex;
};
  
StringTranslatorStow::StringTranslatorStow(ExpressionManager &eManagerIn) : failedPosition(-1), eManager(eManagerIn),
  graphWrapper(eManager.getGraphWrapper())
{
}

StringTranslatorStow::~StringTranslatorStow(){}

int StringTranslatorStow::parseString(const std::string &formula)
{
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
  }
//   std::cout << "parse succeeded" << std::endl;
  
  return failedPosition;
}

boost::uuids::uuid StringTranslatorStow::getFormulaOutId() const
{
  assert(formulaNodeOut != Graph::null_vertex());
  return graphWrapper.getFormulaId(formulaNodeOut);
}

void StringTranslatorStow::cleanFailedParse()
{
  /*
   * note: we can NOT use GraphWrapper::cleanFormula here because cleanFormula
   * assumes a valid, connected subgraph. A failed parse might leave unconnected vertices
   * that cleanFormula will not visit.
   */
  
  for (std::vector<Edge>::iterator it = addedEdges.begin(); it != addedEdges.end(); ++it)
    boost::remove_edge(*it, graphWrapper.graph);
  
  bool removedFormulaNode = false;
  for (std::vector<Vertex>::iterator it = addedVertices.begin(); it != addedVertices.end(); ++it)
  {
    if (graphWrapper.graph[*it]->getType() == NodeType::Formula)
    {
      eManager.removeFormulaFromAllGroup(graphWrapper.getFormulaId(*it));
      removedFormulaNode = true;
    }
    boost::remove_vertex(*it, graphWrapper.graph);
  }
  /*by setting this clean we can remove the formula later without a call to update.
   * see GraphWrapper::removeFormula and it's use of 'getValue', which can assert.
   * not totally sure of the implications of this on formula value. We should be OK,
   * because any failed editing is reparsed with original string value, thus
   * making formula value dirty and forcing a recalc.
   */
  if (!removedFormulaNode)
    graphWrapper.graph[formulaNodeOut]->setClean();
}

void StringTranslatorStow::buildConstantNode(const double& valueIn)
{
  Vertex cNode = graphWrapper.buildConstantNode(valueIn);
  vStack.push(cNode);
  addedVertices.push_back(cNode);
}

void StringTranslatorStow::buildFormulaNode(const std::string& stringIn, bool &carryOn)
{
  Vertex fVertex = graphWrapper.graph.null_vertex();
  if (graphWrapper.hasFormula(stringIn))
  {
    fVertex = graphWrapper.getFormulaVertex(stringIn);
    if (boost::out_degree(fVertex, graphWrapper.graph) > 0)
    {
      //TODO error message.
      std::cout << "out degree in StringTranslatorStow::buildFormulaNode is: " << boost::out_degree(fVertex, graphWrapper.graph) << std::endl;
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
    eManager.addFormulaToAllGroup(graphWrapper.getFormulaId(fVertex));
    addedVertices.push_back(fVertex);
  }
  
  assert(fVertex != graphWrapper.graph.null_vertex());
  formulaNodeOut = fVertex;
  vStack.push(fVertex);
}

void StringTranslatorStow::buildAdditionNode()
{
//   assert(currentNode != graphWrapper.graph.null_vertex());
  
  Vertex aNode = graphWrapper.buildAdditionNode();
  vStack.push(aNode);
  addedVertices.push_back(aNode);
}

void StringTranslatorStow::buildSubractionNode()
{
  Vertex sNode = graphWrapper.buildSubractionNode();
  vStack.push(sNode);
  addedVertices.push_back(sNode);
}

void StringTranslatorStow::buildMultiplicationNode()
{
  Vertex mNode = graphWrapper.buildMultiplicationNode(); 
  vStack.push(mNode);
  addedVertices.push_back(mNode);
}

void StringTranslatorStow::buildDivisionNode()
{
  Vertex dNode = graphWrapper.buildDivisionNode();
  vStack.push(dNode);
  addedVertices.push_back(dNode);
}

void StringTranslatorStow::buildLinkNode(const std::string &stringIn, bool &carryOn)
{
  if(!graphWrapper.hasFormula(stringIn))
  {
    carryOn = false;
    return;
  }
  Vertex lVertex = graphWrapper.getFormulaVertex(stringIn);
  vStack.push(lVertex);
}

void StringTranslatorStow::startParenthesesNode()
{
  Vertex pNode = graphWrapper.buildParenthesesNode();
  vStack.push(pNode);
  addedVertices.push_back(pNode);
}

void StringTranslatorStow::finishParenthesesNode()
{
  Vertex op = vStack.top();
  vStack.pop();
  Vertex parentheses = vStack.top();
  buildEdgeNone(parentheses, op);
}

void StringTranslatorStow::buildSinNode()
{
  Vertex sNode = graphWrapper.buildSinNode();
  vStack.push(sNode);
  addedVertices.push_back(sNode);
}

void StringTranslatorStow::buildCosNode()
{
  Vertex cNode = graphWrapper.buildCosNode();
  vStack.push(cNode);
  addedVertices.push_back(cNode);
}

void StringTranslatorStow::buildTanNode()
{
  Vertex tNode = graphWrapper.buildTanNode();
  vStack.push(tNode);
  addedVertices.push_back(tNode);
}

void StringTranslatorStow::buildAsinNode()
{
  Vertex node = graphWrapper.buildAsinNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslatorStow::buildAcosNode()
{
  Vertex node = graphWrapper.buildAcosNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslatorStow::buildAtanNode()
{
  Vertex node = graphWrapper.buildAtanNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslatorStow::buildAtan2Node()
{
  Vertex node = graphWrapper.buildAtan2Node();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslatorStow::buildPowNode()
{
  Vertex node = graphWrapper.buildPowNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslatorStow::buildAbsNode()
{
  Vertex node = graphWrapper.buildAbsNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslatorStow::buildMinNode()
{
  Vertex node = graphWrapper.buildMinNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslatorStow::buildMaxNode()
{
  Vertex node = graphWrapper.buildMaxNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslatorStow::buildFloorNode()
{
  Vertex node = graphWrapper.buildFloorNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslatorStow::buildCeilNode()
{
  Vertex node = graphWrapper.buildCeilNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslatorStow::buildRoundNode()
{
  Vertex node = graphWrapper.buildRoundNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslatorStow::buildRadToDegNode()
{
  Vertex node = graphWrapper.buildRadToDegNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslatorStow::buildDegToRadNode()
{
  Vertex node = graphWrapper.buildDegToRadNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslatorStow::buildLogNode()
{
  Vertex node = graphWrapper.buildLogNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslatorStow::buildExpNode()
{
  Vertex node = graphWrapper.buildExpNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslatorStow::buildSqrtNode()
{
  Vertex node = graphWrapper.buildSqrtNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslatorStow::buildHypotNode()
{
  Vertex node = graphWrapper.buildHypotNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslatorStow::buildConditionalNode()
{
  Vertex node = graphWrapper.buildConditionalNode();
  vStack.push(node);
  addedVertices.push_back(node);
}

void StringTranslatorStow::setConditionGreaterThan()
{
  Vertex ifNode = vStack.top();
  ConditionalNode *node = dynamic_cast<ConditionalNode*>(graphWrapper.graph[ifNode].get());
  assert(node);
  node->type = ConditionalNode::GreaterThan;
}

void StringTranslatorStow::setConditionLessThan()
{
  Vertex ifNode = vStack.top();
  ConditionalNode *node = dynamic_cast<ConditionalNode*>(graphWrapper.graph[ifNode].get());
  assert(node);
  node->type = ConditionalNode::LessThan;
}

void StringTranslatorStow::setConditionGreaterThanEqual()
{
  Vertex ifNode = vStack.top();
  ConditionalNode *node = dynamic_cast<ConditionalNode*>(graphWrapper.graph[ifNode].get());
  assert(node);
  node->type = ConditionalNode::GreaterThanEqual;
}

void StringTranslatorStow::setConditionLessThanEqual()
{
  Vertex ifNode = vStack.top();
  ConditionalNode *node = dynamic_cast<ConditionalNode*>(graphWrapper.graph[ifNode].get());
  assert(node);
  node->type = ConditionalNode::LessThanEqual;
}

void StringTranslatorStow::setConditionEqual()
{
  Vertex ifNode = vStack.top();
  ConditionalNode *node = dynamic_cast<ConditionalNode*>(graphWrapper.graph[ifNode].get());
  assert(node);
  node->type = ConditionalNode::Equal;
}

void StringTranslatorStow::setConditionNotEqual()
{
  Vertex ifNode = vStack.top();
  ConditionalNode *node = dynamic_cast<ConditionalNode*>(graphWrapper.graph[ifNode].get());
  assert(node);
  node->type = ConditionalNode::NotEqual;
}

void StringTranslatorStow::setConditionLhs()
{
  Vertex lhs = vStack.top();
  vStack.pop();
  Vertex ifNode = vStack.top();
  Edge edge = buildEdgeCommon(ifNode, lhs);
  graphWrapper.graph[edge] = EdgeProperty::Lhs;
}

void StringTranslatorStow::setConditionRhs()
{
  Vertex rhs = vStack.top();
  vStack.pop();
  Vertex ifNode = vStack.top();
  Edge edge = buildEdgeCommon(ifNode, rhs);
  graphWrapper.graph[edge] = EdgeProperty::Rhs;
}

void StringTranslatorStow::setConditionThen()
{
  Vertex thenNode = vStack.top();
  vStack.pop();
  Vertex ifNode = vStack.top();
  Edge edge = buildEdgeCommon(ifNode, thenNode);
  graphWrapper.graph[edge] = EdgeProperty::Then;
}

void StringTranslatorStow::setConditionElse()
{
  Vertex elseNode = vStack.top();
  vStack.pop();
  Vertex ifNode = vStack.top();
  Edge edge = buildEdgeCommon(ifNode, elseNode);
  graphWrapper.graph[edge] = EdgeProperty::Else;
}

void StringTranslatorStow::setParameter1()
{
  Vertex p1Node = vStack.top();
  vStack.pop();
  Vertex functionNode = vStack.top();
  Edge edge = buildEdgeCommon(functionNode, p1Node);
  graphWrapper.graph[edge] = EdgeProperty::Parameter1;
}

void StringTranslatorStow::setParameter2()
{
  Vertex p2Node = vStack.top();
  vStack.pop();
  Vertex functionNode = vStack.top();
  Edge edge = buildEdgeCommon(functionNode, p2Node);
  graphWrapper.graph[edge] = EdgeProperty::Parameter2;
}

void StringTranslatorStow::finishFunction1()
{
  Vertex child = vStack.top();
  vStack.pop();
  Vertex trig = vStack.top();
  buildEdgeNone(trig, child);
}

void StringTranslatorStow::buildEdgeLHS(const Vertex &source, const Vertex &target)
{
  Edge edgeLhs = buildEdgeCommon(source, target);
  graphWrapper.graph[edgeLhs] = EdgeProperty::Lhs;
}

void StringTranslatorStow::buildEdgeRHS(const Vertex &source, const Vertex &target)
{
  Edge edgeRhs = buildEdgeCommon(source, target);
  graphWrapper.graph[edgeRhs] = EdgeProperty::Rhs;
}

void StringTranslatorStow::buildEdgeNone(const Vertex& source, const Vertex& target)
{
  Edge edgeNone = buildEdgeCommon(source, target);
  graphWrapper.graph[edgeNone] = EdgeProperty::None;
}

Edge StringTranslatorStow::buildEdgeCommon(const Vertex &source, const Vertex &target)
{
  assert(source != graphWrapper.graph.null_vertex());
  assert(target != graphWrapper.graph.null_vertex());
  
  Edge anEdge = graphWrapper.buildEdge(source, target);
  addedEdges.push_back(anEdge);
  
  return(anEdge);
}

void StringTranslatorStow::makeCurrentRHS()
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

void StringTranslatorStow::finish()
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

std::string StringTranslatorStow::buildStringRhs(const boost::uuids::uuid &idIn) const
{
  assert(graphWrapper.hasFormula(idIn));
  Vertex fVertex = graphWrapper.getFormulaVertex(idIn);
  std::list<std::string> expression = buildStringCommon(fVertex);
  assert(!expression.empty());
  std::ostringstream stream;
  //first entry is name + '='. skip
  std::copy(++(expression.begin()), expression.end(), std::ostream_iterator<std::string>(stream));
  return stream.str();
}

std::string StringTranslatorStow::buildStringAll(const boost::uuids::uuid &idIn) const
{
  assert(graphWrapper.hasFormula(idIn));
  Vertex fVertex = graphWrapper.getFormulaVertex(idIn);
  std::list<std::string> expression = buildStringCommon(fVertex);
  assert(!expression.empty());
  std::ostringstream stream;
  std::copy(expression.begin(), expression.end(), std::ostream_iterator<std::string>(stream));
  return stream.str();
}

/*! used in buildStringCommon to 'clean' extranous graph vertices
 * for building of string representation. After ran through dfs search,
 * verticesOut should only contain relevant vertices for passed in
 * start vertex and any 'sub formula' vertices. sub formula vertices
 * will be gone.
 */
template<typename VertexT>
class DepthLimitVisitor : public boost::default_dfs_visitor
{
public:
  DepthLimitVisitor(std::vector<VertexT> &verticesOutIn) :
    verticesOut(verticesOutIn) {}
  
  template<typename GraphT>
  void discover_vertex(VertexT vertex, GraphT &graph)
  {
    if (graph[vertex]->getType() == NodeType::Formula)
    {
      //we grab up to 2 formula vertices. the original and 1 child deep.
      formulaStack.push(vertex);
      if (formulaStack.size() < 3)
        verticesOut.push_back(vertex);
    }
    else if (formulaStack.size() == 1) //grab all 'non formula' vertices that are 1st generation children.
      verticesOut.push_back(vertex);
  }
  
  template <typename EdgeT, typename GraphT>
  void forward_or_cross_edge(EdgeT edge, GraphT &graph)
  {
    // only formula vetices should be target of a forward edge.
    assert(graph[boost::target(edge, graph)]->getType() == NodeType::Formula);
    if (formulaStack.size() == 1)
      verticesOut.push_back(boost::target(edge, graph));
  }
  
  template<typename GraphT>
  void finish_vertex(VertexT vertex, GraphT &graph)
  {
    if (graph[vertex]->getType() == NodeType::Formula)
      formulaStack.pop();
  }
  
  std::vector<VertexT> &verticesOut;
  std::stack<VertexT> formulaStack;
};


std::list< std::string > StringTranslatorStow::buildStringCommon(const Vertex& vertexIn) const
{
  graphWrapper.indexVerticesAndEdges();
  
  /* was initially trying to do it all with one 1 dfs and 1 visitor. this was becoming really
   * hard to reason. So I am taking a more divided(slower) approach.
   * filter to only connected vertices by using a bfs and a collection visitor.
   * filter out and any 'sub-formula' nodes using a dfs and collection visitor.
   * use existing formula visitor to build string
   */
  
  std::vector<Vertex> limitVertices;
  gu::BFSLimitVisitor<Vertex>limitVisitor(limitVertices);
  boost::breadth_first_search(graphWrapper.graph, vertexIn, visitor(limitVisitor));
  
  gu::SubsetFilter<Graph> filter(graphWrapper.graph, limitVertices);
  typedef boost::filtered_graph<Graph, boost::keep_all, gu::SubsetFilter<Graph> > FilteredGraph;
  typedef boost::graph_traits<FilteredGraph>::vertex_descriptor FilteredVertex;
  FilteredGraph filteredGraph(graphWrapper.graph, boost::keep_all(), filter);
  
  std::vector<FilteredVertex> depthVertices;
  DepthLimitVisitor<FilteredVertex> depthVisitor(depthVertices);
  boost::depth_first_search(filteredGraph, boost::visitor(depthVisitor).root_vertex(vertexIn));
  
  gu::SubsetFilter<FilteredGraph> filterDepth(filteredGraph, depthVertices);
  typedef boost::filtered_graph<FilteredGraph, boost::keep_all, gu::SubsetFilter<FilteredGraph> > DepthFilteredGraph;
  DepthFilteredGraph depthFilteredGraph(filteredGraph, boost::keep_all(), filterDepth);
  
  std::list<std::string> expression;
  
  FormulaVisitor visitor(expression);
  boost::depth_first_search(depthFilteredGraph, boost::visitor(visitor).root_vertex(vertexIn));
  
  return expression;
}

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

#ifndef STRINGTRANSLATOR_H
#define STRINGTRANSLATOR_H

#include <stack>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_utree.hpp>
#include <boost/spirit/include/phoenix_function.hpp>
#include <boost/spirit/include/phoenix_bind.hpp>
#include <boost/graph/depth_first_search.hpp>

#include <expressions/expressiongraph.h>
#include <expressions/expressionmanager.h>

#define BOOST_SPIRIT_DEBUG

using boost::spirit::qi::grammar;
using boost::spirit::qi::rule;
using boost::spirit::qi::_val;
using boost::spirit::qi::_1;
using boost::spirit::qi::_pass;
using boost::spirit::utree;
using boost::spirit::ascii::space_type;
using boost::spirit::utf8_symbol_range_type;


namespace expr
{
/*! @brief Interface between GraphWrapper and text strings
 * 
 * Using boost::spirit + boost::phoenix, StringTranslator translates an input string
 * into boost::graph vertices containing AbstractNode decendants for evaluation. For translation
 * out, StringTranslator using a boost graph visitor, FormulaVisitor. 
 */
class StringTranslator
{
public:
  //! No doc yet. this will probably change.
  enum TotalState{None, ParseFailed, ParseSucceeded, GraphUpdateFailed, GraphUpdateSucceeded};
  //! construct a translator
  StringTranslator(expr::ExpressionManager &eManagerIn);
  //! No doc yet.
  TotalState getTotalState(){return totalState;}
  
  /*! @brief Construct graph representation from the string.
   * 
   * String must take the form of: LHS = RHS. Where LHS will be just a name. A name cannot
   * start with a number and must not be equal to keywords or the parse will fail. An example formula: 
   * @code test = 8 + 3 - sin(0.75) * 4 / 2.1 + 3. @endcode
   * If an expression with the name already exists, it will be modified. Otherwise a new expression 
   * will be created. If the parsing fails, 
   * the failing position is recorded in the member #failedPosition. @note If parsing fails on an
   * already existing formula, the previous formulas RHS is lost. User should cache the current string before
   * calling parse, so the formula can be put back with another call to parse, if so desired. An example
   * of this can be seen in TableModel::setData.
   * 
   * If the parsing succeeds then
   * the newly created formula graph vertex is recorded in member #formulaNodeOut. @todo put something
   * here to describe the supported grammer.
   */
  TotalState parseString(const std::string &formula);
  
  //@{
  //! Graph construction callbacks invoked from grammar and boost::phoenix in #ExpressionGrammar.
  void buildConstantNode(const double &valueIn);
  void buildFormulaNode(const std::string &stringIn, bool &carryOn);
  void buildAdditionNode();
  void buildSubractionNode();
  void buildMultiplicationNode();
  void buildDivisionNode();
  void startParenthesesNode();
  void buildLinkNode(const std::string &stringIn, bool &carryOn);
  void makeCurrentRHS();
  void finish();
  void finishParenthesesNode();
  void buildSinNode();
  void buildCosNode();
  void buildTanNode();
  void buildAsinNode();
  void buildAcosNode();
  void buildAtanNode();
  void buildAtan2Node();
  void buildPowNode();
  void buildAbsNode();
  void buildMinNode();
  void buildMaxNode();
  void buildFloorNode();
  void buildCeilNode();
  void buildRoundNode();
  void buildRadToDegNode();
  void buildDegToRadNode();
  void buildLogNode();
  void buildExpNode();
  void buildSqrtNode();
  void buildHypotNode();
  void buildConditionalNode();
  void setConditionGreaterThan();
  void setConditionLessThan();
  void setConditionGreaterThanEqual();
  void setConditionLessThanEqual();
  void setConditionEqual();
  void setConditionNotEqual();
  void setConditionLhs();
  void setConditionRhs();
  void setConditionThen();
  void setConditionElse();
  void setParameter1();
  void setParameter2();
  void finishFunction1();
  //@}
  
  //! Build the RHS string. No formula name or equals.
  std::string buildStringRhs(const expr::Vertex &vertexIn) const;
  //! Build the entire string.
  std::string buildStringAll(const expr::Vertex &vertexIn) const;
  //! Build string common. Used in both #buildStringRhs and #buildStringAll.
  std::list<std::string> buildStringCommon(const expr::Vertex &vertexIn) const;
  //! This is the main formula node created or edited by the parse.
  expr::Vertex getFormulaNodeOut();
  //! Failing position of the parse. -1 means no failure.
  int failedPosition; //-1 means no failure.
  
protected:
  //@{
  //! Convenience to build graph edges with properties.
  void buildEdgeLHS(const expr::Vertex &source, const expr::Vertex &target);
  void buildEdgeRHS(const expr::Vertex &source, const expr::Vertex &target);
  void buildEdgeNone(const expr::Vertex &source, const expr::Vertex &target);
  expr::Edge buildEdgeCommon(const expr::Vertex &source, const expr::Vertex &target);
  //@}
  
  /*! @brief Clean a failed parse 
   * 
   * If the parsing fails, this function will be called from parseString to undo any changes during parse.
   * This will not restore any nodes that may have existed on an already existing formula. After this call,
   * There will only be the formula node itself.
   */
  void cleanFailedParse();
  
  //!ExpressionManager to work on.
  expr::ExpressionManager &eManager;
  //!GraphWrapper to work on.
  expr::GraphWrapper &graphWrapper;
  //!No doc yet.
  TotalState totalState;
  
  //! Stack of graph vertices used during parsing
 std::stack<expr::Vertex> vStack;
 //! All the vertices added while parsing.
 std::vector<expr::Vertex> addedVertices;
 //! All the edges added while parsing.
 std::vector<expr::Edge> addedEdges;
 //! The main formula node created/modified by last parse.
 expr::Vertex formulaNodeOut;
};

/*! @brief Describes boost::spirit parsing grammar
 * 
 * @todo Explain acceptable string format.
 */
template <typename Iterator>
struct ExpressionGrammar : grammar<Iterator, space_type>
{
  ExpressionGrammar(StringTranslator &translatorIn) : ExpressionGrammar::base_type(expression), translator(translatorIn)
  {
    using boost::spirit::qi::alpha;
    using boost::spirit::qi::alnum;
    using boost::spirit::qi::punct;
    using boost::spirit::qi::double_;
    using boost::spirit::qi::char_;
    using boost::spirit::qi::lit;
    
    expression = formulaName [boost::phoenix::bind(&StringTranslator::buildFormulaNode, &translator, boost::spirit::_1, _pass)] >>
              '=' >> rhs [boost::phoenix::bind(&StringTranslator::finish, &translator)];
    rhs = term >>
          *((char_('+') [boost::phoenix::bind(&StringTranslator::buildAdditionNode, &translator)] >>
          term [boost::phoenix::bind(&StringTranslator::makeCurrentRHS, &translator)]) |
          (char_('-') [boost::phoenix::bind(&StringTranslator::buildSubractionNode, &translator)] >>
          term [boost::phoenix::bind(&StringTranslator::makeCurrentRHS, &translator)]));
    item = double_ [boost::phoenix::bind(&StringTranslator::buildConstantNode, &translator, boost::spirit::_1)] |
                    linkName [boost::phoenix::bind(&StringTranslator::buildLinkNode, &translator, boost::spirit::_1, _pass)];
    function1 = (lit("sin(") [boost::phoenix::bind(&StringTranslator::buildSinNode, &translator)] |
               lit("cos(") [boost::phoenix::bind(&StringTranslator::buildCosNode, &translator)] |
               lit("tan(") [boost::phoenix::bind(&StringTranslator::buildTanNode, &translator)] |
               lit("asin(") [boost::phoenix::bind(&StringTranslator::buildAsinNode, &translator)] |
               lit("acos(") [boost::phoenix::bind(&StringTranslator::buildAcosNode, &translator)] |
               lit("atan(") [boost::phoenix::bind(&StringTranslator::buildAtanNode, &translator)] |
               lit("radtodeg(") [boost::phoenix::bind(&StringTranslator::buildRadToDegNode, &translator)] |
               lit("degtorad(") [boost::phoenix::bind(&StringTranslator::buildDegToRadNode, &translator)] |
               lit("log(") [boost::phoenix::bind(&StringTranslator::buildLogNode, &translator)] |
               lit("exp(") [boost::phoenix::bind(&StringTranslator::buildExpNode, &translator)] |
               lit("sqrt(") [boost::phoenix::bind(&StringTranslator::buildSqrtNode, &translator)] |
               lit("abs(") [boost::phoenix::bind(&StringTranslator::buildAbsNode, &translator)]) >> rhs >>
               char_(')') [boost::phoenix::bind(&StringTranslator::finishFunction1, &translator)];
    function2 = (lit("pow(") [boost::phoenix::bind(&StringTranslator::buildPowNode, &translator)] |
                lit("atan2(") [boost::phoenix::bind(&StringTranslator::buildAtan2Node, &translator)] |
                lit("min(") [boost::phoenix::bind(&StringTranslator::buildMinNode, &translator)] |
                lit("max(") [boost::phoenix::bind(&StringTranslator::buildMaxNode, &translator)] |
                lit("floor(") [boost::phoenix::bind(&StringTranslator::buildFloorNode, &translator)] |
                lit("ceil(") [boost::phoenix::bind(&StringTranslator::buildCeilNode, &translator)] |
                lit("round(") [boost::phoenix::bind(&StringTranslator::buildRoundNode, &translator)] | 
                lit("hypot(") [boost::phoenix::bind(&StringTranslator::buildHypotNode, &translator)]) >> rhs >>
                char_(',') [boost::phoenix::bind(&StringTranslator::setParameter1, &translator)] >> rhs >>
                char_(')') [boost::phoenix::bind(&StringTranslator::setParameter2, &translator)];
    //when one parser is a subset of another (i.e. '>' and '>=') put the larger first, so it fails before trying the smaller.
    conditionalOperator = lit(">=") [boost::phoenix::bind(&StringTranslator::setConditionGreaterThanEqual, &translator)] |
                          lit("<=") [boost::phoenix::bind(&StringTranslator::setConditionLessThanEqual, &translator)] |
                          lit(">") [boost::phoenix::bind(&StringTranslator::setConditionGreaterThan, &translator)] |
                          lit("<") [boost::phoenix::bind(&StringTranslator::setConditionLessThan, &translator)] |
                          lit("==") [boost::phoenix::bind(&StringTranslator::setConditionEqual, &translator)] |
                          lit("!=") [boost::phoenix::bind(&StringTranslator::setConditionNotEqual, &translator)];
    condition = rhs [boost::phoenix::bind(&StringTranslator::setConditionLhs, &translator)] >> conditionalOperator >>
                rhs [boost::phoenix::bind(&StringTranslator::setConditionRhs, &translator)];
    ifCondition = lit("if(") [boost::phoenix::bind(&StringTranslator::buildConditionalNode, &translator)] >>
                  condition >> lit(")") >>
                  lit("then(") >> rhs [boost::phoenix::bind(&StringTranslator::setConditionThen, &translator)] >> char_(')') >>
                  lit("else(") >> rhs [boost::phoenix::bind(&StringTranslator::setConditionElse, &translator)] >> char_(')');
    keyword = lit("sin(") | lit("cos(") | lit("tan(") | lit("asin(") | lit("acos(") | lit("atan(") | lit("pow(") |
              lit("atan2(") | lit("abs(") | lit("min(") | lit("max(") | lit("floor(") | lit("ceil(") | lit("round(") |
              lit("radtodeg(") | lit("degtorad(") | lit("log(") | lit("exp(") | lit("sqrt(") | lit("hypot(") | lit("if(");
    linkName = (alpha >> *(alnum | char_('_'))) - keyword;
    formulaName = (alpha >> *(alnum | char_('_'))) - keyword;
    factor = item | function1 | function2 | ifCondition |
                    char_('(') [boost::phoenix::bind(&StringTranslator::startParenthesesNode, &translator)] >> rhs >>
                    char_ (')') [boost::phoenix::bind(&StringTranslator::finishParenthesesNode, &translator)];
    term = factor >>
                  *((char_('*') [boost::phoenix::bind(&StringTranslator::buildMultiplicationNode, &translator)] >>
                  factor [boost::phoenix::bind(&StringTranslator::makeCurrentRHS, &translator)]) |
                  (char_('/') [boost::phoenix::bind(&StringTranslator::buildDivisionNode, &translator)] >>
                  factor [boost::phoenix::bind(&StringTranslator::makeCurrentRHS, &translator)]));
    
    BOOST_SPIRIT_DEBUG_NODE(expression);
    BOOST_SPIRIT_DEBUG_NODE(rhs);
    BOOST_SPIRIT_DEBUG_NODE(item);
    BOOST_SPIRIT_DEBUG_NODE(keyword);
    BOOST_SPIRIT_DEBUG_NODE(linkName);
    BOOST_SPIRIT_DEBUG_NODE(formulaName);
    BOOST_SPIRIT_DEBUG_NODE(term);
    BOOST_SPIRIT_DEBUG_NODE(factor);
    BOOST_SPIRIT_DEBUG_NODE(function1);
    BOOST_SPIRIT_DEBUG_NODE(function2);
    BOOST_SPIRIT_DEBUG_NODE(conditionalOperator);
    BOOST_SPIRIT_DEBUG_NODE(condition);
    BOOST_SPIRIT_DEBUG_NODE(ifCondition);
  }
  rule<Iterator, space_type> expression;
  rule<Iterator, space_type> rhs;
  rule<Iterator, space_type> item;
  rule<Iterator, space_type> term;
  rule<Iterator, space_type> factor;
  rule<Iterator, space_type> function1;
  rule<Iterator, space_type> function2;
  rule<Iterator, space_type> conditionalOperator;
  rule<Iterator, space_type> condition;
  rule<Iterator, space_type> ifCondition;
  rule<Iterator, space_type, std::string()> keyword;
  rule<Iterator, space_type, std::string()> linkName;
  rule<Iterator, space_type, std::string()> formulaName;
  
  StringTranslator &translator;
};

/*! @brief Depth first search to build a representation string of a formula.
 * 
 * Traverses child vertices of a formulaNode. Will visit dependent formulaNodes,
 * but not their children. This is used to build a string representation of the formulaNode.
 * */
class FormulaVisitor : public boost::default_dfs_visitor
{
public:
    FormulaVisitor(std::list<std::string> &stringIn) : expression(stringIn), currentPosition(expression.begin()), finished(false)
    {
      typeMap.insert(std::make_pair(expr::NodeType::Addition, " + "));
      typeMap.insert(std::make_pair(expr::NodeType::Subtraction, " - "));
      typeMap.insert(std::make_pair(expr::NodeType::Multiplication, " * "));
      typeMap.insert(std::make_pair(expr::NodeType::Division, " / "));
    }
    
    template<typename FindVertex, typename FindGraph>
    void start_vertex(FindVertex vertex, FindGraph &graph)
    {
      startVertex = vertex;
    }
    
    //watchout for storing iterator == end();
    
    template<typename FindVertex, typename FindGraph>
    void discover_vertex(FindVertex vertex, FindGraph &graph)
    {
      if (finished)
        return;
      
      if (graph[vertex]->getType() == expr::NodeType::Formula)
      {
        expr::FormulaNode *fNode = dynamic_cast<expr::FormulaNode *>(graph[vertex].get());
        if (startVertex == vertex)
        {
          //found root formula.
          currentPosition = expression.insert(currentPosition, fNode->name + " = ");
          currentPosition++;
          StackEntry temp;
          temp.push_back(currentPosition);
          itStack.push(temp);
        }
        else
        {
          if (subFormulaVertices.empty())
          {
            //we want the first formula name but no more.
            currentPosition = expression.insert(currentPosition, fNode->name);
            currentPosition++;
            StackEntry temp;
            temp.push_back(currentPosition);
            itStack.push(temp);
          }
          subFormulaVertices.push(vertex);
        }
      }
      else if (!subFormulaVertices.empty())
        return;
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
      if (finished || !subFormulaVertices.empty())
        return;
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
    
    template<typename FindVertex, typename FindGraph>
    void finish_vertex(FindVertex vertex, FindGraph &graph)
    {
      if (finished)
        return;
      if (vertex == startVertex)
        finished = true;
      if ((graph[vertex]->getType() == expr::NodeType::Formula) && (!subFormulaVertices.empty()))
        subFormulaVertices.pop();
      if (!subFormulaVertices.empty())
        return;
      
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
  //!Keeps track when we cross into sub formula vertices, so we can ignore them.
  std::stack<expr::Vertex> subFormulaVertices;
  //!The first vertex visited.
  expr::Vertex startVertex;
  
  /*! @brief Signals that the visitation is finished.
   * 
   * With a dfs search all vertices will be visited. It only starts with a starting vertex
   * and is not constrained to it's tree. So we keep track of when the #startVertex is
   * finished so we can ignore all subsequent calls to #discover_vertex.
   */
  bool finished;
};
}

#endif // STRINGTRANSLATOR_H

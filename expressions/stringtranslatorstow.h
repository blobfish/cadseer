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

#ifndef EXPR_STRINGTRANSLATORSTOW_H
#define EXPR_STRINGTRANSLATORSTOW_H

#include <expressions/graph.h>
#include <expressions/manager.h>

namespace expr
{
  /*! @brief Interface between GraphWrapper and text strings
  * 
  * Using boost::spirit + boost::phoenix, StringTranslator translates an input string
  * into boost::graph vertices containing AbstractNode decendants for evaluation. For translation
  * out, StringTranslator using a boost graph visitor, FormulaVisitor. 
  */
  class StringTranslatorStow
  {
  public:
    //! construct a translator
    StringTranslatorStow(expr::Manager &eManagerIn);
    ~StringTranslatorStow();
    
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
    int parseString(const std::string &formula);
    
    //@{
    //! Graph construction callbacks invoked from grammar and boost::phoenix in #ExpressionGrammar.
    void buildScalarConstantNode(const double &valueIn);
    void buildFormulaNode(const std::string &stringIn, bool &carryOn);
    void buildScalarAdditionNode();
    void buildSubractionNode();
    void buildScalarMultiplicationNode();
    void buildScalarDivisionNode();
    void startScalarParenthesesNode();
    void buildLinkNode(const std::string &stringIn, bool &carryOn);
    void makeCurrentRHS(bool &carryOn);
    void finish();
    void finishScalarParenthesesNode();
    void buildScalarSinNode();
    void buildScalarCosNode();
    void buildScalarTanNode();
    void buildScalarAsinNode();
    void buildScalarAcosNode();
    void buildScalarAtanNode();
    void buildScalarAtan2Node();
    void buildScalarPowNode();
    void buildScalarAbsNode();
    void buildScalarMinNode();
    void buildScalarMaxNode();
    void buildScalarFloorNode();
    void buildScalarCeilNode();
    void buildScalarRoundNode();
    void buildScalarRadToDegNode();
    void buildScalarDegToRadNode();
    void buildScalarLogNode();
    void buildScalarExpNode();
    void buildScalarSqrtNode();
    void buildScalarHypotNode();
    void buildScalarConditionalNode();
    void setConditionGreaterThan();
    void setConditionLessThan();
    void setConditionGreaterThanEqual();
    void setConditionLessThanEqual();
    void setConditionEqual();
    void setConditionNotEqual();
    void setConditionLhs(bool &carryOn);
    void setConditionRhs(bool &carryOn);
    void setConditionThen();
    void setConditionElse();
    void setParameter1(bool &carryOn);
    void setParameter2(bool &carryOn);
    void finishFunction1(bool &carryOn);
    void buildVectorConstantNode();
    void buildQuatConstantNode();
    //@}
    
    //! Build the RHS string. No formula name or equals.
    std::string buildStringRhs(const boost::uuids::uuid&) const;
    //! Build the entire string.
    std::string buildStringAll(const boost::uuids::uuid&) const;
    //! This is the main formula node created or edited by the parse.
    boost::uuids::uuid getFormulaOutId() const;
    //! Failing position of the parse. -1 means no failure.
    int failedPosition; //-1 means no failure.
    //! A message relevant to parsing failure.
    std::string failureMessage;
    
  protected:
    //@{
    //! Convenience to build graph edges with properties.
    void buildEdgeLHS(const expr::Vertex &source, const expr::Vertex &target);
    void buildEdgeRHS(const expr::Vertex &source, const expr::Vertex &target);
    void buildEdgeNone(const expr::Vertex &source, const expr::Vertex &target);
    expr::Edge buildEdgeCommon(const expr::Vertex &source, const expr::Vertex &target);
    //@}
    
    //! Used by both #buildStringRhs and #buildStringAll.
    std::list<std::string> buildStringCommon(const expr::Vertex &vertexIn) const;
    
    /*! @brief Clean a failed parse 
    * 
    * If the parsing fails, this function will be called from parseString to undo any changes during parse.
    * This will not restore any nodes that may have existed on an already existing formula. After this call,
    * There will only be the formula node itself.
    */
    void cleanFailedParse();
    
    //!Manager to work on.
    expr::Manager &eManager;
    //!GraphWrapper to work on.
    expr::GraphWrapper &graphWrapper;
    
    //! Stack of graph vertices used during parsing
  std::stack<expr::Vertex> vStack;
  //! All the vertices added while parsing.
  std::vector<expr::Vertex> addedVertices;
  //! All the edges added while parsing.
  std::vector<expr::Edge> addedEdges;
  //! The main formula node created/modified by last parse.
  expr::Vertex formulaNodeOut;
  };
}

#endif //EXPR_STRINGTRANSLATORSTOW_H

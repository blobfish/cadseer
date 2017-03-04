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

#ifndef EXPR_GRAMMER_H
#define EXPR_GRAMMER_H

// #define BOOST_SPIRIT_DEBUG

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_utree.hpp>
#include <boost/spirit/include/phoenix_function.hpp>
#include <boost/spirit/include/phoenix_bind.hpp>

#include <expressions/stringtranslatorstow.h>


namespace bp = boost::phoenix;
namespace bs = boost::spirit;

using bs::qi::grammar;
using bs::qi::rule;
using bs::qi::_val;
using bs::qi::_pass;
using bs::utree;
using bs::ascii::space_type;
using bs::utf8_symbol_range_type;


namespace expr
{
  typedef StringTranslatorStow Translator;
  
  /*! @brief Describes boost::spirit parsing grammar
  * 
  * @todo Explain acceptable string format.
  */
  template <typename Iterator>
  struct ExpressionGrammar : grammar<Iterator, space_type>
  {
    ExpressionGrammar(Translator &translatorIn) : ExpressionGrammar::base_type(expression), translator(translatorIn)
    {
      using bs::qi::alpha;
      using bs::qi::alnum;
      using bs::qi::punct;
      using bs::qi::double_;
      using bs::qi::char_;
      using bs::qi::lit;
      
    expression = formulaName [bp::bind(&Translator::buildFormulaNode, &translator, bs::_1, _pass)] >>
        '=' >>
        (
          quatRhs [bp::bind(&Translator::finish, &translator)]
          | vectorRhs [bp::bind(&Translator::finish, &translator)]
          | scalarRhs [bp::bind(&Translator::finish, &translator)]
        );
    scalarRhs = term >>
        *((char_('+') [bp::bind(&Translator::buildScalarAdditionNode, &translator)] >>
        term [bp::bind(&Translator::makeCurrentRHS, &translator, _pass)]) |
        (char_('-') [bp::bind(&Translator::buildSubractionNode, &translator)] >>
        term [bp::bind(&Translator::makeCurrentRHS, &translator, _pass)]));
    scalarItem = double_ [bp::bind(&Translator::buildScalarConstantNode, &translator, bs::_1)] |
        linkName [bp::bind(&Translator::buildLinkNode, &translator, bs::_1, _pass)];
    function1 = (lit("sin(") [bp::bind(&Translator::buildScalarSinNode, &translator)] |
        lit("cos(") [bp::bind(&Translator::buildScalarCosNode, &translator)] |
        lit("tan(") [bp::bind(&Translator::buildScalarTanNode, &translator)] |
        lit("asin(") [bp::bind(&Translator::buildScalarAsinNode, &translator)] |
        lit("acos(") [bp::bind(&Translator::buildScalarAcosNode, &translator)] |
        lit("atan(") [bp::bind(&Translator::buildScalarAtanNode, &translator)] |
        lit("radtodeg(") [bp::bind(&Translator::buildScalarRadToDegNode, &translator)] |
        lit("degtorad(") [bp::bind(&Translator::buildScalarDegToRadNode, &translator)] |
        lit("log(") [bp::bind(&Translator::buildScalarLogNode, &translator)] |
        lit("exp(") [bp::bind(&Translator::buildScalarExpNode, &translator)] |
        lit("sqrt(") [bp::bind(&Translator::buildScalarSqrtNode, &translator)] |
        lit("abs(") [bp::bind(&Translator::buildScalarAbsNode, &translator)]) >> scalarRhs >>
        char_(')') [bp::bind(&Translator::finishFunction1, &translator, _pass)];
    function2 = (lit("pow(") [bp::bind(&Translator::buildScalarPowNode, &translator)] |
        lit("atan2(") [bp::bind(&Translator::buildScalarAtan2Node, &translator)] |
        lit("min(") [bp::bind(&Translator::buildScalarMinNode, &translator)] |
        lit("max(") [bp::bind(&Translator::buildScalarMaxNode, &translator)] |
        lit("floor(") [bp::bind(&Translator::buildScalarFloorNode, &translator)] |
        lit("ceil(") [bp::bind(&Translator::buildScalarCeilNode, &translator)] |
        lit("round(") [bp::bind(&Translator::buildScalarRoundNode, &translator)] | 
        lit("hypot(") [bp::bind(&Translator::buildScalarHypotNode, &translator)]) >> scalarRhs >>
        char_(',') [bp::bind(&Translator::setParameter1, &translator, _pass)] >> scalarRhs >>
        char_(')') [bp::bind(&Translator::setParameter2, &translator, _pass)];
    //when one parser is a subset of another (i.e. '>' and '>=') put the larger first, so it fails before trying the smaller.
    conditionalOperator = lit(">=") [bp::bind(&Translator::setConditionGreaterThanEqual, &translator)] |
        lit("<=") [bp::bind(&Translator::setConditionLessThanEqual, &translator)] |
        lit(">") [bp::bind(&Translator::setConditionGreaterThan, &translator)] |
        lit("<") [bp::bind(&Translator::setConditionLessThan, &translator)] |
        lit("==") [bp::bind(&Translator::setConditionEqual, &translator)] |
        lit("!=") [bp::bind(&Translator::setConditionNotEqual, &translator)];
    condition = scalarRhs [bp::bind(&Translator::setConditionLhs, &translator, _pass)] >> conditionalOperator >>
        scalarRhs [bp::bind(&Translator::setConditionRhs, &translator, _pass)];
    ifCondition = lit("if(") [bp::bind(&Translator::buildScalarConditionalNode, &translator)] >>
        condition >> lit(")") >>
        lit("then(") >> scalarRhs [bp::bind(&Translator::setConditionThen, &translator)] >> char_(')') >>
        lit("else(") >> scalarRhs [bp::bind(&Translator::setConditionElse, &translator)] >> char_(')');
    keyword = lit("sin(") | lit("cos(") | lit("tan(") | lit("asin(") | lit("acos(") | lit("atan(") | lit("pow(") |
        lit("atan2(") | lit("abs(") | lit("min(") | lit("max(") | lit("floor(") | lit("ceil(") | lit("round(") |
        lit("radtodeg(") | lit("degtorad(") | lit("log(") | lit("exp(") | lit("sqrt(") | lit("hypot(") | lit("if(");
    linkName = (alpha >> *(alnum | char_('_'))) - keyword;
    formulaName = (alpha >> *(alnum | char_('_'))) - keyword;
    factor = scalarItem | function1 | function2 | ifCondition |
        char_('(') [bp::bind(&Translator::startScalarParenthesesNode, &translator)] >> scalarRhs >>
        char_ (')') [bp::bind(&Translator::finishScalarParenthesesNode, &translator)];
    term = factor >>
        *((char_('*') [bp::bind(&Translator::buildScalarMultiplicationNode, &translator)] >>
        factor [bp::bind(&Translator::makeCurrentRHS, &translator, _pass)]) |
        (char_('/') [bp::bind(&Translator::buildScalarDivisionNode, &translator)] >>
        factor [bp::bind(&Translator::makeCurrentRHS, &translator, _pass)]));
    vector = char_('[') >> scalarRhs >> char_(',') >> scalarRhs >> char_(',') >> scalarRhs
      >> char_(']') [bp::bind(&Translator::buildVectorConstantNode, &translator)];
    vectorItem = vector | linkName [bp::bind(&Translator::buildLinkNode, &translator, bs::_1, _pass)];
    vectorRhs = vectorItem;
    quat = char_('[')
      >> vectorItem >> char_(',') >> scalarRhs
      >> char_(']') [bp::bind(&Translator::buildQuatConstantNode, &translator)];
    quatItem = quat | linkName [bp::bind(&Translator::buildLinkNode, &translator, bs::_1, _pass)];
    quatRhs = quatItem;
      
      BOOST_SPIRIT_DEBUG_NODE(expression);
      BOOST_SPIRIT_DEBUG_NODE(scalarRhs);
      BOOST_SPIRIT_DEBUG_NODE(scalarItem);
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
      BOOST_SPIRIT_DEBUG_NODE(vector);
      BOOST_SPIRIT_DEBUG_NODE(vectorItem);
      BOOST_SPIRIT_DEBUG_NODE(vectorRhs);
      BOOST_SPIRIT_DEBUG_NODE(quat);
      BOOST_SPIRIT_DEBUG_NODE(quatItem);
      BOOST_SPIRIT_DEBUG_NODE(quatRhs);
    }
    rule<Iterator, space_type> expression;
    rule<Iterator, space_type> scalarRhs;
    rule<Iterator, space_type> scalarItem;
    rule<Iterator, space_type> term;
    rule<Iterator, space_type> factor;
    rule<Iterator, space_type> function1;
    rule<Iterator, space_type> function2;
    rule<Iterator, space_type> conditionalOperator;
    rule<Iterator, space_type> condition;
    rule<Iterator, space_type> ifCondition;
    rule<Iterator, space_type> vector;
    rule<Iterator, space_type> vectorItem;
    rule<Iterator, space_type> vectorRhs;
    rule<Iterator, space_type> quat;
    rule<Iterator, space_type> quatItem;
    rule<Iterator, space_type> quatRhs;
    rule<Iterator, space_type, std::string()> keyword;
    rule<Iterator, space_type, std::string()> linkName;
    rule<Iterator, space_type, std::string()> formulaName;
    
    Translator &translator;
  };
}

#endif // EXPR_GRAMMER_H

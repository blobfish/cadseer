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

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_utree.hpp>
#include <boost/spirit/include/phoenix_function.hpp>
#include <boost/spirit/include/phoenix_bind.hpp>

#include <expressions/stringtranslatorstow.h>

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
  /*! @brief Describes boost::spirit parsing grammar
  * 
  * @todo Explain acceptable string format.
  */
  template <typename Iterator>
  struct ExpressionGrammar : grammar<Iterator, space_type>
  {
    ExpressionGrammar(StringTranslatorStow &translatorIn) : ExpressionGrammar::base_type(expression), translator(translatorIn)
    {
      using boost::spirit::qi::alpha;
      using boost::spirit::qi::alnum;
      using boost::spirit::qi::punct;
      using boost::spirit::qi::double_;
      using boost::spirit::qi::char_;
      using boost::spirit::qi::lit;
      
    expression = formulaName [boost::phoenix::bind(&StringTranslatorStow::buildFormulaNode, &translator, boost::spirit::_1, _pass)] >>
        '=' >> rhs [boost::phoenix::bind(&StringTranslatorStow::finish, &translator)];
    rhs = term >>
        *((char_('+') [boost::phoenix::bind(&StringTranslatorStow::buildAdditionNode, &translator)] >>
        term [boost::phoenix::bind(&StringTranslatorStow::makeCurrentRHS, &translator)]) |
        (char_('-') [boost::phoenix::bind(&StringTranslatorStow::buildSubractionNode, &translator)] >>
        term [boost::phoenix::bind(&StringTranslatorStow::makeCurrentRHS, &translator)]));
    item = double_ [boost::phoenix::bind(&StringTranslatorStow::buildConstantNode, &translator, boost::spirit::_1)] |
        linkName [boost::phoenix::bind(&StringTranslatorStow::buildLinkNode, &translator, boost::spirit::_1, _pass)];
    function1 = (lit("sin(") [boost::phoenix::bind(&StringTranslatorStow::buildSinNode, &translator)] |
        lit("cos(") [boost::phoenix::bind(&StringTranslatorStow::buildCosNode, &translator)] |
        lit("tan(") [boost::phoenix::bind(&StringTranslatorStow::buildTanNode, &translator)] |
        lit("asin(") [boost::phoenix::bind(&StringTranslatorStow::buildAsinNode, &translator)] |
        lit("acos(") [boost::phoenix::bind(&StringTranslatorStow::buildAcosNode, &translator)] |
        lit("atan(") [boost::phoenix::bind(&StringTranslatorStow::buildAtanNode, &translator)] |
        lit("radtodeg(") [boost::phoenix::bind(&StringTranslatorStow::buildRadToDegNode, &translator)] |
        lit("degtorad(") [boost::phoenix::bind(&StringTranslatorStow::buildDegToRadNode, &translator)] |
        lit("log(") [boost::phoenix::bind(&StringTranslatorStow::buildLogNode, &translator)] |
        lit("exp(") [boost::phoenix::bind(&StringTranslatorStow::buildExpNode, &translator)] |
        lit("sqrt(") [boost::phoenix::bind(&StringTranslatorStow::buildSqrtNode, &translator)] |
        lit("abs(") [boost::phoenix::bind(&StringTranslatorStow::buildAbsNode, &translator)]) >> rhs >>
        char_(')') [boost::phoenix::bind(&StringTranslatorStow::finishFunction1, &translator)];
    function2 = (lit("pow(") [boost::phoenix::bind(&StringTranslatorStow::buildPowNode, &translator)] |
        lit("atan2(") [boost::phoenix::bind(&StringTranslatorStow::buildAtan2Node, &translator)] |
        lit("min(") [boost::phoenix::bind(&StringTranslatorStow::buildMinNode, &translator)] |
        lit("max(") [boost::phoenix::bind(&StringTranslatorStow::buildMaxNode, &translator)] |
        lit("floor(") [boost::phoenix::bind(&StringTranslatorStow::buildFloorNode, &translator)] |
        lit("ceil(") [boost::phoenix::bind(&StringTranslatorStow::buildCeilNode, &translator)] |
        lit("round(") [boost::phoenix::bind(&StringTranslatorStow::buildRoundNode, &translator)] | 
        lit("hypot(") [boost::phoenix::bind(&StringTranslatorStow::buildHypotNode, &translator)]) >> rhs >>
        char_(',') [boost::phoenix::bind(&StringTranslatorStow::setParameter1, &translator)] >> rhs >>
        char_(')') [boost::phoenix::bind(&StringTranslatorStow::setParameter2, &translator)];
    //when one parser is a subset of another (i.e. '>' and '>=') put the larger first, so it fails before trying the smaller.
    conditionalOperator = lit(">=") [boost::phoenix::bind(&StringTranslatorStow::setConditionGreaterThanEqual, &translator)] |
        lit("<=") [boost::phoenix::bind(&StringTranslatorStow::setConditionLessThanEqual, &translator)] |
        lit(">") [boost::phoenix::bind(&StringTranslatorStow::setConditionGreaterThan, &translator)] |
        lit("<") [boost::phoenix::bind(&StringTranslatorStow::setConditionLessThan, &translator)] |
        lit("==") [boost::phoenix::bind(&StringTranslatorStow::setConditionEqual, &translator)] |
        lit("!=") [boost::phoenix::bind(&StringTranslatorStow::setConditionNotEqual, &translator)];
    condition = rhs [boost::phoenix::bind(&StringTranslatorStow::setConditionLhs, &translator)] >> conditionalOperator >>
        rhs [boost::phoenix::bind(&StringTranslatorStow::setConditionRhs, &translator)];
    ifCondition = lit("if(") [boost::phoenix::bind(&StringTranslatorStow::buildConditionalNode, &translator)] >>
        condition >> lit(")") >>
        lit("then(") >> rhs [boost::phoenix::bind(&StringTranslatorStow::setConditionThen, &translator)] >> char_(')') >>
        lit("else(") >> rhs [boost::phoenix::bind(&StringTranslatorStow::setConditionElse, &translator)] >> char_(')');
    keyword = lit("sin(") | lit("cos(") | lit("tan(") | lit("asin(") | lit("acos(") | lit("atan(") | lit("pow(") |
        lit("atan2(") | lit("abs(") | lit("min(") | lit("max(") | lit("floor(") | lit("ceil(") | lit("round(") |
        lit("radtodeg(") | lit("degtorad(") | lit("log(") | lit("exp(") | lit("sqrt(") | lit("hypot(") | lit("if(");
    linkName = (alpha >> *(alnum | char_('_'))) - keyword;
    formulaName = (alpha >> *(alnum | char_('_'))) - keyword;
    factor = item | function1 | function2 | ifCondition |
        char_('(') [boost::phoenix::bind(&StringTranslatorStow::startParenthesesNode, &translator)] >> rhs >>
        char_ (')') [boost::phoenix::bind(&StringTranslatorStow::finishParenthesesNode, &translator)];
    term = factor >>
        *((char_('*') [boost::phoenix::bind(&StringTranslatorStow::buildMultiplicationNode, &translator)] >>
        factor [boost::phoenix::bind(&StringTranslatorStow::makeCurrentRHS, &translator)]) |
        (char_('/') [boost::phoenix::bind(&StringTranslatorStow::buildDivisionNode, &translator)] >>
        factor [boost::phoenix::bind(&StringTranslatorStow::makeCurrentRHS, &translator)]));
      
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
    
    StringTranslatorStow &translator;
  };
}

#endif // EXPR_GRAMMER_H

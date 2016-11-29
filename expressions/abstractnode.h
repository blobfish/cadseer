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

#ifndef EXPRESSIONNODES_H
#define EXPRESSIONNODES_H

#include <boost/uuid/uuid.hpp>

#include <expressions/expressionedgeproperty.h>

namespace expr{

//! @brief Types for graph vertices
namespace NodeType
{
  //! @brief Types for graph vertices
  enum Type
  {
    None,
    Formula,
    Constant,
    Addition,
    Subtraction,
    Multiplication,
    Division,
    Parentheses,
    Sin,
    Cos,
    Tan,
    Asin,
    Acos,
    Atan,
    Atan2,
    Pow,
    Abs,
    Min,
    Max,
    Floor,
    Ceil,
    Round,
    RadToDeg,
    DegToRad,
    Log,
    Exp,
    Sqrt,
    Hypot,
    Conditional
  };
}

/*! @brief Abstract. 
 * 
 * Base node for all nodes contained in a graph vertex.
 */
class AbstractNode
{
public:
  AbstractNode();
  virtual ~AbstractNode(){};
  //@{
  //! Set dirty state. @see dirtyTest
  void setDirty(){dirtyTest = true;}
  void setClean(){dirtyTest = false;}
  //@}
  //@{
  //! Inquire dirty state. @see dirtyTest
  bool isDirty(){return dirtyTest;}
  bool isClean(){return !dirtyTest;}
  //@}
  //! Returns the value. @see value
  double getValue();
  //! get this nodes type. @see NodeType::Type
  virtual NodeType::Type getType() = 0;
  //! get this nodes types name.
  virtual std::string className() = 0;
  /*! @brief calculate the value of this node.
   * 
   * Property map is constructed in the computation of the graph.
   * Passed in and used to evaluate the node.
   * This separates the graph definition from the nodes. @see EdgePropertiesMap
   */
  virtual void calculate(const EdgePropertiesMap &propertyMap) = 0;
  
protected:
  //! Signifies whether the node needs to be calculated.
  bool dirtyTest;
  //! Value of this node. @see calculate
  double value;
};

/*! @brief Constant number. No parameters. */
class ConstantNode : public AbstractNode
{
public:
  ConstantNode();
  virtual ~ConstantNode() {}
  virtual NodeType::Type getType() {return NodeType::Constant;}
  virtual std::string className() {return "Constant";}
  virtual void calculate(const EdgePropertiesMap &propertyMap) {setClean();}
  
  //! Set the value of this constant node.
  void setValue(const double &valueIn){value = valueIn;}
};

/*! @brief Formula. 
 * 
 * Root node for all named formulas.
 */
class FormulaNode : public AbstractNode
{
public:
  FormulaNode();
  virtual ~FormulaNode() {}
  virtual NodeType::Type getType() {return NodeType::Formula;}
  virtual std::string className() {return "Formula";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
  
  //! Returns the id. @see id
  boost::uuids::uuid getId(){return id;}
  
  //! Name of the formula
  std::string name;
protected:
  //! Unique identifier.
  boost::uuids::uuid id;
};

/*! @brief Addition. 2 parameters. */
class AdditionNode : public AbstractNode
{
public:
  AdditionNode();
  virtual ~AdditionNode() {}
  virtual NodeType::Type getType() {return NodeType::Addition;}
  virtual std::string className() {return "Addition";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
};

/*! @brief Subtraction. 2 parameters. */
class SubtractionNode : public AbstractNode
{
public:
  SubtractionNode();
  virtual ~SubtractionNode() {}
  virtual NodeType::Type getType() {return NodeType::Subtraction;}
  virtual std::string className() {return "Subtraction";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
};

/*! @brief Multiplication. 2 parameters. */
class MultiplicationNode : public AbstractNode
{
public:
  MultiplicationNode();
  virtual ~MultiplicationNode() {}
  virtual NodeType::Type getType() {return NodeType::Multiplication;}
  virtual std::string className() {return "Multiplication";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
};

/*! @brief Division.  2 parameters.*/
class DivisionNode : public AbstractNode
{
public:
  DivisionNode();
  virtual ~DivisionNode() {}
  virtual NodeType::Type getType() {return NodeType::Division;}
  virtual std::string className() {return "Division";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
};

/*! @brief Parenthesis. 1 parameter. */
class ParenthesesNode : public AbstractNode
{
public:
  ParenthesesNode();
  virtual ~ParenthesesNode() {}
  virtual NodeType::Type getType() {return NodeType::Parentheses;}
  virtual std::string className() {return "Parentheses";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
};

/*! @brief Sin. 1 parameter. */
class SinNode : public AbstractNode
{
public:
  SinNode();
  virtual ~SinNode() {}
  virtual NodeType::Type getType() {return NodeType::Sin;}
  virtual std::string className() {return "Sin";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
};

/*! @brief Cosine. 1 parameter. */
class CosNode : public AbstractNode
{
public:
  CosNode();
  virtual ~CosNode() {}
  virtual NodeType::Type getType() {return NodeType::Cos;}
  virtual std::string className() {return "Cos";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
};

/*! @brief Tangent. 1 parameter. */
class TanNode : public AbstractNode
{
public:
  TanNode();
  virtual ~TanNode() {}
  virtual NodeType::Type getType() {return NodeType::Tan;}
  virtual std::string className() {return "Tan";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
};

/*! @brief Arc Sin. 1 parameter. */
class AsinNode : public AbstractNode
{
public:
  AsinNode();
  virtual ~AsinNode() {}
  virtual NodeType::Type getType() {return NodeType::Asin;}
  virtual std::string className() {return "Asin";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
};

/*! @brief Arc Cosine. 1 parameter.*/
class AcosNode : public AbstractNode
{
public:
  AcosNode();
  virtual ~AcosNode() {}
  virtual NodeType::Type getType() {return NodeType::Acos;}
  virtual std::string className() {return "Acos";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
};

/*! @brief Arc Tangent. 1 parameter. */
class AtanNode : public AbstractNode
{
public:
  AtanNode();
  virtual ~AtanNode() {}
  virtual NodeType::Type getType() {return NodeType::Atan;}
  virtual std::string className() {return "Atan";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
};

/*! @brief Arc Tangent function with 2 parameters.
 * 
 * first parameter is the y component and x is the second. Like std::atan2 function.
 */
class Atan2Node : public AbstractNode
{
public:
  Atan2Node();
  virtual ~Atan2Node() {}
  virtual NodeType::Type getType() {return NodeType::Atan2;}
  virtual std::string className() {return "Atan2";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
};

/*! @brief Power. 2 parameters with one base and one exponent. */
class PowNode : public AbstractNode
{
public:
  PowNode();
  virtual ~PowNode() {}
  virtual NodeType::Type getType() {return NodeType::Pow;}
  virtual std::string className() {return "Pow";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
};

/*! @brief Absolute Value. 1 parameter*/
class AbsNode : public AbstractNode
{
public:
  AbsNode();
  virtual ~AbsNode() {}
  virtual NodeType::Type getType() {return NodeType::Abs;}
  virtual std::string className() {return "Abs";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
};

/*! @brief Minimum Value. 2 parameters */
class MinNode : public AbstractNode
{
public:
  MinNode();
  virtual ~MinNode() {}
  virtual NodeType::Type getType() {return NodeType::Min;}
  virtual std::string className() {return "Min";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
};

/*! @brief Maximum Value. 2 parameters */
class MaxNode : public AbstractNode
{
public:
  MaxNode();
  virtual ~MaxNode() {}
  virtual NodeType::Type getType() {return NodeType::Max;}
  virtual std::string className() {return "Max";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
};

/*! @brief Round Down. 2 parameters */
class FloorNode : public AbstractNode
{
public:
  FloorNode();
  virtual ~FloorNode() {}
  virtual NodeType::Type getType() {return NodeType::Floor;}
  virtual std::string className() {return "Floor";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
};

/*! @brief Round Up. 2 parameters */
class CeilNode : public AbstractNode
{
public:
  CeilNode();
  virtual ~CeilNode() {}
  virtual NodeType::Type getType() {return NodeType::Ceil;}
  virtual std::string className() {return "Ceil";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
};

/*! @brief Round. 2 parameters */
class RoundNode : public AbstractNode
{
public:
  RoundNode();
  virtual ~RoundNode() {}
  virtual NodeType::Type getType() {return NodeType::Round;}
  virtual std::string className() {return "Round";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
};

/*! @brief Convert radians to degrees. 1 parameter */
class RadToDegNode : public AbstractNode
{
public:
  RadToDegNode();
  virtual ~RadToDegNode() {}
  virtual NodeType::Type getType() {return NodeType::RadToDeg;}
  virtual std::string className() {return "RadToDeg";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
};

/*! @brief Convert degrees to radians. 1 parameter */
class DegToRadNode : public AbstractNode
{
public:
  DegToRadNode();
  virtual ~DegToRadNode() {}
  virtual NodeType::Type getType() {return NodeType::DegToRad;}
  virtual std::string className() {return "DegToRad";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
};

/*! @brief Logarithm. 1 parameter */
class LogNode : public AbstractNode
{
public:
  LogNode();
  virtual ~LogNode() {}
  virtual NodeType::Type getType() {return NodeType::Log;}
  virtual std::string className() {return "Log";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
};

/*! @brief Exponent. 1 parameter */
class ExpNode : public AbstractNode
{
public:
  ExpNode();
  virtual ~ExpNode() {}
  virtual NodeType::Type getType() {return NodeType::Exp;}
  virtual std::string className() {return "Exp";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
};

/*! @brief Square root. 1 parameter */
class SqrtNode : public AbstractNode
{
public:
  SqrtNode();
  virtual ~SqrtNode() {}
  virtual NodeType::Type getType() {return NodeType::Sqrt;}
  virtual std::string className() {return "Sqrt";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
};

/*! @brief Hypotenuse. 2 parameters */
class HypotNode : public AbstractNode
{
public:
  HypotNode();
  virtual ~HypotNode() {}
  virtual NodeType::Type getType() {return NodeType::Hypot;}
  virtual std::string className() {return "Hypot";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
};

/*! @brief If, Then, Else. 4 parameters */
class ConditionalNode : public AbstractNode
{
public:
  enum Type{None, GreaterThan, LessThan, GreaterThanEqual, LessThanEqual, Equal, NotEqual};
  ConditionalNode();
  virtual ~ConditionalNode() {}
  virtual NodeType::Type getType() {return NodeType::Conditional;}
  virtual std::string className() {return "Conditional";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
  Type type;
};

}
#endif // EXPRESSIONNODES_H

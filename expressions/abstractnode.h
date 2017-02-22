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
#include <expressions/value.h>

namespace expr{

//! @brief Types for graph vertices
enum class NodeType
{
  None,
  Formula,
  ScalarConstant,
  ScalarAddition,
  ScalarSubtraction,
  ScalarMultiplication,
  ScalarDivision,
  ScalarParentheses,
  ScalarSin,
  ScalarCos,
  ScalarTan,
  ScalarAsin,
  ScalarAcos,
  ScalarAtan,
  ScalarAtan2,
  ScalarPow,
  ScalarAbs,
  ScalarMin,
  ScalarMax,
  ScalarFloor,
  ScalarCeil,
  ScalarRound,
  ScalarRadToDeg,
  ScalarDegToRad,
  ScalarLog,
  ScalarExp,
  ScalarSqrt,
  ScalarHypot,
  ScalarConditional
};

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
  bool isDirty() const{return dirtyTest;}
  bool isClean() const{return !dirtyTest;}
  //@}
  //! Returns the value. @see value
  double getValue() const;
  //! get this nodes type. @see NodeType
  virtual NodeType getType() const = 0;
  //! get this nodes types name.
  virtual std::string className() const = 0;
  /*! @brief calculate the value of this node.
   * 
   * Property map is constructed in the computation of the graph.
   * Passed in and used to evaluate the node.
   * This separates the graph definition from the nodes. @see EdgePropertiesMap
   */
  virtual void calculate(const EdgePropertiesMap &propertyMap) = 0;
  /*! @brief get the output type of node.
   * 
   * we are now supporting some basic vector math variable and
   * we need to verify that the input types are of the expected
   * type.
   */
  virtual ValueType getOutputType() const = 0;
  
protected:
  //! Signifies whether the node needs to be calculated.
  bool dirtyTest;
  //! Value of this node. @see calculate
  Value value;
};

/*! @brief Constant number. No parameters. */
class ScalarConstantNode : public AbstractNode
{
public:
  ScalarConstantNode();
  virtual ~ScalarConstantNode() {}
  virtual NodeType getType() const override {return NodeType::ScalarConstant;}
  virtual std::string className() const override {return "ScalarConstant";}
  virtual void calculate(const EdgePropertiesMap &propertyMap) {setClean();}
  virtual ValueType getOutputType() const override {return ValueType::Scalar;}
  
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
  virtual NodeType getType() const override {return NodeType::Formula;}
  virtual std::string className() const override {return "Formula";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
  virtual ValueType getOutputType() const override {return ValueType::Variant;}
  
  //! Returns the id. @see id
  boost::uuids::uuid getId(){return id;}
  //! Set the id of the formula. SERIALIZE IN ONLY?
  void setId(const boost::uuids::uuid &idIn){id = idIn;}
  
  //! Name of the formula
  std::string name;
protected:
  //! Unique identifier.
  boost::uuids::uuid id;
};

/*! @brief Addition. 2 parameters. */
class ScalarAdditionNode : public AbstractNode
{
public:
  ScalarAdditionNode();
  virtual ~ScalarAdditionNode() {}
  virtual NodeType getType() const override {return NodeType::ScalarAddition;}
  virtual std::string className() const override {return "ScalarAddition";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
  virtual ValueType getOutputType() const override {return ValueType::Scalar;}
};

/*! @brief Subtraction. 2 parameters. */
class ScalarSubtractionNode : public AbstractNode
{
public:
  ScalarSubtractionNode();
  virtual ~ScalarSubtractionNode() {}
  virtual NodeType getType() const override {return NodeType::ScalarSubtraction;}
  virtual std::string className() const override {return "ScalarSubtraction";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
  virtual ValueType getOutputType() const override {return ValueType::Scalar;}
};

/*! @brief Multiplication. 2 parameters. */
class ScalarMultiplicationNode : public AbstractNode
{
public:
  ScalarMultiplicationNode();
  virtual ~ScalarMultiplicationNode() {}
  virtual NodeType getType() const override {return NodeType::ScalarMultiplication;}
  virtual std::string className() const override {return "ScalarMultiplication";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
  virtual ValueType getOutputType() const override {return ValueType::Scalar;}
};

/*! @brief Division.  2 parameters.*/
class ScalarDivisionNode : public AbstractNode
{
public:
  ScalarDivisionNode();
  virtual ~ScalarDivisionNode() {}
  virtual NodeType getType() const override {return NodeType::ScalarDivision;}
  virtual std::string className() const override {return "ScalarDivision";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
  virtual ValueType getOutputType() const override {return ValueType::Scalar;}
};

/*! @brief Parenthesis. 1 parameter. */
class ScalarParenthesesNode : public AbstractNode
{
public:
  ScalarParenthesesNode();
  virtual ~ScalarParenthesesNode() {}
  virtual NodeType getType() const override {return NodeType::ScalarParentheses;}
  virtual std::string className() const override {return "ScalarParentheses";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
  virtual ValueType getOutputType() const override {return ValueType::Scalar;}
};

/*! @brief Sin. 1 parameter. */
class ScalarSinNode : public AbstractNode
{
public:
  ScalarSinNode();
  virtual ~ScalarSinNode() {}
  virtual NodeType getType() const override {return NodeType::ScalarSin;}
  virtual std::string className() const override {return "ScalarSin";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
  virtual ValueType getOutputType() const override {return ValueType::Scalar;}
};

/*! @brief Cosine. 1 parameter. */
class ScalarCosNode : public AbstractNode
{
public:
  ScalarCosNode();
  virtual ~ScalarCosNode() {}
  virtual NodeType getType() const override {return NodeType::ScalarCos;}
  virtual std::string className() const override {return "ScalarCos";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
  virtual ValueType getOutputType() const override {return ValueType::Scalar;}
};

/*! @brief Tangent. 1 parameter. */
class ScalarTanNode : public AbstractNode
{
public:
  ScalarTanNode();
  virtual ~ScalarTanNode() {}
  virtual NodeType getType() const override {return NodeType::ScalarTan;}
  virtual std::string className() const override {return "ScalarTan";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
  virtual ValueType getOutputType() const override {return ValueType::Scalar;}
};

/*! @brief Arc Sin. 1 parameter. */
class ScalarAsinNode : public AbstractNode
{
public:
  ScalarAsinNode();
  virtual ~ScalarAsinNode() {}
  virtual NodeType getType() const override {return NodeType::ScalarAsin;}
  virtual std::string className() const override {return "ScalarAsin";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
  virtual ValueType getOutputType() const override {return ValueType::Scalar;}
};

/*! @brief Arc Cosine. 1 parameter.*/
class ScalarAcosNode : public AbstractNode
{
public:
  ScalarAcosNode();
  virtual ~ScalarAcosNode() {}
  virtual NodeType getType() const override {return NodeType::ScalarAcos;}
  virtual std::string className() const override {return "ScalarAcos";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
  virtual ValueType getOutputType() const override {return ValueType::Scalar;}
};

/*! @brief Arc Tangent. 1 parameter. */
class ScalarAtanNode : public AbstractNode
{
public:
  ScalarAtanNode();
  virtual ~ScalarAtanNode() {}
  virtual NodeType getType() const override {return NodeType::ScalarAtan;}
  virtual std::string className() const override {return "ScalarAtan";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
  virtual ValueType getOutputType() const override {return ValueType::Scalar;}
};

/*! @brief Arc Tangent function with 2 parameters.
 * 
 * first parameter is the y component and x is the second. Like std::atan2 function.
 */
class ScalarAtan2Node : public AbstractNode
{
public:
  ScalarAtan2Node();
  virtual ~ScalarAtan2Node() {}
  virtual NodeType getType() const override {return NodeType::ScalarAtan2;}
  virtual std::string className() const override {return "ScalarAtan2";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
  virtual ValueType getOutputType() const override {return ValueType::Scalar;}
};

/*! @brief Power. 2 parameters with one base and one exponent. */
class ScalarPowNode : public AbstractNode
{
public:
  ScalarPowNode();
  virtual ~ScalarPowNode() {}
  virtual NodeType getType() const override {return NodeType::ScalarPow;}
  virtual std::string className() const override {return "ScalarPow";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
  virtual ValueType getOutputType() const override {return ValueType::Scalar;}
};

/*! @brief Absolute Value. 1 parameter*/
class ScalarAbsNode : public AbstractNode
{
public:
  ScalarAbsNode();
  virtual ~ScalarAbsNode() {}
  virtual NodeType getType() const override {return NodeType::ScalarAbs;}
  virtual std::string className() const override {return "ScalarAbs";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
  virtual ValueType getOutputType() const override {return ValueType::Scalar;}
};

/*! @brief Minimum Value. 2 parameters */
class ScalarMinNode : public AbstractNode
{
public:
  ScalarMinNode();
  virtual ~ScalarMinNode() {}
  virtual NodeType getType() const override {return NodeType::ScalarMin;}
  virtual std::string className() const override {return "ScalarMin";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
  virtual ValueType getOutputType() const override {return ValueType::Scalar;}
};

/*! @brief Maximum Value. 2 parameters */
class ScalarMaxNode : public AbstractNode
{
public:
  ScalarMaxNode();
  virtual ~ScalarMaxNode() {}
  virtual NodeType getType() const override {return NodeType::ScalarMax;}
  virtual std::string className() const override {return "ScalarMax";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
  virtual ValueType getOutputType() const override {return ValueType::Scalar;}
};

/*! @brief Round Down. 2 parameters */
class ScalarFloorNode : public AbstractNode
{
public:
  ScalarFloorNode();
  virtual ~ScalarFloorNode() {}
  virtual NodeType getType() const override {return NodeType::ScalarFloor;}
  virtual std::string className() const override {return "ScalarFloor";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
  virtual ValueType getOutputType() const override {return ValueType::Scalar;}
};

/*! @brief Round Up. 2 parameters */
class ScalarCeilNode : public AbstractNode
{
public:
  ScalarCeilNode();
  virtual ~ScalarCeilNode() {}
  virtual NodeType getType() const override {return NodeType::ScalarCeil;}
  virtual std::string className() const override {return "ScalarCeil";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
  virtual ValueType getOutputType() const override {return ValueType::Scalar;}
};

/*! @brief Round. 2 parameters */
class ScalarRoundNode : public AbstractNode
{
public:
  ScalarRoundNode();
  virtual ~ScalarRoundNode() {}
  virtual NodeType getType() const override {return NodeType::ScalarRound;}
  virtual std::string className() const override {return "ScalarRound";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
  virtual ValueType getOutputType() const override {return ValueType::Scalar;}
};

/*! @brief Convert radians to degrees. 1 parameter */
class ScalarRadToDegNode : public AbstractNode
{
public:
  ScalarRadToDegNode();
  virtual ~ScalarRadToDegNode() {}
  virtual NodeType getType() const override {return NodeType::ScalarRadToDeg;}
  virtual std::string className() const override {return "ScalarRadToDeg";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
  virtual ValueType getOutputType() const override {return ValueType::Scalar;}
};

/*! @brief Convert degrees to radians. 1 parameter */
class ScalarDegToRadNode : public AbstractNode
{
public:
  ScalarDegToRadNode();
  virtual ~ScalarDegToRadNode() {}
  virtual NodeType getType() const override {return NodeType::ScalarDegToRad;}
  virtual std::string className() const override {return "ScalarDegToRad";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
  virtual ValueType getOutputType() const override {return ValueType::Scalar;}
};

/*! @brief Logarithm. 1 parameter */
class ScalarLogNode : public AbstractNode
{
public:
  ScalarLogNode();
  virtual ~ScalarLogNode() {}
  virtual NodeType getType() const override {return NodeType::ScalarLog;}
  virtual std::string className() const override {return "ScalarLog";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
  virtual ValueType getOutputType() const override {return ValueType::Scalar;}
};

/*! @brief Exponent. 1 parameter */
class ScalarExpNode : public AbstractNode
{
public:
  ScalarExpNode();
  virtual ~ScalarExpNode() {}
  virtual NodeType getType() const override {return NodeType::ScalarExp;}
  virtual std::string className() const override {return "ScalarExp";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
  virtual ValueType getOutputType() const override {return ValueType::Scalar;}
};

/*! @brief Square root. 1 parameter */
class ScalarSqrtNode : public AbstractNode
{
public:
  ScalarSqrtNode();
  virtual ~ScalarSqrtNode() {}
  virtual NodeType getType() const override {return NodeType::ScalarSqrt;}
  virtual std::string className() const override {return "ScalarSqrt";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
  virtual ValueType getOutputType() const override {return ValueType::Scalar;}
};

/*! @brief Hypotenuse. 2 parameters */
class ScalarHypotNode : public AbstractNode
{
public:
  ScalarHypotNode();
  virtual ~ScalarHypotNode() {}
  virtual NodeType getType() const override {return NodeType::ScalarHypot;}
  virtual std::string className() const override {return "ScalarHypot";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
  virtual ValueType getOutputType() const override {return ValueType::Scalar;}
};

/*! @brief If, Then, Else. 4 parameters */
class ScalarConditionalNode : public AbstractNode
{
public:
  enum Type{None, GreaterThan, LessThan, GreaterThanEqual, LessThanEqual, Equal, NotEqual};
  ScalarConditionalNode();
  virtual ~ScalarConditionalNode() {}
  virtual NodeType getType() const override {return NodeType::ScalarConditional;}
  virtual std::string className() const override {return "ScalarConditional";}
  virtual void calculate(const EdgePropertiesMap &propertyMap);
  virtual ValueType getOutputType() const override {return ValueType::Scalar;}
  Type type;
};

}
#endif // EXPRESSIONNODES_H

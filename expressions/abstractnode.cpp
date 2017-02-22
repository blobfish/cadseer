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
#include <cmath>

#include <boost/math/constants/constants.hpp>

#include <tools/idtools.h>
#include <expressions/abstractnode.h>

using namespace expr;

AbstractNode::AbstractNode() : dirtyTest(true), value(1.0)
{

}

double AbstractNode::getValue() const
{
  assert(this->isClean());
  return boost::get<double>(value);
}

ScalarConstantNode::ScalarConstantNode() : AbstractNode()
{

}

FormulaNode::FormulaNode() : AbstractNode(), name("no name"), id(gu::createRandomId())
{

}

void FormulaNode::calculate(const EdgePropertiesMap &propertyMap)
{
  assert(propertyMap.size() == 1);
  assert(propertyMap.count(EdgeProperty::None) == 1);
  value = propertyMap.at(EdgeProperty::None)->getValue();
  this->setClean();
}

ScalarAdditionNode::ScalarAdditionNode() : AbstractNode()
{

}

void ScalarAdditionNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 2);
  assert(propertyMap.count(EdgeProperty::Lhs) == 1);
  assert(propertyMap.count(EdgeProperty::Rhs) == 1);
  value = propertyMap.at(EdgeProperty::Lhs)->getValue() + propertyMap.at(EdgeProperty::Rhs)->getValue();
  this->setClean();
}

ScalarSubtractionNode::ScalarSubtractionNode() : AbstractNode()
{

}

void ScalarSubtractionNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 2);
  assert(propertyMap.count(EdgeProperty::Lhs) == 1);
  assert(propertyMap.count(EdgeProperty::Rhs) == 1);
  value = propertyMap.at(EdgeProperty::Lhs)->getValue() - propertyMap.at(EdgeProperty::Rhs)->getValue();
  this->setClean();
}

ScalarMultiplicationNode::ScalarMultiplicationNode() : AbstractNode()
{

}

void ScalarMultiplicationNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 2);
  assert(propertyMap.count(EdgeProperty::Lhs) == 1);
  assert(propertyMap.count(EdgeProperty::Rhs) == 1);
  value = propertyMap.at(EdgeProperty::Lhs)->getValue() * propertyMap.at(EdgeProperty::Rhs)->getValue();
  this->setClean();
}

ScalarDivisionNode::ScalarDivisionNode() : AbstractNode()
{

}

void ScalarDivisionNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 2);
  assert(propertyMap.count(EdgeProperty::Lhs) == 1);
  assert(propertyMap.count(EdgeProperty::Rhs) == 1);
  value = propertyMap.at(EdgeProperty::Lhs)->getValue() / propertyMap.at(EdgeProperty::Rhs)->getValue();
  this->setClean();
}

ScalarParenthesesNode::ScalarParenthesesNode() : AbstractNode()
{

}

void ScalarParenthesesNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 1);
  assert(propertyMap.count(EdgeProperty::None) == 1);
  value = propertyMap.at(EdgeProperty::None)->getValue();
  this->setClean();
}

ScalarSinNode::ScalarSinNode() : AbstractNode()
{

}

void ScalarSinNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 1);
  assert(propertyMap.count(EdgeProperty::None) == 1);
  double childValue = propertyMap.at(EdgeProperty::None)->getValue();
  value = std::sin(childValue);
  this->setClean();
}

ScalarCosNode::ScalarCosNode() : AbstractNode()
{

}

void ScalarCosNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 1);
  assert(propertyMap.count(EdgeProperty::None) == 1);
  double childValue = propertyMap.at(EdgeProperty::None)->getValue();
  value = std::cos(childValue);
  this->setClean();
}

ScalarTanNode::ScalarTanNode() : AbstractNode()
{

}

void ScalarTanNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 1);
  assert(propertyMap.count(EdgeProperty::None) == 1);
  double childValue = propertyMap.at(EdgeProperty::None)->getValue();
  value = std::tan(childValue);
  this->setClean();
}

ScalarAsinNode::ScalarAsinNode() : AbstractNode()
{

}

void ScalarAsinNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 1);
  assert(propertyMap.count(EdgeProperty::None) == 1);
  double childValue = propertyMap.at(EdgeProperty::None)->getValue();
  value = std::asin(childValue);
  this->setClean();
}

ScalarAcosNode::ScalarAcosNode() : AbstractNode()
{

}

void ScalarAcosNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 1);
  assert(propertyMap.count(EdgeProperty::None) == 1);
  double childValue = propertyMap.at(EdgeProperty::None)->getValue();
  value = std::acos(childValue);
  this->setClean();
}

ScalarAtanNode::ScalarAtanNode() : AbstractNode()
{

}

void ScalarAtanNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 1);
  assert(propertyMap.count(EdgeProperty::None) == 1);
  double childValue = propertyMap.at(EdgeProperty::None)->getValue();
  value = std::atan(childValue);
  this->setClean();
}

ScalarAtan2Node::ScalarAtan2Node() : AbstractNode()
{

}

void ScalarAtan2Node::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 2);
  assert(propertyMap.count(EdgeProperty::Parameter1) == 1);
  assert(propertyMap.count(EdgeProperty::Parameter2) == 1);
  // y is first to match standard function.
  double y = propertyMap.at(EdgeProperty::Parameter1)->getValue();
  double x = propertyMap.at(EdgeProperty::Parameter2)->getValue();
  value = std::atan2(y, x);
  this->setClean();
}

ScalarPowNode::ScalarPowNode()
{

}

void ScalarPowNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 2);
  assert(propertyMap.count(EdgeProperty::Parameter1) == 1);
  assert(propertyMap.count(EdgeProperty::Parameter2) == 1);
  double base = propertyMap.at(EdgeProperty::Parameter1)->getValue();
  double exp = propertyMap.at(EdgeProperty::Parameter2)->getValue();
  value = std::pow(base, exp);
  this->setClean();
}

ScalarAbsNode::ScalarAbsNode() : AbstractNode()
{

}

void ScalarAbsNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 1);
  assert(propertyMap.count(EdgeProperty::None) == 1);
  double valueIn = propertyMap.at(EdgeProperty::None)->getValue();
  value = std::fabs(valueIn);
  this->setClean();
}

ScalarMinNode::ScalarMinNode() : AbstractNode()
{

}

void ScalarMinNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 2);
  assert(propertyMap.count(EdgeProperty::Parameter1) == 1);
  assert(propertyMap.count(EdgeProperty::Parameter2) == 1);
  double p1 = propertyMap.at(EdgeProperty::Parameter1)->getValue();
  double p2 = propertyMap.at(EdgeProperty::Parameter2)->getValue();
  value = std::min(p1, p2);
  this->setClean();
}

ScalarMaxNode::ScalarMaxNode() : AbstractNode()
{

}

void ScalarMaxNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 2);
  assert(propertyMap.count(EdgeProperty::Parameter1) == 1);
  assert(propertyMap.count(EdgeProperty::Parameter2) == 1);
  double p1 = propertyMap.at(EdgeProperty::Parameter1)->getValue();
  double p2 = propertyMap.at(EdgeProperty::Parameter2)->getValue();
  value = std::max(p1, p2);
  this->setClean();
}

ScalarFloorNode::ScalarFloorNode()
{

}

void ScalarFloorNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 2);
  assert(propertyMap.count(EdgeProperty::Parameter1) == 1);
  assert(propertyMap.count(EdgeProperty::Parameter2) == 1);
  double p1 = propertyMap.at(EdgeProperty::Parameter1)->getValue();
  double p2 = propertyMap.at(EdgeProperty::Parameter2)->getValue();
  
  if (p2 == 0)
  {
    //don't divide by zero.
    value = p1;
    this->setClean();
    return;
  }
  
  p2 = std::fabs(p2);//negative doesn't make sense for a round factor.
  
  int factor = static_cast<int>(p1 / p2);
  if (p1 < 0)
  {
    if (std::fabs(std::fmod(p1, p2)) > 0)
      factor--;
  }
  value = p2 * static_cast<double>(factor);
  this->setClean();
}

ScalarCeilNode::ScalarCeilNode()
{

}

void ScalarCeilNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 2);
  assert(propertyMap.count(EdgeProperty::Parameter1) == 1);
  assert(propertyMap.count(EdgeProperty::Parameter2) == 1);
  double p1 = propertyMap.at(EdgeProperty::Parameter1)->getValue();
  double p2 = propertyMap.at(EdgeProperty::Parameter2)->getValue();
  
  if (p2 == 0)
  {
    //don't divide by zero.
    value = p1;
    this->setClean();
    return;
  }
  
  p2 = std::fabs(p2); //negative doesn't make sense for a round factor.
  
  int factor = static_cast<int>(p1 / p2);
  if (p1 > 0)
  {
    if (std::fmod(p1, p2) > 0)
      factor++;
  }

  value = p2 * static_cast<double>(factor);
  this->setClean();
}

ScalarRoundNode::ScalarRoundNode()
{

}

void ScalarRoundNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 2);
  assert(propertyMap.count(EdgeProperty::Parameter1) == 1);
  assert(propertyMap.count(EdgeProperty::Parameter2) == 1);
  double p1 = propertyMap.at(EdgeProperty::Parameter1)->getValue();
  double p2 = propertyMap.at(EdgeProperty::Parameter2)->getValue();
  
  if (p2 == 0)
  {
    //don't divide by zero.
    value = p1;
    this->setClean();
    return;
  }
  
  p2 = std::fabs(p2); //negative doesn't make sense for a round factor.
  
  int factor = static_cast<int>(p1 / p2);
  if (p1 < 0)
  {
    if (std::fabs(std::fmod(p1, p2)) > (p2 / 2))
        factor--;
  }
  else
  {
    if (std::fmod(p1, p2) >= (p2 / 2))
        factor++;
  }
  
  value = p2 * static_cast<double>(factor);
  this->setClean();
}

ScalarRadToDegNode::ScalarRadToDegNode() : AbstractNode()
{

}

void ScalarRadToDegNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 1);
  assert(propertyMap.count(EdgeProperty::None) == 1);
  double valueIn = propertyMap.at(EdgeProperty::None)->getValue();
  value = valueIn * 180 / boost::math::constants::pi<double>();
  this->setClean();
}

ScalarDegToRadNode::ScalarDegToRadNode() : AbstractNode()
{

}

void ScalarDegToRadNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 1);
  assert(propertyMap.count(EdgeProperty::None) == 1);
  double valueIn = propertyMap.at(EdgeProperty::None)->getValue();
  value = valueIn * boost::math::constants::pi<double>() / 180.0;
  this->setClean();
}

ScalarLogNode::ScalarLogNode() : AbstractNode()
{

}

void ScalarLogNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 1);
  assert(propertyMap.count(EdgeProperty::None) == 1);
  double valueIn = propertyMap.at(EdgeProperty::None)->getValue();
  value = std::log(valueIn);
  this->setClean();
}

ScalarExpNode::ScalarExpNode() : AbstractNode()
{

}

void ScalarExpNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 1);
  assert(propertyMap.count(EdgeProperty::None) == 1);
  double valueIn = propertyMap.at(EdgeProperty::None)->getValue();
  value = std::exp(valueIn);
  this->setClean();
}

ScalarSqrtNode::ScalarSqrtNode() : AbstractNode()
{
  
}

void ScalarSqrtNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 1);
  assert(propertyMap.count(EdgeProperty::None) == 1);
  double valueIn = propertyMap.at(EdgeProperty::None)->getValue();
  value = std::sqrt(valueIn);
  this->setClean();
}

ScalarHypotNode::ScalarHypotNode() : AbstractNode()
{

}

void ScalarHypotNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 2);
  assert(propertyMap.count(EdgeProperty::Parameter1) == 1);
  assert(propertyMap.count(EdgeProperty::Parameter2) == 1);
  double p1 = propertyMap.at(EdgeProperty::Parameter1)->getValue();
  double p2 = propertyMap.at(EdgeProperty::Parameter2)->getValue();
  value = hypot(p1, p2);
  this->setClean();
}

ScalarConditionalNode::ScalarConditionalNode() : AbstractNode(), type(ScalarConditionalNode::None)
{

}

void ScalarConditionalNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 4);
  assert(propertyMap.count(EdgeProperty::Lhs) == 1);
  assert(propertyMap.count(EdgeProperty::Rhs) == 1);
  assert(propertyMap.count(EdgeProperty::Then) == 1);
  assert(propertyMap.count(EdgeProperty::Else) == 1);
  double lhs = propertyMap.at(EdgeProperty::Lhs)->getValue();
  double rhs = propertyMap.at(EdgeProperty::Rhs)->getValue();
  double localThen = propertyMap.at(EdgeProperty::Then)->getValue();
  double localElse = propertyMap.at(EdgeProperty::Else)->getValue();
  
  if (type == GreaterThan)
  {
    if (lhs > rhs)
      value = localThen;
    else
      value = localElse;
  }
  else if(type == LessThan)
  {
    if (lhs < rhs)
      value = localThen;
    else
      value = localElse;
  }
  else if(type == GreaterThanEqual)
  {
    if (lhs >= rhs)
      value = localThen;
    else
      value = localElse;
  }
  else if(type == LessThanEqual)
  {
    if (lhs <= rhs)
      value = localThen;
    else
      value = localElse;
  }
  else if(type == Equal)
  {
    if (lhs == rhs)
      value = localThen;
    else
      value = localElse;
  }
  else if(type == NotEqual)
  {
    if (lhs != rhs)
      value = localThen;
    else
      value = localElse;
  }
  else
    assert(0); //unrecognized operator.
  this->setClean();
}

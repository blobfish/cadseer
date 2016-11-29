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

#include <boost/uuid/uuid_generators.hpp>
#include <boost/math/constants/constants.hpp>

#include <expressions/abstractnode.h>

static thread_local boost::uuids::random_generator gen;

using namespace expr;

AbstractNode::AbstractNode() : dirtyTest(true), value(1.0)
{

}

double AbstractNode::getValue()
{
  assert(this->isClean());
  return value;
}

ConstantNode::ConstantNode() : AbstractNode()
{

}

FormulaNode::FormulaNode() : AbstractNode(), name("no name"), id(gen())
{

}

void FormulaNode::calculate(const EdgePropertiesMap &propertyMap)
{
  assert(propertyMap.size() == 1);
  assert(propertyMap.count(EdgeProperty::None) == 1);
  value = propertyMap.at(EdgeProperty::None);
  this->setClean();
}

AdditionNode::AdditionNode() : AbstractNode()
{

}

void AdditionNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 2);
  assert(propertyMap.count(EdgeProperty::Lhs) == 1);
  assert(propertyMap.count(EdgeProperty::Rhs) == 1);
  value = propertyMap.at(EdgeProperty::Lhs) + propertyMap.at(EdgeProperty::Rhs);
  this->setClean();
}

SubtractionNode::SubtractionNode() : AbstractNode()
{

}

void SubtractionNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 2);
  assert(propertyMap.count(EdgeProperty::Lhs) == 1);
  assert(propertyMap.count(EdgeProperty::Rhs) == 1);
  value = propertyMap.at(EdgeProperty::Lhs) - propertyMap.at(EdgeProperty::Rhs);
  this->setClean();
}

MultiplicationNode::MultiplicationNode() : AbstractNode()
{

}

void MultiplicationNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 2);
  assert(propertyMap.count(EdgeProperty::Lhs) == 1);
  assert(propertyMap.count(EdgeProperty::Rhs) == 1);
  value = propertyMap.at(EdgeProperty::Lhs) * propertyMap.at(EdgeProperty::Rhs);
  this->setClean();
}

DivisionNode::DivisionNode() : AbstractNode()
{

}

void DivisionNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 2);
  assert(propertyMap.count(EdgeProperty::Lhs) == 1);
  assert(propertyMap.count(EdgeProperty::Rhs) == 1);
  value = propertyMap.at(EdgeProperty::Lhs) / propertyMap.at(EdgeProperty::Rhs);
  this->setClean();
}

ParenthesesNode::ParenthesesNode() : AbstractNode()
{

}

void ParenthesesNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 1);
  assert(propertyMap.count(EdgeProperty::None) == 1);
  value = propertyMap.at(EdgeProperty::None);
  this->setClean();
}

SinNode::SinNode() : AbstractNode()
{

}

void SinNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 1);
  assert(propertyMap.count(EdgeProperty::None) == 1);
  double childValue = propertyMap.at(EdgeProperty::None);
  value = std::sin(childValue);
  this->setClean();
}

CosNode::CosNode() : AbstractNode()
{

}

void CosNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 1);
  assert(propertyMap.count(EdgeProperty::None) == 1);
  double childValue = propertyMap.at(EdgeProperty::None);
  value = std::cos(childValue);
  this->setClean();
}

TanNode::TanNode() : AbstractNode()
{

}

void TanNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 1);
  assert(propertyMap.count(EdgeProperty::None) == 1);
  double childValue = propertyMap.at(EdgeProperty::None);
  value = std::tan(childValue);
  this->setClean();
}

AsinNode::AsinNode() : AbstractNode()
{

}

void AsinNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 1);
  assert(propertyMap.count(EdgeProperty::None) == 1);
  double childValue = propertyMap.at(EdgeProperty::None);
  value = std::asin(childValue);
  this->setClean();
}

AcosNode::AcosNode() : AbstractNode()
{

}

void AcosNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 1);
  assert(propertyMap.count(EdgeProperty::None) == 1);
  double childValue = propertyMap.at(EdgeProperty::None);
  value = std::acos(childValue);
  this->setClean();
}

AtanNode::AtanNode() : AbstractNode()
{

}

void AtanNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 1);
  assert(propertyMap.count(EdgeProperty::None) == 1);
  double childValue = propertyMap.at(EdgeProperty::None);
  value = std::atan(childValue);
  this->setClean();
}

Atan2Node::Atan2Node() : AbstractNode()
{

}

void Atan2Node::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 2);
  assert(propertyMap.count(EdgeProperty::Parameter1) == 1);
  assert(propertyMap.count(EdgeProperty::Parameter2) == 1);
  // y is first to match standard function.
  double y = propertyMap.at(EdgeProperty::Parameter1);
  double x = propertyMap.at(EdgeProperty::Parameter2);
  value = std::atan2(y, x);
  this->setClean();
}

PowNode::PowNode()
{

}

void PowNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 2);
  assert(propertyMap.count(EdgeProperty::Parameter1) == 1);
  assert(propertyMap.count(EdgeProperty::Parameter2) == 1);
  double base = propertyMap.at(EdgeProperty::Parameter1);
  double exp = propertyMap.at(EdgeProperty::Parameter2);
  value = std::pow(base, exp);
  this->setClean();
}

AbsNode::AbsNode() : AbstractNode()
{

}

void AbsNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 1);
  assert(propertyMap.count(EdgeProperty::None) == 1);
  double valueIn = propertyMap.at(EdgeProperty::None);
  value = std::fabs(valueIn);
  this->setClean();
}

MinNode::MinNode() : AbstractNode()
{

}

void MinNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 2);
  assert(propertyMap.count(EdgeProperty::Parameter1) == 1);
  assert(propertyMap.count(EdgeProperty::Parameter2) == 1);
  double p1 = propertyMap.at(EdgeProperty::Parameter1);
  double p2 = propertyMap.at(EdgeProperty::Parameter2);
  value = std::min(p1, p2);
  this->setClean();
}

MaxNode::MaxNode() : AbstractNode()
{

}

void MaxNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 2);
  assert(propertyMap.count(EdgeProperty::Parameter1) == 1);
  assert(propertyMap.count(EdgeProperty::Parameter2) == 1);
  double p1 = propertyMap.at(EdgeProperty::Parameter1);
  double p2 = propertyMap.at(EdgeProperty::Parameter2);
  value = std::max(p1, p2);
  this->setClean();
}

FloorNode::FloorNode()
{

}

void FloorNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 2);
  assert(propertyMap.count(EdgeProperty::Parameter1) == 1);
  assert(propertyMap.count(EdgeProperty::Parameter2) == 1);
  double p1 = propertyMap.at(EdgeProperty::Parameter1);
  double p2 = propertyMap.at(EdgeProperty::Parameter2);
  
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

CeilNode::CeilNode()
{

}

void CeilNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 2);
  assert(propertyMap.count(EdgeProperty::Parameter1) == 1);
  assert(propertyMap.count(EdgeProperty::Parameter2) == 1);
  double p1 = propertyMap.at(EdgeProperty::Parameter1);
  double p2 = propertyMap.at(EdgeProperty::Parameter2);
  
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

RoundNode::RoundNode()
{

}

void RoundNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 2);
  assert(propertyMap.count(EdgeProperty::Parameter1) == 1);
  assert(propertyMap.count(EdgeProperty::Parameter2) == 1);
  double p1 = propertyMap.at(EdgeProperty::Parameter1);
  double p2 = propertyMap.at(EdgeProperty::Parameter2);
  
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

RadToDegNode::RadToDegNode() : AbstractNode()
{

}

void RadToDegNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 1);
  assert(propertyMap.count(EdgeProperty::None) == 1);
  double valueIn = propertyMap.at(EdgeProperty::None);
  value = valueIn * 180 / boost::math::constants::pi<double>();
  this->setClean();
}

DegToRadNode::DegToRadNode() : AbstractNode()
{

}

void DegToRadNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 1);
  assert(propertyMap.count(EdgeProperty::None) == 1);
  double valueIn = propertyMap.at(EdgeProperty::None);
  value = valueIn * boost::math::constants::pi<double>() / 180.0;
  this->setClean();
}

LogNode::LogNode() : AbstractNode()
{

}

void LogNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 1);
  assert(propertyMap.count(EdgeProperty::None) == 1);
  double valueIn = propertyMap.at(EdgeProperty::None);
  value = std::log(valueIn);
  this->setClean();
}

ExpNode::ExpNode() : AbstractNode()
{

}

void ExpNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 1);
  assert(propertyMap.count(EdgeProperty::None) == 1);
  double valueIn = propertyMap.at(EdgeProperty::None);
  value = std::exp(valueIn);
  this->setClean();
}

SqrtNode::SqrtNode() : AbstractNode()
{
  
}

void SqrtNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 1);
  assert(propertyMap.count(EdgeProperty::None) == 1);
  double valueIn = propertyMap.at(EdgeProperty::None);
  value = std::sqrt(valueIn);
  this->setClean();
}

HypotNode::HypotNode() : AbstractNode()
{

}

void HypotNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 2);
  assert(propertyMap.count(EdgeProperty::Parameter1) == 1);
  assert(propertyMap.count(EdgeProperty::Parameter2) == 1);
  double p1 = propertyMap.at(EdgeProperty::Parameter1);
  double p2 = propertyMap.at(EdgeProperty::Parameter2);
  value = hypot(p1, p2);
  this->setClean();
}

ConditionalNode::ConditionalNode() : AbstractNode(), type(ConditionalNode::None)
{

}

void ConditionalNode::calculate(const EdgePropertiesMap& propertyMap)
{
  assert(propertyMap.size() == 4);
  assert(propertyMap.count(EdgeProperty::Lhs) == 1);
  assert(propertyMap.count(EdgeProperty::Rhs) == 1);
  assert(propertyMap.count(EdgeProperty::Then) == 1);
  assert(propertyMap.count(EdgeProperty::Else) == 1);
  double lhs = propertyMap.at(EdgeProperty::Lhs);
  double rhs = propertyMap.at(EdgeProperty::Rhs);
  double localThen = propertyMap.at(EdgeProperty::Then);
  double localElse = propertyMap.at(EdgeProperty::Else);
  
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

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

using namespace expr;

AbstractNode::AbstractNode() : dirtyTest(true), value(1.0), id(boost::uuids::random_generator()())
{

}

double AbstractNode::getValue()
{
  assert(this->isClean());
  return value;
}

void AbstractNode::copy(const AbstractNode* source)
{
  this->dirtyTest = source->dirtyTest;
  this->value = source->value;
  this->id = source->id;
}

ConstantNode::ConstantNode() : AbstractNode()
{

}

AbstractNode* ConstantNode::clone()
{
  ConstantNode *out = new ConstantNode();
  out->copy(this);
  return out;
}

FormulaNode::FormulaNode() : AbstractNode()
{

}

void FormulaNode::calculate(const EdgePropertiesMap &propertyMap)
{
  assert(propertyMap.size() == 1);
  assert(propertyMap.count(EdgeProperty::None) == 1);
  value = propertyMap.at(EdgeProperty::None);
  this->setClean();
}

AbstractNode* FormulaNode::clone()
{
  FormulaNode *out = new FormulaNode();
  out->copy(this);
  out->name = this->name;
  return out;
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

AbstractNode* AdditionNode::clone()
{
  AdditionNode *out = new AdditionNode();
  out->copy(this);
  return out;
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

AbstractNode* SubtractionNode::clone()
{
  SubtractionNode *out = new SubtractionNode();
  out->copy(this);
  return out;
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

AbstractNode* MultiplicationNode::clone()
{
  MultiplicationNode *out = new MultiplicationNode();
  out->copy(this);
  return out;
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

AbstractNode* DivisionNode::clone()
{
  DivisionNode *out = new DivisionNode();
  out->copy(this);
  return out;
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

AbstractNode* ParenthesesNode::clone()
{
  ParenthesesNode *out = new ParenthesesNode();
  out->copy(this);
  return out;
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

AbstractNode* SinNode::clone()
{
  SinNode *out = new SinNode();
  out->copy(this);
  return out;
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

AbstractNode* CosNode::clone()
{
  CosNode *out = new CosNode();
  out->copy(this);
  return out;
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

AbstractNode* TanNode::clone()
{
  TanNode *out = new TanNode();
  out->copy(this);
  return out;
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

AbstractNode* AsinNode::clone()
{
  AsinNode *out = new AsinNode();
  out->copy(this);
  return out;
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

AbstractNode* AcosNode::clone()
{
  AcosNode *out = new AcosNode();
  out->copy(this);
  return out;
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

AbstractNode* AtanNode::clone()
{
  AtanNode *out = new AtanNode();
  out->copy(this);
  return out;
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

AbstractNode* Atan2Node::clone()
{
  Atan2Node *out = new Atan2Node();
  out->copy(this);
  return out;
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

AbstractNode* PowNode::clone()
{
  PowNode *out = new PowNode();
  out->copy(this);
  return out;
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

AbstractNode* AbsNode::clone()
{
  AbsNode *out = new AbsNode();
  out->copy(this);
  return out;
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

AbstractNode* MinNode::clone()
{
  MinNode *out = new MinNode();
  out->copy(this);
  return out;
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

AbstractNode* MaxNode::clone()
{
  MaxNode *out = new MaxNode();
  out->copy(this);
  return out;
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

AbstractNode* FloorNode::clone()
{
  FloorNode *out = new FloorNode();
  out->copy(this);
  return out;
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

AbstractNode* CeilNode::clone()
{
  CeilNode *out = new CeilNode();
  out->copy(this);
  return out;
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

AbstractNode* RoundNode::clone()
{
  RoundNode *out = new RoundNode();
  out->copy(this);
  return out;
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

AbstractNode* RadToDegNode::clone()
{
  RadToDegNode *out = new RadToDegNode();
  out->copy(this);
  return out;
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

AbstractNode* DegToRadNode::clone()
{
  DegToRadNode *out = new DegToRadNode();
  out->copy(this);
  return out;
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

AbstractNode* LogNode::clone()
{
  LogNode *out = new LogNode();
  out->copy(this);
  return out;
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

AbstractNode* ExpNode::clone()
{
  ExpNode *out = new ExpNode();
  out->copy(this);
  return out;
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

AbstractNode* SqrtNode::clone()
{
  SqrtNode *out = new SqrtNode();
  out->copy(this);
  return out;
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

AbstractNode* HypotNode::clone()
{
  HypotNode *out = new HypotNode();
  out->copy(this);
  return out;
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

AbstractNode* ConditionalNode::clone()
{
  ConditionalNode *out = new ConditionalNode();
  out->copy(this);
  out->type = this->type;
  return out;
}


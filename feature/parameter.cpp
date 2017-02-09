/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2015  Thomas S. Anderson blobfish.at.gmx.com
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

#include <limits>

#include <tools/idtools.h>
#include <project/serial/xsdcxxoutput/featurebase.h>
#include <feature/parameter.h>

using namespace ftr;

ParameterBoundary::ParameterBoundary(double valueIn, End endIn) :
  value(valueIn), end(endIn)
{}

ParameterInterval::ParameterInterval(const ParameterBoundary &lowerIn, const ParameterBoundary &upperIn) :
  lower(lowerIn), upper(upperIn)
{}

bool ParameterInterval::test(double testValue) const
{
  assert(lower.end != ParameterBoundary::End::None);
  assert(upper.end != ParameterBoundary::End::None);
  
  if (lower.end == ParameterBoundary::End::Open)
  {
    if (!(testValue > lower.value))
      return false;
  }
  else //lower.end == ParameterBoundary::End::Closed
  {
    if (!(testValue >= lower.value))
      return false;
  }
  
  if (upper.end == ParameterBoundary::End::Open)
  {
    if (!(testValue < upper.value))
      return false;
  }
  else //upper.end == ParameterBoundary::End::Closed
  {
    if (!(testValue <= upper.value))
      return false;
  }
  
  return true;
}

ParameterConstraint ParameterConstraint::buildAll()
{
  ParameterBoundary lower(std::numeric_limits<double>::lowest(), ParameterBoundary::End::Closed);
  ParameterBoundary upper(std::numeric_limits<double>::max(), ParameterBoundary::End::Closed);
  ParameterInterval interval(lower, upper);
  
  ParameterConstraint out;
  out.intervals.push_back(interval);
  return out;
}

ParameterConstraint ParameterConstraint::buildNonZeroPositive()
{
  ParameterBoundary lower(0.0, ParameterBoundary::End::Open);
  ParameterBoundary upper(std::numeric_limits<double>::max(), ParameterBoundary::End::Closed);
  ParameterInterval interval(lower, upper);
  
  ParameterConstraint out;
  out.intervals.push_back(interval);
  return out;
}

ParameterConstraint ParameterConstraint::buildZeroPositive()
{
  ParameterBoundary lower(0.0, ParameterBoundary::End::Closed);
  ParameterBoundary upper(std::numeric_limits<double>::max(), ParameterBoundary::End::Closed);
  ParameterInterval interval(lower, upper);
  
  ParameterConstraint out;
  out.intervals.push_back(interval);
  return out;
}

ParameterConstraint ParameterConstraint::buildNonZeroNegative()
{
  ParameterBoundary lower(std::numeric_limits<double>::lowest(), ParameterBoundary::End::Closed);
  ParameterBoundary upper(0.0, ParameterBoundary::End::Open);
  ParameterInterval interval(lower, upper);
  
  ParameterConstraint out;
  out.intervals.push_back(interval);
  return out;
}

ParameterConstraint ParameterConstraint::buildZeroNegative()
{
  ParameterBoundary lower(std::numeric_limits<double>::lowest(), ParameterBoundary::End::Closed);
  ParameterBoundary upper(0.0, ParameterBoundary::End::Closed);
  ParameterInterval interval(lower, upper);
  
  ParameterConstraint out;
  out.intervals.push_back(interval);
  return out;
}

ParameterConstraint ParameterConstraint::buildNonZero()
{
  ParameterBoundary lower1(std::numeric_limits<double>::lowest(), ParameterBoundary::End::Closed);
  ParameterBoundary upper1(0.0, ParameterBoundary::End::Open);
  ParameterInterval interval1(lower1, upper1);
  
  ParameterBoundary lower2(0.0, ParameterBoundary::End::Open);
  ParameterBoundary upper2(std::numeric_limits<double>::max(), ParameterBoundary::End::Closed);
  ParameterInterval interval2(lower2, upper2);
  
  ParameterConstraint out;
  out.intervals.push_back(interval1);
  out.intervals.push_back(interval2);
  return out;
}

ParameterConstraint ParameterConstraint::buildUnit()
{
  ParameterBoundary lower(0.0, ParameterBoundary::End::Closed);
  ParameterBoundary upper(1.0, ParameterBoundary::End::Closed);
  ParameterInterval interval(lower, upper);
  
  ParameterConstraint out;
  out.intervals.push_back(interval);
  return out;
}

void ParameterConstraint::unitTest()
{
  ParameterConstraint all = ParameterConstraint::buildAll();
  assert(all.test(-1200.345) == true);
  assert(all.test(-234) == true);
  assert(all.test(13456.987) == true);
  assert(all.test(593) == true);
  assert(all.test(0) == true);
  
  ParameterConstraint nonZeroPositive = ParameterConstraint::buildNonZeroPositive();
  assert(nonZeroPositive.test(-231.432) == false);
  assert(nonZeroPositive.test(-562) == false);
  assert(nonZeroPositive.test(0) == false);
  assert(nonZeroPositive.test(74321) == true);
  assert(nonZeroPositive.test(984.3587) == true);
  
  ParameterConstraint zeroPositive = ParameterConstraint::buildZeroPositive();
  assert(zeroPositive.test(-231.432) == false);
  assert(zeroPositive.test(-562) == false);
  assert(zeroPositive.test(0) == true);
  assert(zeroPositive.test(74321) == true);
  assert(zeroPositive.test(984.3587) == true);
  
  ParameterConstraint nonZeroNegative = ParameterConstraint::buildNonZeroNegative();
  assert(nonZeroNegative.test(-231.432) == true);
  assert(nonZeroNegative.test(-562) == true);
  assert(nonZeroNegative.test(0) == false);
  assert(nonZeroNegative.test(74321) == false);
  assert(nonZeroNegative.test(984.3587) == false);
  
  ParameterConstraint zeroNegative = ParameterConstraint::buildZeroNegative();
  assert(zeroNegative.test(-231.432) == true);
  assert(zeroNegative.test(-562) == true);
  assert(zeroNegative.test(0) == true);
  assert(zeroNegative.test(74321) == false);
  assert(zeroNegative.test(984.3587) == false);
  
  ParameterConstraint nonZero = ParameterConstraint::buildNonZero();
  assert(nonZero.test(-231.432) == true);
  assert(nonZero.test(-562) == true);
  assert(nonZero.test(0) == false);
  assert(nonZero.test(74321) == true);
  assert(nonZero.test(984.3587) == true);
  
  ParameterConstraint unit = ParameterConstraint::buildUnit();
  assert(unit.test(-100) == false);
  assert(unit.test(-0.125) == false);
  assert(unit.test(0) == true);
  assert(unit.test(0.5) == true);
  assert(unit.test(1) == true);
  assert(unit.test(1.125) == false);
  assert(unit.test(100) == false);
}

bool ParameterConstraint::test(double testValue) const
{
  assert(!intervals.empty());
  
  bool out = intervals.at(0).test(testValue);
  for (auto it = intervals.begin() + 1; it != intervals.end(); ++it)
    out |= it->test(testValue);
  
  return out;
}

Parameter::Parameter() :
  name("no name"),
  value(1.0),
  id(gu::createRandomId()),
  constraint(ParameterConstraint::buildAll())
{
}

Parameter::Parameter(const QString& nameIn, double valueIn) :
  name(nameIn),
  value(valueIn),
  id(gu::createRandomId()),
  constraint(ParameterConstraint::buildAll())
{
}

Parameter& Parameter::operator=(double valueIn)
{
  setValue(valueIn);
  return *this;
}

void Parameter::setConstant(bool constantIn)
{
  if (constantIn == constant)
    return;
  constant = constantIn;
  constantChangedSignal();
}

void Parameter::setValue(double valueIn)
{
  if (value == valueIn)
    return;
  
  if (!isValidValue(valueIn))
    return;
  
  value = valueIn;
  valueChangedSignal();
}

void Parameter::setConstraint(const ParameterConstraint &cIn)
{
  /* not really worried about the current value conflicting
   * with new constraint. Constraints should be set at construction
   * and not swapped in and out during run
   */
  constraint = cIn;
}

bool Parameter::isValidValue(const double &valueIn)
{
  return constraint.test(valueIn);
}

prj::srl::Parameter Parameter::serialOut() const
{
  return prj::srl::Parameter(name.toStdString(), constant, value, gu::idToString(id)); 
}

void Parameter::serialIn(const prj::srl::Parameter& sParameterIn)
{
  name = QString::fromStdString(sParameterIn.name());
  setConstant(sParameterIn.constant());
  setValue(sParameterIn.value());
  id = gu::stringToId(sParameterIn.id());
}

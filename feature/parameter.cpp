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

#include <boost/variant.hpp>

#include <tools/idtools.h>
#include <project/serial/xsdcxxoutput/featurebase.h>
#include <feature/parameter.h>

using namespace ftr::prm;
using boost::filesystem::path;

Boundary::Boundary(double valueIn, Boundary::End endIn) :
  value(valueIn), end(endIn)
{}

Interval::Interval(const Boundary &lowerIn, const Boundary &upperIn) :
  lower(lowerIn), upper(upperIn)
{}

bool Interval::test(double testValue) const
{
  assert(lower.end != Boundary::End::None);
  assert(upper.end != Boundary::End::None);
  
  if (lower.end == Boundary::End::Open)
  {
    if (!(testValue > lower.value))
      return false;
  }
  else //lower.end == Boundary::End::Closed
  {
    if (!(testValue >= lower.value))
      return false;
  }
  
  if (upper.end == Boundary::End::Open)
  {
    if (!(testValue < upper.value))
      return false;
  }
  else //upper.end == Boundary::End::Closed
  {
    if (!(testValue <= upper.value))
      return false;
  }
  
  return true;
}

Constraint Constraint::buildAll()
{
  Boundary lower(std::numeric_limits<double>::lowest(), Boundary::End::Closed);
  Boundary upper(std::numeric_limits<double>::max(), Boundary::End::Closed);
  Interval interval(lower, upper);
  
  Constraint out;
  out.intervals.push_back(interval);
  return out;
}

Constraint Constraint::buildNonZeroPositive()
{
  Boundary lower(0.0, Boundary::End::Open);
  Boundary upper(std::numeric_limits<double>::max(), Boundary::End::Closed);
  Interval interval(lower, upper);
  
  Constraint out;
  out.intervals.push_back(interval);
  return out;
}

Constraint Constraint::buildZeroPositive()
{
  Boundary lower(0.0, Boundary::End::Closed);
  Boundary upper(std::numeric_limits<double>::max(), Boundary::End::Closed);
  Interval interval(lower, upper);
  
  Constraint out;
  out.intervals.push_back(interval);
  return out;
}

Constraint Constraint::buildNonZeroNegative()
{
  Boundary lower(std::numeric_limits<double>::lowest(), Boundary::End::Closed);
  Boundary upper(0.0, Boundary::End::Open);
  Interval interval(lower, upper);
  
  Constraint out;
  out.intervals.push_back(interval);
  return out;
}

Constraint Constraint::buildZeroNegative()
{
  Boundary lower(std::numeric_limits<double>::lowest(), Boundary::End::Closed);
  Boundary upper(0.0, Boundary::End::Closed);
  Interval interval(lower, upper);
  
  Constraint out;
  out.intervals.push_back(interval);
  return out;
}

Constraint Constraint::buildNonZero()
{
  Boundary lower1(std::numeric_limits<double>::lowest(), Boundary::End::Closed);
  Boundary upper1(0.0, Boundary::End::Open);
  Interval interval1(lower1, upper1);
  
  Boundary lower2(0.0, Boundary::End::Open);
  Boundary upper2(std::numeric_limits<double>::max(), Boundary::End::Closed);
  Interval interval2(lower2, upper2);
  
  Constraint out;
  out.intervals.push_back(interval1);
  out.intervals.push_back(interval2);
  return out;
}

Constraint Constraint::buildUnit()
{
  Boundary lower(0.0, Boundary::End::Closed);
  Boundary upper(1.0, Boundary::End::Closed);
  Interval interval(lower, upper);
  
  Constraint out;
  out.intervals.push_back(interval);
  return out;
}

void Constraint::unitTest()
{
  Constraint all = Constraint::buildAll();
  assert(all.test(-1200.345) == true);
  assert(all.test(-234) == true);
  assert(all.test(13456.987) == true);
  assert(all.test(593) == true);
  assert(all.test(0) == true);
  
  Constraint nonZeroPositive = Constraint::buildNonZeroPositive();
  assert(nonZeroPositive.test(-231.432) == false);
  assert(nonZeroPositive.test(-562) == false);
  assert(nonZeroPositive.test(0) == false);
  assert(nonZeroPositive.test(74321) == true);
  assert(nonZeroPositive.test(984.3587) == true);
  
  Constraint zeroPositive = Constraint::buildZeroPositive();
  assert(zeroPositive.test(-231.432) == false);
  assert(zeroPositive.test(-562) == false);
  assert(zeroPositive.test(0) == true);
  assert(zeroPositive.test(74321) == true);
  assert(zeroPositive.test(984.3587) == true);
  
  Constraint nonZeroNegative = Constraint::buildNonZeroNegative();
  assert(nonZeroNegative.test(-231.432) == true);
  assert(nonZeroNegative.test(-562) == true);
  assert(nonZeroNegative.test(0) == false);
  assert(nonZeroNegative.test(74321) == false);
  assert(nonZeroNegative.test(984.3587) == false);
  
  Constraint zeroNegative = Constraint::buildZeroNegative();
  assert(zeroNegative.test(-231.432) == true);
  assert(zeroNegative.test(-562) == true);
  assert(zeroNegative.test(0) == true);
  assert(zeroNegative.test(74321) == false);
  assert(zeroNegative.test(984.3587) == false);
  
  Constraint nonZero = Constraint::buildNonZero();
  assert(nonZero.test(-231.432) == true);
  assert(nonZero.test(-562) == true);
  assert(nonZero.test(0) == false);
  assert(nonZero.test(74321) == true);
  assert(nonZero.test(984.3587) == true);
  
  Constraint unit = Constraint::buildUnit();
  assert(unit.test(-100) == false);
  assert(unit.test(-0.125) == false);
  assert(unit.test(0) == true);
  assert(unit.test(0.5) == true);
  assert(unit.test(1) == true);
  assert(unit.test(1.125) == false);
  assert(unit.test(100) == false);
}

bool Constraint::test(double testValue) const
{
  if(intervals.empty())
    return true;
  
  bool out = intervals.at(0).test(testValue);
  for (auto it = intervals.begin() + 1; it != intervals.end(); ++it)
    out &= it->test(testValue);
  
  return out;
}

class DoubleVisitor : public boost::static_visitor<double>
{
public:
  double operator()(double d) const {return d;}
  double operator()(int) const {assert(0); return 0.0;}
  double operator()(bool) const {assert(0); return 0.0;}
  double operator()(const std::string&) const {assert(0); return 0.0;}
  double operator()(const boost::filesystem::path&) const {assert(0); return 0.0;}
  double operator()(const osg::Vec3d&) const {assert(0); return 0.0;}
  double operator()(const osg::Quat&) const {assert(0); return 0.0;}
  double operator()(const osg::Matrixd&) const {assert(0); return 0.0;}
};

class IntVisitor : public boost::static_visitor<int>
{
public:
  int operator()(double) const {assert(0); return 0;}
  int operator()(int i) const {return i;}
  int operator()(bool) const {assert(0); return 0;}
  int operator()(const std::string&) const {assert(0); return 0;}
  int operator()(const boost::filesystem::path&) const {assert(0); return 0;}
  int operator()(const osg::Vec3d&) const {assert(0); return 0;}
  int operator()(const osg::Quat&) const {assert(0); return 0;}
  int operator()(const osg::Matrixd&) const {assert(0); return 0;}
};

class BoolVisitor : public boost::static_visitor<bool>
{
public:
  bool operator()(double) const {assert(0); return false;}
  bool operator()(int) const {assert(0); return false;}
  bool operator()(bool b) const {return b;}
  bool operator()(const std::string&) const {assert(0); return false;}
  bool operator()(const boost::filesystem::path&) const {assert(0); return false;}
  bool operator()(const osg::Vec3d&) const {assert(0); return false;}
  bool operator()(const osg::Quat&) const {assert(0); return false;}
  bool operator()(const osg::Matrixd&) const {assert(0); return false;}
};

class PathVisitor : public boost::static_visitor<path>
{
public:
  path operator()(double) const {assert(0); return path();}
  path operator()(int) const {assert(0); return path();}
  path operator()(bool) const {assert(0); return path();}
  path operator()(const std::string&) const {assert(0); return path();}
  path operator()(const boost::filesystem::path &p) const {return p;}
  path operator()(const osg::Vec3d&) const {assert(0); return path();}
  path operator()(const osg::Quat&) const {assert(0); return path();}
  path operator()(const osg::Matrixd&) const {assert(0); return path();}
};

class Vec3dVisitor : public boost::static_visitor<osg::Vec3d>
{
public:
  osg::Vec3d operator()(double) const {assert(0); return osg::Vec3d();}
  osg::Vec3d operator()(int) const {assert(0); return osg::Vec3d();}
  osg::Vec3d operator()(bool) const {assert(0); return osg::Vec3d();}
  osg::Vec3d operator()(const std::string&) const {assert(0); return osg::Vec3d();}
  osg::Vec3d operator()(const boost::filesystem::path) const {assert(0); return osg::Vec3d();}
  osg::Vec3d operator()(const osg::Vec3d &v) const {return v;}
  osg::Vec3d operator()(const osg::Quat&) const {assert(0); return osg::Vec3d();}
  osg::Vec3d operator()(const osg::Matrixd&) const {assert(0); return osg::Vec3d();}
};

class TypeStringVisitor : public boost::static_visitor<std::string>
{
public:
  std::string operator()(double) const {return "Double";}
  std::string operator()(int) const {return "Int";}
  std::string operator()(bool) const {return "Bool";}
  std::string operator()(const std::string&) const {return "String";}
  std::string operator()(const boost::filesystem::path&) const {return "Path";}
  std::string operator()(const osg::Vec3d&) const {return "Vec3d";}
  std::string operator()(const osg::Quat&) const {return "Quat";}
  std::string operator()(const osg::Matrixd&) const {return "Matrixd";}
};

class SrlVisitor : public boost::static_visitor<prj::srl::ParameterValue>
{
public:
  prj::srl::ParameterValue operator()(double d) const
  {
    prj::srl::ParameterValue out;
    out.aDouble() = d;
    return out;
  }
  prj::srl::ParameterValue operator()(int i) const
  {
    prj::srl::ParameterValue out;
    out.anInteger() = i;
    return out;
  }
  prj::srl::ParameterValue operator()(bool b) const
  {
    prj::srl::ParameterValue out;
    out.aBool() = b;
    return out;
  }
  prj::srl::ParameterValue operator()(const std::string &s) const
  {
    prj::srl::ParameterValue out;
    out.aString() = s;
    return out;
  }
  prj::srl::ParameterValue operator()(const boost::filesystem::path &p) const
  {
    prj::srl::ParameterValue out;
    out.aPath() = p.string();
    return out;
  }
  prj::srl::ParameterValue operator()(const osg::Vec3d &v) const
  {
    prj::srl::ParameterValue out;
    out.aVec3d() = prj::srl::Vec3d(v.x(), v.y(), v.z());
    return out;
  }
  prj::srl::ParameterValue operator()(const osg::Quat &q) const
  {
    prj::srl::ParameterValue out;
    out.aQuat() = prj::srl::Quat(q.x(), q.y(), q.z(), q.z());
    return out;
  }
  prj::srl::ParameterValue operator()(const osg::Matrixd &m) const
  {
    prj::srl::ParameterValue out;
    out.aMatrixd() = prj::srl::Matrixd
    (
      m(0,0), m(0,1), m(0,2), m(0,3),
      m(1,0), m(1,1), m(1,2), m(1,3),
      m(2,0), m(2,1), m(2,2), m(2,3),
      m(3,0), m(3,1), m(3,2), m(3,3)
    );
    return out;
  }
};

Parameter::Parameter(const QString& nameIn, double valueIn) :
  name(nameIn),
  value(valueIn),
  id(gu::createRandomId()),
  constraint(Constraint::buildAll())
{
}

Parameter::Parameter(const QString& nameIn, int valueIn) :
  name(nameIn),
  value(valueIn),
  id(gu::createRandomId()),
  constraint(Constraint::buildAll())
{
}

Parameter::Parameter(const QString& nameIn, bool valueIn) :
  name(nameIn),
  value(valueIn),
  id(gu::createRandomId()),
  constraint()
{
}

Parameter::Parameter(const QString &nameIn, const boost::filesystem::path &valueIn, PathType ptIn) :
  name(nameIn),
  value(valueIn),
  id(gu::createRandomId()),
  constraint(),
  pathType(ptIn)
{
}

Parameter::Parameter(const QString &nameIn, const osg::Vec3d &valueIn) :
  name(nameIn),
  value(valueIn),
  id(gu::createRandomId()),
  constraint()
{
}

void Parameter::setConstant(bool constantIn)
{
  if (constantIn == constant)
    return;
  constant = constantIn;
  constantChangedSignal();
}

const std::type_info& Parameter::getValueType() const
{
  return value.type();
}

std::string Parameter::getValueTypeString() const
{
  return boost::apply_visitor(TypeStringVisitor(), value);
}

bool Parameter::setValue(double valueIn)
{
  if (setValueQuiet(valueIn))
  {
    valueChangedSignal();
    return true;
  }
  
  return false;
}

bool Parameter::setValueQuiet(double valueIn)
{
  if (boost::apply_visitor(DoubleVisitor(), value) == valueIn)
    return false;
  
  if (!isValidValue(valueIn))
    return false;
  
  value = valueIn;
  return true;
}

Parameter::operator double() const
{
  return boost::apply_visitor(DoubleVisitor(), value);
}

bool Parameter::isValidValue(const double &valueIn) const
{
  return constraint.test(valueIn);
}

void Parameter::setConstraint(const Constraint &cIn)
{
  /* not really worried about the current value conflicting
   * with new constraint. Constraints should be set at construction
   * and not swapped in and out during run
   */
  constraint = cIn;
}

bool Parameter::setValue(int valueIn)
{
  if (setValueQuiet(valueIn))
  {
    valueChangedSignal();
    return true;
  }
  
  return false;
}

bool Parameter::setValueQuiet(int valueIn)
{
  if (boost::apply_visitor(IntVisitor(), value) == valueIn)
    return false;
  
  if (!isValidValue(valueIn))
    return false;
  
  value = valueIn;
  return true;
}

bool Parameter::isValidValue(const int &valueIn) const
{
  return constraint.test(static_cast<double>(valueIn));
}

Parameter::operator int() const
{
  return boost::apply_visitor(IntVisitor(), value);
}

bool Parameter::setValue(bool valueIn)
{
  if (setValueQuiet(valueIn))
  {
    valueChangedSignal();
    return true;
  }
  
  return false;
}

bool Parameter::setValueQuiet(bool valueIn)
{
  if (boost::apply_visitor(BoolVisitor(), value) == valueIn)
    return false;
  
  value = valueIn;
  return true;
}

Parameter::operator bool() const
{
  return boost::apply_visitor(BoolVisitor(), value);
}

bool Parameter::setValue(const path &valueIn)
{
  if (setValueQuiet(valueIn))
  {
    valueChangedSignal();
    return true;
  }
  
  return false;
}

bool Parameter::setValueQuiet(const path &valueIn)
{
  if (boost::apply_visitor(PathVisitor(), value) == valueIn)
    return false;
  
  value = valueIn;
  return true;
}

Parameter::operator boost::filesystem::path() const
{
  return boost::apply_visitor(PathVisitor(), value);
}

bool Parameter::setValue(const osg::Vec3d &vIn)
{
  if (setValueQuiet(vIn))
  {
    valueChangedSignal();
    return true;
  }
  
  return false;
}

bool Parameter::setValueQuiet(const osg::Vec3d &vIn)
{
  if (boost::apply_visitor(Vec3dVisitor(), value) == vIn)
    return false;
  
  value = vIn;
  return true;
}

Parameter::operator osg::Vec3d() const
{
  return boost::apply_visitor(Vec3dVisitor(), value);
}

//todo
Parameter::operator std::string() const{return std::string();}
Parameter::operator osg::Quat() const{return osg::Quat();}
Parameter::operator osg::Matrixd() const{return osg::Matrixd();}





prj::srl::Parameter Parameter::serialOut() const
{
  prj::srl::Parameter out(name.toStdString(), constant, gu::idToString(id));
  out.pValue() = boost::apply_visitor(SrlVisitor(), value);
  
  return out;
}

void Parameter::serialIn(const prj::srl::Parameter& sParameterIn)
{
  name = QString::fromStdString(sParameterIn.name());
  constant = sParameterIn.constant();
  id = gu::stringToId(sParameterIn.id());
  
  if (sParameterIn.pValue().present())
  {
    const auto &vIn = sParameterIn.pValue().get();
    if (vIn.aDouble().present())
      value = vIn.aDouble().get();
    else if (vIn.anInteger().present())
      value = vIn.anInteger().get();
    else if (vIn.aBool().present())
      value = vIn.aBool().get();
    else if (vIn.aString().present())
      value = vIn.aString().get();
    else if (vIn.aPath().present())
      value = boost::filesystem::path(vIn.aPath().get());
    else if (vIn.aVec3d().present())
      value = osg::Vec3d(vIn.aVec3d().get().x(), vIn.aVec3d().get().y(), vIn.aVec3d().get().z());
    else if (vIn.aQuat().present())
      value = osg::Quat(vIn.aQuat().get().x(), vIn.aQuat().get().y(), vIn.aQuat().get().z(), vIn.aQuat().get().w());
    else if (vIn.aMatrixd().present())
    {
      const auto &mIn = vIn.aMatrixd().get();
      value = osg::Matrixd
      (
        mIn.i0j0(), mIn.i0j1(), mIn.i0j2(), mIn.i0j3(),
        mIn.i1j0(), mIn.i1j1(), mIn.i1j2(), mIn.i1j3(),
        mIn.i2j0(), mIn.i2j1(), mIn.i2j2(), mIn.i2j3(),
        mIn.i3j0(), mIn.i3j1(), mIn.i3j2(), mIn.i3j3()
      );
    }
  }
  else if (sParameterIn.value().present())
  {
    value = sParameterIn.value().get();
  }
  else
    assert(0);
}

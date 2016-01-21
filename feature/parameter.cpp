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

#include <project/serial/xsdcxxoutput/featurebase.h>
#include "parameter.h"

using namespace ftr;

Parameter::Parameter()
{
  name = "no name";
  value = 1.0;
}

Parameter::Parameter(const std::string& nameIn, double valueIn)
{
  name = nameIn;
  value = valueIn;
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
  if (!isConstant())
    return;
  
  value = valueIn;
  valueChangedSignal();
}

prj::srl::Parameter Parameter::serialOut() const
{
  return prj::srl::Parameter(name, constant, value); 
}

void Parameter::serialIn(const prj::srl::Parameter& sParameterIn)
{
  name = sParameterIn.name();
  setConstant(sParameterIn.constant());
  setValue(sParameterIn.value());
}

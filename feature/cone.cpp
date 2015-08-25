/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2015  tanderson <email>
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

#include <BRepPrimAPI_MakeCone.hxx>

#include "cone.h"

using namespace Feature;

Cone::Cone() : Base(), radius1(5.0), radius2(0.0), height(10.0)
{

}

void Cone::setRadius1(const double& radius1In)
{
  if (radius1 == radius1In)
    return;
  assert(radius1In > Precision::Confusion());
  setDirty();
  radius1 = radius1In;
}

void Cone::setRadius2(const double& radius2In)
{
  if (radius2 == radius2In)
    return;
  assert(radius2In >= 0.0); //radius2 can be zero. a point.
  setDirty();
  radius2 = radius2In;
}

void Cone::setHeight(const double& heightIn)
{
  if (height == heightIn)
    return;
  assert(heightIn > Precision::Confusion());
  setDirty();
  height = heightIn;
}

void Cone::setParameters(const double& radius1In, const double& radius2In, const double& heightIn)
{
  //asserts and dirty in setters.
  setRadius1(radius1In);
  setRadius2(radius2In);
  setHeight(heightIn);
}

void Cone::getParameters(double& radius1Out, double& radius2Out, double& heightOut) const
{
  radius1Out = radius1;
  radius2Out = radius2;
  heightOut = height;
}

void Cone::update(const UpdateMap& mapIn)
{
  try
  {
    BRepPrimAPI_MakeCone coneMaker(radius1, radius2, height);
    coneMaker.Build();
    assert(coneMaker.IsDone());
    shape = coneMaker.Solid();
    setClean();
  }
  catch (Standard_Failure)
  {
    Handle_Standard_Failure e = Standard_Failure::Caught();
    std::cout << std::endl << "Error in cone update. " << e->GetMessageString() << std::endl;
  }
}

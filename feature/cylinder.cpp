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

#include <assert.h>

#include <BRepPrimAPI_MakeCylinder.hxx>

#include "cylinder.h"

using namespace Feature;

Cylinder::Cylinder() : Base(), radius(5.0), height(20.0)
{

}

void Cylinder::setRadius(const double& radiusIn)
{
  if (radius == radiusIn)
    return;
  assert(radiusIn > Precision::Confusion());
  setDirty();
  radius = radiusIn;
}

void Cylinder::setHeight(const double& heightIn)
{
  if (height == heightIn)
    return;
  assert(heightIn > Precision::Confusion());
  setDirty();
  height = heightIn;
}

void Cylinder::setParameters(const double& radiusIn, const double& heightIn)
{
  //dirty and asserts in setters.
  setRadius(radiusIn);
  setHeight(heightIn);
}

void Cylinder::getParameters(double& radiusOut, double& heightOut) const
{
  radiusOut = radius;
  heightOut = height;
}

void Cylinder::update(const UpdateMap& mapIn)
{
  try
  {
    BRepPrimAPI_MakeCylinder cylinderMaker(radius, height);
    cylinderMaker.Build();
    assert(cylinderMaker.IsDone());
    shape = cylinderMaker.Shape();
    setClean();
  }
  catch (Standard_Failure)
  {
    Handle_Standard_Failure e = Standard_Failure::Caught();
    std::cout << std::endl << "Error in cylinder update. " << e->GetMessageString() << std::endl;
  }
}

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

#include <BRepPrimAPI_MakeSphere.hxx>

#include "sphere.h"

using namespace Feature;

Sphere::Sphere() : Base(), radius(5.0)
{

}

void Sphere::setRadius(const double& radiusIn)
{
  if (radius == radiusIn)
    return;
  assert(radiusIn > Precision::Confusion());
  setDirty();
  radius = radiusIn;
}

void Sphere::update(const UpdateMap& mapIn)
{
  try
  {
    BRepPrimAPI_MakeSphere sphereMaker(radius);
    sphereMaker.Build();
    assert(sphereMaker.IsDone());
    shape = sphereMaker.Shape();
    setClean();
  }
  catch (Standard_Failure)
  {
    Handle_Standard_Failure e = Standard_Failure::Caught();
    std::cout << std::endl << "Error in sphere update. " << e->GetMessageString() << std::endl;
  }
}

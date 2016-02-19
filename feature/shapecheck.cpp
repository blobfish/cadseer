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

#include <TopoDS_Shape.hxx>
#include <BRepCheck_Analyzer.hxx>

#include "shapecheck.h"

namespace ftr
{
  class ShapeCheckPrivate
  {
  public:
    ShapeCheckPrivate(const TopoDS_Shape &shapeIn):
      shape(shapeIn),
      checker(shape)
    {}
    const TopoDS_Shape &shape;
    BRepCheck_Analyzer checker;
  };
}

using namespace ftr;

ShapeCheck::ShapeCheck(const TopoDS_Shape &shapeIn)
{
  try
  {
    shapeCheckPrivate = std::unique_ptr<ShapeCheckPrivate>(new ShapeCheckPrivate(shapeIn));
    successfulRun = true;
  }
  catch (Standard_Failure)
  {
    Handle_Standard_Failure e = Standard_Failure::Caught();
    std::cout << std::endl << "OCCT Exception in ShapeCheck: " << e->GetMessageString() << std::endl;
  }
}

ShapeCheck::~ShapeCheck()
{

}

bool ShapeCheck::isValid()
{
  if (!successfulRun)
    return false;
  if (!shapeCheckPrivate->checker.IsValid())
    return false;
  return true;
}

const BRepCheck_Analyzer& ShapeCheck::getChecker()
{
  return shapeCheckPrivate->checker;
}

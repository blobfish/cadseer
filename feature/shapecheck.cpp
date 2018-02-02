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

#include <tools/occtools.h>
#include <feature/shapecheck.h>

namespace ftr
{
  class ShapeCheckPrivate
  {
  public:
    ShapeCheckPrivate(const TopoDS_Shape &shapeIn):
      shape(shapeIn),
      checker(shape)
    {}
    bool isEmpty()
    {
      if (shape.IsNull())
        return true;
      occt::ShapeVector ss = occt::mapShapes(shape);
      bool foundValid = false;
      for (const auto &s : ss)
      {
        if
        (
          (s.ShapeType() != TopAbs_SHAPE)
          && (s.ShapeType() != TopAbs_COMPOUND)
          && (s.ShapeType() != TopAbs_COMPSOLID)
        )
        {
          foundValid = true;
          break;
        }
      }
      return !foundValid;
    }
    const TopoDS_Shape &shape;
    BRepCheck_Analyzer checker;
  };
}

using namespace ftr;

ShapeCheck::ShapeCheck(const TopoDS_Shape &shapeIn)
{
  try
  {
    //defaults to invalid.
    if (shapeIn.IsNull())
      return;
    
    shapeCheckPrivate = std::unique_ptr<ShapeCheckPrivate>(new ShapeCheckPrivate(shapeIn));
    if (!shapeCheckPrivate->checker.IsValid())
      return;
    
    //look for something 'concrete' in the shape.
    occt::ShapeVector ss = occt::mapShapes(shapeIn);
    bool foundValid = false;
    for (const auto &s : ss)
    {
      if
      (
        (s.ShapeType() != TopAbs_SHAPE)
        && (s.ShapeType() != TopAbs_COMPOUND)
        && (s.ShapeType() != TopAbs_COMPSOLID)
      )
      {
        foundValid = true;
        break;
      }
    }
    if (!foundValid)
      return;
    
    validity = true;
    
  }
  catch (const Standard_Failure &e)
  {
    std::cout << std::endl << "OCCT Exception in ShapeCheck: " << e.GetMessageString() << std::endl;
  }
  catch (...)
  {
    std::cout << std::endl << "Unknown error in ShapeCheck: " << std::endl;
  }
}

ShapeCheck::~ShapeCheck()
{

}

const BRepCheck_Analyzer& ShapeCheck::getChecker()
{
  return shapeCheckPrivate->checker;
}

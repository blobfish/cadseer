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

#ifndef FTR_SHAPECHECK_H
#define FTR_SHAPECHECK_H

#include <memory>

class TopoDS_Shape;
class BRepCheck_Analyzer;
namespace ftr
{
  class ShapeCheckPrivate;
  
  /*! @brief check the validity of an occt shape.
   * 
   * Wanted to abstract away the idea of what a valid shape
   * in cadseer is. Using regular check and bopargcheck and
   * possible more.
   */
  class ShapeCheck
  {
  public:
    ShapeCheck(const TopoDS_Shape&);
    ~ShapeCheck();
    bool isValid(){return validity;}
    const BRepCheck_Analyzer& getChecker();
  private:
    bool validity = false;
    std::unique_ptr<ShapeCheckPrivate> shapeCheckPrivate;
  };
}

#endif // FTR_SHAPECHECK_H

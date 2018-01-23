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

#ifndef FTR_BOOLEANOPERATION_H
#define FTR_BOOLEANOPERATION_H

#include <BRepAlgoAPI_BooleanOperation.hxx>

#include <tools/occtools.h>

class BOPAlgo_Builder;

namespace ftr
{
  /*! @brief Custom occt boolean operation
   * 
   * The following is the answer to 'Why make this class?'
   * At the time of developing id mapping for the booleans,
   * OCCTs(6.9.1) BRepAlgoAPI*: Generated, Modified and Deleted
   * functions are all but worthless. Not sure if there is a
   * regression in this version or if the code never worked or
   * just isn't finished. Anyway, this subclass allows us to access
   * the protected BOPAlgo_Builder object where it all happens.
   */
  class BooleanOperation : public BRepAlgoAPI_BooleanOperation
  {
  public:
    BooleanOperation(const TopoDS_Shape &, const TopoDS_Shape &, BOPAlgo_Operation);
    BooleanOperation(const TopoDS_Shape &, const occt::ShapeVector &, BOPAlgo_Operation);
    BooleanOperation(const occt::ShapeVector &, const occt::ShapeVector &, BOPAlgo_Operation);
    BOPAlgo_Builder& getBuilder();
  private:
    void init(const occt::ShapeVector &, const occt::ShapeVector &, BOPAlgo_Operation);
  };
}

#endif // FTR_BOOLEANOPERATION_H

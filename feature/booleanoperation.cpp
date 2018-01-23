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

#include <BOPAlgo_Builder.hxx>

#include "booleanoperation.h"

using namespace ftr;

BooleanOperation::BooleanOperation(const TopoDS_Shape &shape1, const TopoDS_Shape &shape2, BOPAlgo_Operation op):
  BRepAlgoAPI_BooleanOperation(shape1, shape2, op)
{
  //let caller call build outside of construction.
}

BooleanOperation::BooleanOperation(const TopoDS_Shape &target, const occt::ShapeVector &tools, BOPAlgo_Operation op):
  BRepAlgoAPI_BooleanOperation()
{
  occt::ShapeVector targets;
  targets.push_back(target);
  init(targets, tools, op);
}

BooleanOperation::BooleanOperation(const occt::ShapeVector &targets, const occt::ShapeVector &tools, BOPAlgo_Operation op)
{
  init(targets, tools, op);
}

void BooleanOperation::init(const occt::ShapeVector &targets, const occt::ShapeVector &tools, BOPAlgo_Operation op)
{
  TopTools_ListOfShape targetList;
  for (const auto &cTarget : targets)
    targetList.Append(cTarget);
  SetArguments(targetList);
  
  TopTools_ListOfShape toolList;
  for (const auto &cTool : tools)
    toolList.Append(cTool);
  SetTools(toolList);
  
  SetOperation(op);
  SetNonDestructive(Standard_True);
  SetRunParallel(Standard_True);
}

BOPAlgo_Builder& BooleanOperation::getBuilder()
{
  return *myBuilder;
}

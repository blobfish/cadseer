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

#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>

#include "blend.h"

using namespace Feature;

Blend::Blend() : Base(), radius(1.0)
{

}

void Blend::setRadius(const double& radiusIn)
{
  if (radius == radiusIn)
    return;
  assert(radiusIn > Precision::Confusion());
  setDirty();
  radius = radiusIn;
}

void Blend::setEdgeIds(const std::vector<boost::uuids::uuid>& edgeIdsIn)
{
  edgeIds = edgeIdsIn;
  setDirty();
}

void Blend::update(const UpdateMap& mapIn)
{
  if (mapIn.count(InputTypes::target) < 1)
    return; //much better error handeling.
    
  const TopoDS_Shape &targetShape = mapIn.at(InputTypes::target)->getShape();
    
  const ResultContainer& targetResultContainer = mapIn.at(InputTypes::target)->getResultContainer();
  
  try
  {
    BRepFilletAPI_MakeFillet filletMaker(targetShape);
    for (const auto &currentId : edgeIds)
    {
      TopoDS_Shape tempShape = findResultById(targetResultContainer, currentId).shape;
      assert(!tempShape.IsNull());
      assert(tempShape.ShapeType() == TopAbs_EDGE);
      filletMaker.Add(radius, TopoDS::Edge(tempShape));
    }
    filletMaker.Build();
    assert(filletMaker.IsDone());
    shape = filletMaker.Shape();
    setClean();
  }
  catch (Standard_Failure)
  {
    Handle_Standard_Failure e = Standard_Failure::Caught();
    std::cout << std::endl << "Error in cylinder update. " << e->GetMessageString() << std::endl;
  }
}
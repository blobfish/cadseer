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

#include <BRepPrimAPI_MakeBox.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopoDS.hxx>
#include <BRepTools.hxx>
#include <BOPTools_AlgoTools.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

#include "boxbuilder.h"

using namespace Feature;

/*
   * note every call to BRepPrimAPI_MakeBox::Solid() returns
   * a different TopoDS_Shape with a different hash and sub-shapes
   * are different true. Not true for Shape(). In short, stay away
   * from using ::Solid(). Shapetype returned from Shape() is solid.
   * I looked at the source and even though ::Shell doesn't appear
   * to be screwing up the shapes, I am suspicious.
  */

BoxBuilder::BoxBuilder(double lengthIn, double widthIn, double heightIn)
{
  BRepPrimAPI_MakeBox boxMaker(lengthIn, widthIn, heightIn);
  boxMaker.Build();
  assert(boxMaker.IsDone());
  solid = boxMaker.Shape();
  assert(solid.ShapeType() == TopAbs_SOLID);
  
  TopoDS_Iterator it(solid);
  shell = it.Value();
  assert(shell.ShapeType() == TopAbs_SHELL);
  
  faceXP = boxMaker.FrontFace();
  assert(faceXP.ShapeType() == TopAbs_FACE);
  faceXN = boxMaker.BackFace();
  assert(faceXN.ShapeType() == TopAbs_FACE);
  faceYP = boxMaker.RightFace();
  assert(faceYP.ShapeType() == TopAbs_FACE);
  faceYN = boxMaker.LeftFace();
  assert(faceYN.ShapeType() == TopAbs_FACE);
  faceZP = boxMaker.TopFace();
  assert(faceZP.ShapeType() == TopAbs_FACE);
  faceZN = boxMaker.BottomFace();
  assert(faceZN.ShapeType() == TopAbs_FACE);
  
  wireXP = BRepTools::OuterWire(TopoDS::Face(faceXP));
  assert(!wireXP.IsNull());
  wireXN = BRepTools::OuterWire(TopoDS::Face(faceXN));
  assert(!wireXN.IsNull());
  wireYP = BRepTools::OuterWire(TopoDS::Face(faceYP));
  assert(!wireYP.IsNull());
  wireYN = BRepTools::OuterWire(TopoDS::Face(faceYN));
  assert(!wireYN.IsNull());
  wireZP = BRepTools::OuterWire(TopoDS::Face(faceZP));
  assert(!wireZP.IsNull());
  wireZN = BRepTools::OuterWire(TopoDS::Face(faceZN));
  assert(!wireZN.IsNull());
  
  edgeXPYP = sharedEdge(faceXP, faceYP);
  edgeXPZP = sharedEdge(faceXP, faceZP);
  edgeXPYN = sharedEdge(faceXP, faceYN);
  edgeXPZN = sharedEdge(faceXP, faceZN);
  edgeXNYN = sharedEdge(faceXN, faceYN);
  edgeXNZP = sharedEdge(faceXN, faceZP);
  edgeXNYP = sharedEdge(faceXN, faceYP);
  edgeXNZN = sharedEdge(faceXN, faceZN);
  edgeYPZP = sharedEdge(faceYP, faceZP);
  edgeYPZN = sharedEdge(faceYP, faceZN);
  edgeYNZP = sharedEdge(faceYN, faceZP);
  edgeYNZN = sharedEdge(faceYN, faceZN);
  
  vertexXPYPZP = sharedVertex(faceXP, faceYP, faceZP);
  vertexXPYNZP = sharedVertex(faceXP, faceYN, faceZP);
  vertexXPYNZN = sharedVertex(faceXP, faceYN, faceZN);
  vertexXPYPZN = sharedVertex(faceXP, faceYP, faceZN);
  vertexXNYNZP = sharedVertex(faceXN, faceYN, faceZP);
  vertexXNYPZP = sharedVertex(faceXN, faceYP, faceZP);
  vertexXNYPZN = sharedVertex(faceXN, faceYP, faceZN);
  vertexXNYNZN = sharedVertex(faceXN, faceYN, faceZN);
}

TopoDS_Shape BoxBuilder::sharedEdge(const TopoDS_Shape& face1, const TopoDS_Shape& face2)
{
  TopoDS_Face testFace = TopoDS::Face(face2);
  TopoDS_Edge out;
  
  TopTools_IndexedMapOfShape shapeMap;
  TopExp::MapShapes(face1, TopAbs_EDGE, shapeMap);
  for (int index = 1; index <= shapeMap.Extent(); ++index)
  {
    const TopoDS_Edge &currentEdge = TopoDS::Edge(shapeMap(index));
    if (BOPTools_AlgoTools::GetEdgeOff(currentEdge, testFace, out))
      break;
  }
  
  assert(!out.IsNull());
  return out;
}

TopoDS_Shape BoxBuilder::sharedVertex(const TopoDS_Shape& face1, const TopoDS_Shape& face2, const TopoDS_Shape& face3)
{
  TopoDS_Shape out;
  
  TopTools_IndexedMapOfShape shapeMap1;
  TopExp::MapShapes(face1, TopAbs_VERTEX, shapeMap1);
  
  TopTools_IndexedMapOfShape shapeMap2;
  TopExp::MapShapes(face2, TopAbs_VERTEX, shapeMap2);
  
  TopTools_IndexedMapOfShape shapeMap3;
  TopExp::MapShapes(face3, TopAbs_VERTEX, shapeMap3);
  
  for (int index = 1; index <= shapeMap1.Extent(); ++index)
  {
    const TopoDS_Shape& current = shapeMap1(index);
    if (shapeMap2.Contains(current) && shapeMap3.Contains(current))
    {
      out = current;
      break;
    }
  }
  
  assert(!out.IsNull());
  return out;
}


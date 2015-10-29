/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2015  Thomas S. Anderson blobfish.at.gmx.com
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

#include <feature/boxbuilder.h>

using namespace ftr;

/*
   * note every call to BRepPrimAPI_MakeBox::Solid() returns
   * a different TopoDS_Shape with a different hash and sub-shapes
   * are different true. Not true for Shape(). In short, stay away
   * from using ::Solid(). Shapetype returned from Shape() is solid.
   * I looked at the source and even though ::Shell doesn't appear
   * to be screwing up the shapes, I am suspicious.
  */

BoxBuilder::BoxBuilder(double lengthIn, double widthIn, double heightIn, gp_Ax2 axis2)
{
  BRepPrimAPI_MakeBox boxMaker(axis2, lengthIn, widthIn, heightIn);
  boxMaker.Build();
  assert(boxMaker.IsDone());
  solid = boxMaker.Shape();
  assert(solid.ShapeType() == TopAbs_SOLID);
  
  BRepPrim_Wedge &boxSubMaker = boxMaker.Wedge();
  shell = boxSubMaker.Shell();
  
  //remove me
  TopoDS_Iterator it(solid);
  TopoDS_Shape testShell = it.Value();
  assert(testShell.ShapeType() == TopAbs_SHELL);
  assert(shell.IsEqual(testShell));
  
  faceXP = boxSubMaker.Face(BRepPrim_XMax);
  faceXN = boxSubMaker.Face(BRepPrim_XMin);
  faceYP = boxSubMaker.Face(BRepPrim_YMax);
  faceYN = boxSubMaker.Face(BRepPrim_YMin);
  faceZP = boxSubMaker.Face(BRepPrim_ZMax);
  faceZN = boxSubMaker.Face(BRepPrim_ZMin);
  
  wireXP = boxSubMaker.Wire(BRepPrim_XMax);
  wireXN = boxSubMaker.Wire(BRepPrim_XMin);
  wireYP = boxSubMaker.Wire(BRepPrim_YMax);
  wireYN = boxSubMaker.Wire(BRepPrim_YMin);
  wireZP = boxSubMaker.Wire(BRepPrim_ZMax);
  wireZN = boxSubMaker.Wire(BRepPrim_ZMin);
  
  edgeXPYP = boxSubMaker.Edge(BRepPrim_XMax, BRepPrim_YMax);
  edgeXPZP = boxSubMaker.Edge(BRepPrim_XMax, BRepPrim_ZMax);
  edgeXPYN = boxSubMaker.Edge(BRepPrim_XMax, BRepPrim_YMin);
  edgeXPZN = boxSubMaker.Edge(BRepPrim_XMax, BRepPrim_ZMin);
  edgeXNYN = boxSubMaker.Edge(BRepPrim_XMin, BRepPrim_YMin);
  edgeXNZP = boxSubMaker.Edge(BRepPrim_XMin, BRepPrim_ZMax);
  edgeXNYP = boxSubMaker.Edge(BRepPrim_XMin, BRepPrim_YMax);
  edgeXNZN = boxSubMaker.Edge(BRepPrim_XMin, BRepPrim_ZMin);
  edgeYPZP = boxSubMaker.Edge(BRepPrim_YMax, BRepPrim_ZMax);
  edgeYPZN = boxSubMaker.Edge(BRepPrim_YMax, BRepPrim_ZMin);
  edgeYNZP = boxSubMaker.Edge(BRepPrim_YMin, BRepPrim_ZMax);
  edgeYNZN = boxSubMaker.Edge(BRepPrim_YMin, BRepPrim_ZMin);
  
  vertexXPYPZP = boxSubMaker.Vertex(BRepPrim_XMax, BRepPrim_YMax, BRepPrim_ZMax);
  vertexXPYNZP = boxSubMaker.Vertex(BRepPrim_XMax, BRepPrim_YMin, BRepPrim_ZMax);
  vertexXPYNZN = boxSubMaker.Vertex(BRepPrim_XMax, BRepPrim_YMin, BRepPrim_ZMin);
  vertexXPYPZN = boxSubMaker.Vertex(BRepPrim_XMax, BRepPrim_YMax, BRepPrim_ZMin);
  vertexXNYNZP = boxSubMaker.Vertex(BRepPrim_XMin, BRepPrim_YMin, BRepPrim_ZMax);
  vertexXNYPZP = boxSubMaker.Vertex(BRepPrim_XMin, BRepPrim_YMax, BRepPrim_ZMax);
  vertexXNYPZN = boxSubMaker.Vertex(BRepPrim_XMin, BRepPrim_YMax, BRepPrim_ZMin);
  vertexXNYNZN = boxSubMaker.Vertex(BRepPrim_XMin, BRepPrim_YMin, BRepPrim_ZMin);
}

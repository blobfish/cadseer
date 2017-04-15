/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#include <cassert>

#include <gp_Ax3.hxx>
#include <gp_Circ.hxx>
#include <gp_Pln.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Face.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <TopExp_Explorer.hxx>
#include <BRepTools.hxx>

#include <globalutilities.h>
#include <feature/oblongbuilder.h>

using namespace ftr;

OblongBuilder::OblongBuilder(double lengthIn, double widthIn, double heightIn, gp_Ax2 axis2)
{
  assert(lengthIn > widthIn);
  
  double radius = widthIn / 2.0;
  gp_Trsf transformation; transformation.SetTransformation(gp_Ax3(axis2));
  transformation.Invert();
  
  //building the base face.
  //build four vertices of base face. starting with lower left and going clockwise as
  //this should reflect the extruded solids orientation
  gp_Pnt point1(radius, 0.0, 0.0); point1.Transform(transformation);
  gp_Pnt point2(radius, widthIn, 0.0); point2.Transform(transformation);
  gp_Pnt point3(lengthIn - radius, widthIn, 0.0); point3.Transform(transformation);
  gp_Pnt point4(lengthIn - radius, 0.0, 0.0); point4.Transform(transformation);
  TopoDS_Vertex vertex1 = BRepBuilderAPI_MakeVertex(point1);
  TopoDS_Vertex vertex2 = BRepBuilderAPI_MakeVertex(point2);
  TopoDS_Vertex vertex3 = BRepBuilderAPI_MakeVertex(point3);
  TopoDS_Vertex vertex4 = BRepBuilderAPI_MakeVertex(point4);
  
  //linear edges
  TopoDS_Edge bEdge = BRepBuilderAPI_MakeEdge(vertex4, vertex1);
  TopoDS_Edge tEdge = BRepBuilderAPI_MakeEdge(vertex2, vertex3);
  
  //circular edges.
  //left edge
  gp_Pnt lCenter(radius, radius, 0.0); lCenter.Transform(transformation);
  gp_Ax2 lAxis(lCenter, -axis2.Direction(), axis2.XDirection());
  gp_Circ lCircle(lAxis, radius);
  TopoDS_Edge lEdge = BRepBuilderAPI_MakeEdge(lCircle, vertex1, vertex2);
  //right edge
  gp_Pnt rCenter(lengthIn - radius, radius, 0.0); rCenter.Transform(transformation);
  gp_Ax2 rAxis(rCenter, -axis2.Direction(), -axis2.XDirection());
  gp_Circ rCircle(rAxis, radius);
  TopoDS_Edge rEdge = BRepBuilderAPI_MakeEdge(rCircle, vertex3, vertex4);
  
  //make bottom face wire.
  TopoDS_Wire bWire = BRepBuilderAPI_MakeWire(lEdge, tEdge, rEdge, bEdge);
  
  //make bottom face.
  gp_Ax3 fAxis(axis2.Location(), -axis2.Direction(), axis2.XDirection());
  gp_Pln fSurface(fAxis);
  TopoDS_Face bFace = BRepBuilderAPI_MakeFace(fSurface, bWire);
  
  //extrude bottom face
  gp_Vec eVector(axis2.Direction());
  eVector *= heightIn;
  BRepPrimAPI_MakePrism extruder(bFace, eVector);
  solid = extruder;
  
  //add lables.
  assert(solid.ShapeType() == TopAbs_SOLID);
  
  TopExp_Explorer exp(solid, TopAbs_SHELL);
  assert(exp.More());
  shell = exp.Current();
  
  faceZN = bFace;
  faceXN = extruder.Generated(lEdge).First();
  faceYP = extruder.Generated(tEdge).First();
  faceXP = extruder.Generated(rEdge).First();
  faceYN = extruder.Generated(bEdge).First();
  faceZP = extruder.LastShape();
  
  wireZN = bWire;
  wireXN = BRepTools::OuterWire(TopoDS::Face(faceXN));
  wireYP = BRepTools::OuterWire(TopoDS::Face(faceYP));
  wireXP = BRepTools::OuterWire(TopoDS::Face(faceXP));
  wireYN = BRepTools::OuterWire(TopoDS::Face(faceYN));
  wireZP = extruder.LastShape(bWire);
  
  edgeXNZN = lEdge;
  edgeYPZN = tEdge;
  edgeXPZN = rEdge;
  edgeYNZN = bEdge;
  edgeXNYN = extruder.Generated(vertex1).First();
  edgeXNYP = extruder.Generated(vertex2).First();
  edgeXPYP = extruder.Generated(vertex3).First();
  edgeXPYN = extruder.Generated(vertex4).First();
  edgeXNZP = extruder.LastShape(lEdge);
  edgeYPZP = extruder.LastShape(tEdge);
  edgeXPZP = extruder.LastShape(rEdge);
  edgeYNZP = extruder.LastShape(bEdge);
  
  vertexXNYNZN = vertex1;
  vertexXNYPZN = vertex2;
  vertexXPYPZN = vertex3;
  vertexXPYNZN = vertex4;
  vertexXNYNZP = extruder.LastShape(vertex1);
  vertexXNYPZP = extruder.LastShape(vertex2);
  vertexXPYPZP = extruder.LastShape(vertex3);
  vertexXPYNZP = extruder.LastShape(vertex4);
  
  
  //explore generated.
//   auto dumpList = [](const TopTools_ListOfShape &listIn)
//   {
//     for (const auto &shape : listIn)
//       std::cout << gu::getShapeTypeString(shape) << "    ";
//   };
//   
//   std::cout << std::endl << std::endl << "begin dump:" << std::endl;
//   std::cout << "generated from bottom face: "; dumpList(extruder.Generated(bFace)); std::cout << std::endl;
//   std::cout << "generated from vertex1: "; dumpList(extruder.Generated(vertex1)); std::cout << std::endl;
//   std::cout << "generated from lEdge: "; dumpList(extruder.Generated(lEdge)); std::cout << std::endl;
//   std::cout << "last shape generated from lEdge: " << gu::getShapeTypeString(extruder.LastShape(lEdge)) << std::endl;
//   std::cout << "last shape generated from vertex1: " << gu::getShapeTypeString(extruder.LastShape(vertex1)) << std::endl;
//   std::cout << "last shape generated from bWire: " << gu::getShapeTypeString(extruder.LastShape(bWire)) << std::endl;
  
  
}

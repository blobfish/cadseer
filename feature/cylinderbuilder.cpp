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

#include <BRepPrimAPI_MakeCylinder.hxx>
#include <TopoDS_Iterator.hxx>

#include <feature/cylinderbuilder.h>

using namespace ftr;

CylinderBuilder::CylinderBuilder(const double& radiusIn, const double& heightIn, gp_Ax2 axis2)
{
  BRepPrimAPI_MakeCylinder cylinderMaker(axis2, radiusIn, heightIn);
  cylinderMaker.Build();
  assert(cylinderMaker.IsDone());
  
  solid = cylinderMaker.Shape();
  
  //I have proven that the shell belonging to solid
  //is the same shell as what is returned by BRepPrim_Cylinder.
  BRepPrim_Cylinder &cylinderSubMaker = cylinderMaker.Cylinder();
  shell = cylinderSubMaker.Shell();
  
  faceBottom = cylinderSubMaker.BottomFace();
  faceCylindrical = cylinderSubMaker.LateralFace();
  faceTop = cylinderSubMaker.TopFace();
  
  wireBottom = cylinderSubMaker.BottomWire();
  wireCylindrical = cylinderSubMaker.LateralWire();
  wireTop = cylinderSubMaker.TopWire();
  
  edgeBottom = cylinderSubMaker.BottomEdge();
  edgeCylindrical = cylinderSubMaker.StartEdge();
  edgeTop = cylinderSubMaker.TopEdge();
  
  vertexBottom = cylinderSubMaker.BottomStartVertex();
  vertexTop = cylinderSubMaker.TopStartVertex();
}

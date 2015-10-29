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

#include <BRepPrimAPI_MakeCone.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopoDS.hxx>
#include <BRepTools.hxx>
#include <BOPTools_AlgoTools.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <Geom_Surface.hxx>
#include <GeomAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <ShapeAnalysis_Edge.hxx>
#include <gp_Circ.hxx>

#include <feature/conebuilder.h>

using namespace ftr;

ConeBuilder::ConeBuilder(const double& radius1, const double& radius2, const double& height, gp_Ax2 axis2)
{
  BRepPrimAPI_MakeCone coneMaker(axis2, radius1, radius2, height);
  coneMaker.Build();
  assert(coneMaker.IsDone());
  
  solid = coneMaker.Shape();
  
  BRepPrim_Cone &coneSubMaker = coneMaker.Cone();
  
  shell = coneSubMaker.Shell();
  
  //will throw exception if doesn't exist.
  if (coneSubMaker.HasBottom())
  {
    faceBottom = coneSubMaker.BottomFace();
    wireBottom = coneSubMaker.BottomWire();
    edgeBottom = coneSubMaker.BottomEdge();
  }
  
  faceConical = coneSubMaker.LateralFace();
  wireConical = coneSubMaker.LateralWire();
  edgeConical = coneSubMaker.StartEdge(); //StartEdge and EndEdge are seam.
  
  if (coneSubMaker.HasTop())
  {
    faceTop = coneSubMaker.TopFace();
    wireTop = coneSubMaker.TopWire();
    edgeTop = coneSubMaker.TopEdge();
  }
  
  vertexBottom = coneSubMaker.BottomStartVertex();
  vertexTop = coneSubMaker.TopStartVertex();
}

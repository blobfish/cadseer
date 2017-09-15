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

#include <memory>
#include <cassert>
#include <functional>
#include <cstddef> //null error from nglib

#define OCCGEOMETRY
namespace nglib //what the fuck is this nonsense!
{
#include <nglib.h>
}
using namespace nglib;

#include "netgen.h"


typedef std::unique_ptr<Ng_OCC_Geometry, std::function<Ng_Result(Ng_OCC_Geometry*)> > OccGeomPtr;
typedef std::unique_ptr<Ng_Mesh, std::function<void(Ng_Mesh *)> > NgMeshPtr;

struct NgManager
{
  NgManager(){Ng_Init();};
  ~NgManager(){Ng_Exit();};
};
typedef std::unique_ptr<NgManager> NgManagerPtr;

Mesh occCommon(Ng_OCC_Geometry* gIn, const sqs::ntg::Parameters &pIn)
{
  NgMeshPtr ngMeshPtr(Ng_NewMesh(), std::bind(&Ng_DeleteMesh, std::placeholders::_1));
  assert(ngMeshPtr);
  
  Ng_Meshing_Parameters mp;
  mp.maxh = pIn.maxh;
  mp.minh = pIn.minh;
//   mp.closeedgeenable = 1;
//   mp.grading = 0.9;
  
  try
  {
    Ng_Result r;
    r = Ng_OCC_SetLocalMeshSize(gIn, ngMeshPtr.get(), &mp);
    if (r != NG_OK)
      std::cout << "error in Ng_OCC_SetLocalMeshSize. code: " << r << std::endl;
    r = Ng_OCC_GenerateEdgeMesh(gIn, ngMeshPtr.get(), &mp);
    if (r != NG_OK)
      std::cout << "error in Ng_OCC_GenerateEdgeMesh. code: " << r << std::endl;
    r = Ng_OCC_GenerateSurfaceMesh(gIn, ngMeshPtr.get(), &mp);
    if (r != NG_OK)
      std::cout << "error in Ng_OCC_GenerateSurfaceMesh. code: " << r << std::endl;
    
  //   Ng_OCC_Uniform_Refinement (gIn, ngMeshPtr.get());//refinement
  }
  catch (...)
  {
    //netgen is throwing an exception about faces not meshed.
    //I would like to warn the user, but try and use what has been meshed.
    std::cout << "Unknown exception in netgen.cpp:occCommon: " << std::endl;
  }
  
  
  Mesh out;
  
  //apparently netgen is using 1 based arrays.
  int pointCount = Ng_GetNP(ngMeshPtr.get());
  for (int i = 0; i < pointCount; ++i)
  {
    double coord[3];
    Ng_GetPoint(ngMeshPtr.get(), i + 1, coord);
    out.add_vertex(Point(coord[0], coord[1], coord[2]));
  }
  
  int faceCount = Ng_GetNSE(ngMeshPtr.get());
  for (int i = 0; i < faceCount; ++i)
  {
    Ng_Surface_Element_Type type;
    int pIndex[8];
    type = Ng_GetSurfaceElement (ngMeshPtr.get(), i + 1, pIndex);
    assert(type == NG_TRIG);
    if (type != NG_TRIG)
      throw std::runtime_error("wrong element type in netgen read");
    
    out.add_face(static_cast<Vertex>(pIndex[0] - 1), static_cast<Vertex>(pIndex[1] - 1), static_cast<Vertex>(pIndex[2] - 1));
  }
  
  return out;
}

Mesh sqs::ntg::readBrepFile(const std::string &fn, const sqs::ntg::Parameters &pIn)
{
  NgManagerPtr manager(new NgManager());
  OccGeomPtr gPtr(Ng_OCC_Load_BREP(fn.c_str()), std::bind(&Ng_OCC_DeleteGeometry, std::placeholders::_1));
  if (!gPtr)
    return Mesh(); //return empty mesh
  
  return occCommon(gPtr.get(), pIn);
}

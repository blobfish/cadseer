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


#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <BRep_Tool.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>

#include <tools/idtools.h>
#include <globalutilities.h>
#include <project/serial/xsdcxxoutput/featurebase.h>
#include <feature/pick.h>

using namespace ftr;
using boost::uuids::uuid;


Pick::Pick() : id(gu::createNilId()), u(0.0), v(0.0){}

Pick::Pick(const uuid &idIn, double uIn, double vIn) :
  id(idIn), u(uIn), v(vIn){}

prj::srl::Pick Pick::serialOut() const
{
  return prj::srl::Pick
  (
    gu::idToString(id),
    u,
    v
  );
}

void Pick::serialIn(const prj::srl::Pick &sPickIn)
{
  id = gu::stringToId(sPickIn.id());
  u = sPickIn.u();
  v = sPickIn.v();
}

bool Pick::operator==(const Pick &rhs) const
{
  return id == rhs.id;
}

/*! Set u parameter
 * @param edgeIn edge to evaluate.
 * @param pointIn closest point.
 * @see Pick::parameter
 */
void Pick::setParameter(const TopoDS_Edge &edgeIn, const osg::Vec3d &pointIn)
{
  u = Pick::parameter(edgeIn, pointIn);
}

/*! Set u and v parameters
 * @param faceIn face to evaluate.
 * @param pointIn closest point.
 * @see Pick::parameter
 */
void Pick::setParameter(const TopoDS_Face &faceIn, const osg::Vec3d &pointIn)
{
  std::tie(u, v) = Pick::parameter(faceIn, pointIn);
}

/*! Calculate point from u parameter
 * @param edgeIn edge to evaluate.
 * @return point on edge at u.
 * @see Pick::point
 */
osg::Vec3d Pick::getPoint(const TopoDS_Edge &edgeIn) const
{
  return Pick::point(edgeIn, u);
}

/*! Calculate point from parameters
 * @param faceIn face to evaluate.
 * @return point on face at u,v
 * @see Pick::point
 */
osg::Vec3d Pick::getPoint(const TopoDS_Face &faceIn) const
{
  return Pick::point(faceIn, u, v);
}

/*! Calculate closest parameter
 * @param edgeIn edge to evaluate.
 * @param pointIn closest point.
 * @return normalized parameter in range [0,1]
 */
double Pick::parameter(const TopoDS_Edge &edgeIn, const osg::Vec3d &pointIn)
{
  TopoDS_Shape aVertex = BRepBuilderAPI_MakeVertex(gp_Pnt(pointIn.x(), pointIn.y(), pointIn.z())).Vertex();
  BRepExtrema_DistShapeShape dist(edgeIn, aVertex);
  if (dist.NbSolution() < 1)
  {
    std::cout << "Warning: no solution for distshapeshape in Pick::Parameter(Edge)" << std::endl;
    return 0.0;
  }
  if (dist.SupportTypeShape1(1) != BRepExtrema_IsOnEdge)
  {
    std::cout << "Warning: wrong support type in Pick::Parameter(edge)" << std::endl;
    return 0.0;
  }
  
  double closestParameter;
  dist.ParOnEdgeS1(1, closestParameter);
  
  double firstP, lastP;
  BRep_Tool::Curve(edgeIn, firstP, lastP);
  //normalize parameter to 0 to 1.
  return (closestParameter - firstP) / (lastP - firstP);
}

/*! Calculate closest parameters
 * @param faceIn face to evaluate.
 * @param pointIn closest point.
 * @return u, v parameters
 */
std::pair<double, double> Pick::parameter(const TopoDS_Face &faceIn, const osg::Vec3d &pointIn)
{
  std::pair<double, double> out;
  out.first = 0.0;
  out.second = 0.0;
  
  TopoDS_Shape aVertex = BRepBuilderAPI_MakeVertex(gp_Pnt(pointIn.x(), pointIn.y(), pointIn.z())).Vertex();
  BRepExtrema_DistShapeShape dist(faceIn, aVertex);
  if (dist.NbSolution() < 1)
  {
    std::cout << "Warning: no solution for distshapeshape in Pick::Parameter(Face)" << std::endl;
    return out;
  }
  if (dist.SupportTypeShape1(1) != BRepExtrema_IsInFace)
  {
    std::cout << "Warning: wrong support type in Pick::Parameter(Face)" << std::endl;
    return out;
  }
  dist.ParOnFaceS1(1, out.first, out.second);
  
  return out;
}

/*! Calculate point from parameter
 * @param edgeIn edge to evaluate.
 * @param uIn parameter.
 * @return point on edge.
 */
osg::Vec3d Pick::point(const TopoDS_Edge &edgeIn, double uIn)
{
  double firstP, lastP;
  const Handle_Geom_Curve &curve = BRep_Tool::Curve(edgeIn, firstP, lastP);
  double parameterOut = uIn * (lastP - firstP) + firstP;
  gp_Pnt occPoint;
  curve->D0(parameterOut, occPoint);
  return gu::toOsg(occPoint);
}

/*! Calculate point from parameters
 * @param faceIn face to evaluate.
 * @param uIn parameter.
 * @param vIn parameter.
 * @return point on face.
 */
osg::Vec3d Pick::point(const TopoDS_Face &faceIn, double uIn, double vIn)
{
  Handle_Geom_Surface surface = BRep_Tool::Surface(faceIn);
  gp_Pnt occPoint;
  surface->D0(uIn, vIn, occPoint);
  return gu::toOsg(occPoint);
}


prj::srl::Picks ftr::serialOut(const Picks &picksIn)
{
  prj::srl::Picks out;
  for (const auto &pick : picksIn)
    out.array().push_back(pick.serialOut());
  
  return out;
}

Picks ftr::serialIn(const prj::srl::Picks &sPicksIn)
{
  Picks out;
  for (const auto &sPick : sPicksIn.array())
  {
    Pick temp;
    temp.serialIn(sPick);
    out.push_back(temp);
  }
  
  return out;
}


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
#include <boost/math/constants/constants.hpp>

#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <TopTools_MapIteratorOfMapOfShape.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopoDS.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <GeomLib_Tool.hxx>
#include <ShapeAnalysis_FreeBoundsProperties.hxx>

#include <tools/occtools.h>

static const double pi = boost::math::constants::pi<double>();

using namespace occt;

ShapeVectorCast::ShapeVectorCast(const ShapeVector& shapeVectorIn)
{
  std::copy(shapeVectorIn.begin(), shapeVectorIn.end(), std::back_inserter(shapeVector));
}

ShapeVectorCast::ShapeVectorCast(const TopoDS_Compound& compoundIn)
{
  for (TopoDS_Iterator it(compoundIn); it.More(); it.Next())
    shapeVector.push_back(it.Value());
  //topods_iterator will revisit shapes, so uniquefy the vector
  occt::uniquefy(shapeVector);
}

ShapeVectorCast::ShapeVectorCast(const SolidVector& solidVectorIn)
{
  std::copy(solidVectorIn.begin(), solidVectorIn.end(), std::back_inserter(shapeVector));
}

ShapeVectorCast::ShapeVectorCast(const ShellVector& shellVectorIn)
{
  std::copy(shellVectorIn.begin(), shellVectorIn.end(), std::back_inserter(shapeVector));
}

ShapeVectorCast::ShapeVectorCast(const FaceVector& faceVectorIn)
{
  std::copy(faceVectorIn.begin(), faceVectorIn.end(), std::back_inserter(shapeVector));
}

ShapeVectorCast::ShapeVectorCast(const WireVector& wireVectorIn)
{
  std::copy(wireVectorIn.begin(), wireVectorIn.end(), std::back_inserter(shapeVector));
}

ShapeVectorCast::ShapeVectorCast(const EdgeVector& edgeVectorIn)
{
  std::copy(edgeVectorIn.begin(), edgeVectorIn.end(), std::back_inserter(shapeVector));
}

ShapeVectorCast::ShapeVectorCast(const VertexVector& vertexVectorIn)
{
  std::copy(vertexVectorIn.begin(), vertexVectorIn.end(), std::back_inserter(shapeVector));
}

ShapeVectorCast::ShapeVectorCast(const TopTools_MapOfShape& mapIn)
{
  TopTools_MapIteratorOfMapOfShape it;
  for (it.Initialize(mapIn); it.More(); it.Next())
    shapeVector.push_back(it.Key());
}

ShapeVectorCast::ShapeVectorCast(const TopTools_IndexedMapOfShape &mapIn)
{
  for (int i = 1; i <= mapIn.Extent(); ++i)
    shapeVector.push_back(mapIn(i));
}

ShapeVectorCast::operator ShapeVector () const
{
  return shapeVector;
}

ShapeVectorCast::operator TopoDS_Compound () const
{
  BRep_Builder builder;
  TopoDS_Compound out;
  builder.MakeCompound(out);
  for (const auto &currentShape : shapeVector)
    builder.Add(out, currentShape);
  return out;
}

ShapeVectorCast::operator SolidVector () const
{
  SolidVector out;
  for (const auto &currentShape : shapeVector)
  {
    if (currentShape.ShapeType() == TopAbs_SOLID)
      out.push_back(TopoDS::Solid(currentShape));
  }
  return out;
}

ShapeVectorCast::operator ShellVector () const
{
  ShellVector out;
  for (const auto &currentShape : shapeVector)
  {
    if (currentShape.ShapeType() == TopAbs_SHELL)
      out.push_back(TopoDS::Shell(currentShape));
  }
  return out;
}

ShapeVectorCast::operator FaceVector () const
{
  FaceVector out;
  for (const auto &currentShape : shapeVector)
  {
    if (currentShape.ShapeType() == TopAbs_FACE)
      out.push_back(TopoDS::Face(currentShape));
  }
  return out;
}

ShapeVectorCast::operator WireVector () const
{
  WireVector out;
  for (const auto &currentShape : shapeVector)
  {
    if (currentShape.ShapeType() == TopAbs_WIRE)
      out.push_back(TopoDS::Wire(currentShape));
  }
  return out;
}

ShapeVectorCast::operator EdgeVector () const
{
  EdgeVector out;
  for (const auto &currentShape : shapeVector)
  {
    if (currentShape.ShapeType() == TopAbs_EDGE)
      out.push_back(TopoDS::Edge(currentShape));
  }
  return out;
}

ShapeVectorCast::operator VertexVector () const
{
  VertexVector out;
  for (const auto &currentShape : shapeVector)
  {
    if (currentShape.ShapeType() == TopAbs_VERTEX)
      out.push_back(TopoDS::Vertex(currentShape));
  }
  return out;
}

/*! @brief BFS of connected faces in a shell.
  * 
  */
template<typename T> class FaceWalker
{
public:
  FaceWalker(T &visitorIn) : visitor(visitorIn)
  {
    TopExp::MapShapesAndAncestors(visitor.getWalkShell(), TopAbs_EDGE, TopAbs_FACE, edgeToFaceMap);
    TopoDS_Face startFace = visitor.getStartFace();
    stack.push(startFace);
    discoveredFaces.Add(startFace);
    visitor.discoverFace(startFace);
    
    while(!stack.empty())
    {
      TopoDS_Face currentFace = stack.top();
      stack.pop();
      TopExp_Explorer hunter;
      for (hunter.Init(currentFace, TopAbs_EDGE); hunter.More(); hunter.Next())
      {
        if (discoveredEdges.Contains(hunter.Current()))
          continue;
        discoveredEdges.Add(hunter.Current());
        if (!visitor.discoverEdge(TopoDS::Edge(hunter.Current())))
          continue;
        
        assert(edgeToFaceMap.Contains(hunter.Current())); //maybe face is not part of walk shell?
        const TopTools_ListOfShape &faceList = edgeToFaceMap.FindFromKey(hunter.Current());
        TopTools_ListIteratorOfListOfShape listIt;
        for (listIt.Initialize(faceList); listIt.More(); listIt.Next())
        {
          if (discoveredFaces.Contains(listIt.Value()))
            continue;
          discoveredFaces.Add(listIt.Value());
          if (!visitor.discoverFace(TopoDS::Face(listIt.Value())))
            continue;
          stack.push(TopoDS::Face(listIt.Value()));
        }
      }
    }
  }
  
private:
  T &visitor;
  TopTools_MapOfShape discoveredEdges;
  TopTools_MapOfShape discoveredFaces;
  TopTools_IndexedDataMapOfShapeListOfShape edgeToFaceMap;
  typedef std::stack<TopoDS_Face, std::deque<TopoDS_Face, Standard_StdAllocator<TopoDS_Face> > > FaceStack;
  FaceStack stack;
};

/*! @brief accumulate tangent faces.
* 
* used with faceWalker
*/
class TangentFaceVisitor
{
public:
  TangentFaceVisitor() = delete;
  TangentFaceVisitor
  (
    const TopoDS_Shell &shellIn,
    const TopoDS_Face &startFaceIn,
    double tIn
  );
  
  // satisfy FaceWalker
  TopoDS_Shell getWalkShell(){return walkShell;}
  TopoDS_Face getStartFace(){return startFace;}
  bool discoverEdge(const TopoDS_Edge &edgeIn);
  bool discoverFace(const TopoDS_Face &faceIn);
  // end satisfy face walker.
  
  FaceVector getResults(){return results;}
private:
  TopoDS_Shell walkShell;
  TopoDS_Face startFace;
  double t; //!< angular tolerance
  TopTools_IndexedDataMapOfShapeListOfShape edgeToFaceMap;
  FaceVector results;
};

TangentFaceVisitor::TangentFaceVisitor(const TopoDS_Shell &shellIn, const TopoDS_Face &startFaceIn, double tIn)
    : walkShell(shellIn), startFace(startFaceIn), t(tIn)
{
  TopExp::MapShapesAndAncestors(walkShell, TopAbs_EDGE, TopAbs_FACE, edgeToFaceMap);
}

bool TangentFaceVisitor::discoverFace(const TopoDS_Face& faceIn)
{
  results.push_back(faceIn);
  return true;
}

bool TangentFaceVisitor::discoverEdge(const TopoDS_Edge& edgeIn)
{
  if (!edgeToFaceMap.Contains(edgeIn))
    return false;
  const TopTools_ListOfShape &faces = edgeToFaceMap.FindFromKey(edgeIn);
  if (faces.Extent() != 2)
    return false;
  
  TopoDS_Face face1 = TopoDS::Face(faces.First());
  TopoDS_Face face2 = TopoDS::Face(faces.Last());
  
  //we know these have continuity through the common edge.
  GeomAbs_Shape continuity = BRep_Tool::Continuity(edgeIn, face1, face2);
  if (continuity >= GeomAbs_G1)
    return true;
  
  if (t == 0.0)
    return false;
  double ta = t * pi / 180.0; // tolerance angle, convert to radians
  
  //get a set of points along edge. copy the shape so we don't mess with the mesh.
  TopoDS_Shape c = BRepBuilderAPI_Copy(edgeIn);
  BRepMesh_FastDiscret::Parameters mp;
  mp.Deflection = 0.5;
  mp.Angle = 1.0;
  mp.Relative = Standard_True;
  BRepMesh_IncrementalMesh mesh(c, mp);
  mesh.Perform();
  assert(mesh.IsDone());
  
  TopLoc_Location location;
  const opencascade::handle<Poly_Polygon3D>& poly = BRep_Tool::Polygon3D(TopoDS::Edge(c), location);
  if (poly.IsNull())
    return false;
  
  gp_Trsf transformation;
  bool identity = true;
  if(!location.IsIdentity())
  {
    identity = false;
    transformation = location.Transformation();
  }
  
  auto getNormal = [&](const BRepAdaptor_Surface& sa, const gp_Pnt &p) -> gp_Vec
  {
    double tol = BRep_Tool::Tolerance(edgeIn) * 2.0 + BRep_Tool::Tolerance(sa.Face());
    double u, v;
    if (!GeomLib_Tool::Parameters(sa.Surface().Surface(), p, tol, u, v))
      throw std::runtime_error("point on surface out of tolerance");
    gp_Pnt pp; //projected point
    gp_Vec d1, d2; //derivatives
    sa.D1(u, v, pp, d1, d2);
    gp_Vec n = d1.Crossed(d2);
    
    if (sa.Face().Orientation() == TopAbs_REVERSED)
      n = -n;
    return n;
  };
  
  BRepAdaptor_Surface sa1(face1);
  BRepAdaptor_Surface sa2(face2);
    
  const TColgp_Array1OfPnt& nodes = poly->Nodes();
  bool anyFound = false;
  for (auto point : nodes)
  {
    if (!identity)
      point.Transform(transformation);
    try
    {
      gp_Vec d1 = getNormal(sa1, point);
      gp_Vec d2 = getNormal(sa2, point);
      double angle = d1.Angle(d2);
      if (angle > ta)
        return false;
      anyFound = true;
    }
    catch(std::runtime_error &){}
  }
  if (!anyFound)
    return false;
  
  return true;
}

FaceVector occt::walkTangentFaces(const TopoDS_Shape& parentIn, const TopoDS_Face& faceIn, double t)
{
  TopTools_IndexedDataMapOfShapeListOfShape faceToShellMap;
  TopExp::MapShapesAndAncestors(parentIn, TopAbs_FACE, TopAbs_SHELL, faceToShellMap);
  if (!faceToShellMap.Contains(faceIn))
    return FaceVector();
  const TopTools_ListOfShape &mappedShapes = faceToShellMap.FindFromKey(faceIn);
  if (mappedShapes.Extent() != 1)
    return FaceVector();
  TopoDS_Shell shell = TopoDS::Shell(mappedShapes.First());
  
  TangentFaceVisitor visitor(shell, faceIn, std::max(t, 0.0));
  FaceWalker<TangentFaceVisitor> walker(visitor);
  
  //temp for now.
  return visitor.getResults();
}

TopoDS_Shape occt::getFirstNonCompound(const TopoDS_Shape &shapeIn)
{
  assert(!shapeIn.IsNull());
  if (shapeIn.ShapeType() > TopAbs_COMPSOLID)
    return shapeIn;
  if (shapeIn.ShapeType() == TopAbs_COMPSOLID)
    return TopoDS_Shape();
  //only thing left is a compound.
  TopoDS_Compound workCompound = occt::getLastUniqueCompound(TopoDS::Compound(shapeIn));
  if (workCompound.IsNull())
    return TopoDS_Shape();
  for (TopoDS_Iterator it(workCompound); it.More(); it.Next())
  {
    if (it.Value().ShapeType() > TopAbs_COMPSOLID)
      return it.Value();
  }
  return TopoDS_Shape();
}

TopoDS_Compound occt::getLastUniqueCompound(const TopoDS_Compound& compoundIn)
{
  /*This has passed some basic tests.
   * 
   * This is basically a DFS. Will walk a tree of compounds, but
   * only returns the first instance of a unique compound.
   * A unique compound is a compound that contains something
   * other than just compounds.
   */
  
  assert(!compoundIn.IsNull());
  typedef std::pair<TopoDS_Iterator, TopoDS_Compound> ItCompPair;
  std::stack<ItCompPair> theStack;
  theStack.push(std::make_pair(TopoDS_Iterator(compoundIn), compoundIn));
  while(!theStack.empty())
  {
    ItCompPair currentPair = theStack.top();
    theStack.pop();
    TopoDS_Iterator currentIt = currentPair.first;
    const TopoDS_Compound &currentCompound = currentPair.second;
    for (; currentIt.More(); currentIt.Next())
    {
      if (currentIt.Value().ShapeType() == TopAbs_COMPSOLID)
        continue;
      if (currentIt.Value().ShapeType() != TopAbs_COMPOUND)
        return currentCompound;
      //we have to manually kick this to next position to avoid repeat processing.
      //because of breaking out of loop.
      if (currentPair.first.More())
        currentPair.first.Next();
      theStack.push(currentPair);
      theStack.push(std::make_pair(TopoDS_Iterator(currentIt.Value()), TopoDS::Compound(currentIt.Value())));
      break;
    }
  }
  return TopoDS_Compound();
}

WireVector occt::getBoundaryWires(const TopoDS_Shape &shapeIn)
{
  WireVector out;
  
  ShapeAnalysis_FreeBoundsProperties freeCheck(shapeIn);
  freeCheck.Perform();
  for (int i = 1; i <= freeCheck.NbClosedFreeBounds(); ++i)
  {
    Handle_ShapeAnalysis_FreeBoundData boundData = freeCheck.ClosedFreeBound(1);
    out.push_back(boundData->FreeBound());
  }

  return out;
}

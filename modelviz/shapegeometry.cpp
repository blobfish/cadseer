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

#include <BRepBndLib.hxx>
#include <TopExp.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepTools.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Iterator.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_Polygon3D.hxx>
#include <Poly_PolygonOnTriangulation.hxx>

#include <osg/Switch>
#include <osg/Depth>
#include <osg/LineWidth>
#include <osg/Point>

#include <feature/seershape.h>
#include <modelviz/shapegeometryprivate.h>
#include <modelviz/shapegeometry.h>
#include <modelviz/nodemaskdefs.h>

using namespace mdv;
using namespace osg;

ShapeGeometry::ShapeGeometry() : Base()
{
  setUseDisplayList(false);
  setDataVariance(osg::Object::DYNAMIC);
  setUseVertexBufferObjects(true);
}

ShapeGeometry::ShapeGeometry(const ShapeGeometry &rhs, const CopyOp& copyOperation) :
  Base(rhs, copyOperation)
{
  setUseDisplayList(false);
  setDataVariance(osg::Object::DYNAMIC);
  setUseVertexBufferObjects(true);
  
  if (rhs.idPSetWrapper)
    idPSetWrapper = std::shared_ptr<IdPSetWrapper>(new IdPSetWrapper(*rhs.idPSetWrapper));
}

void ShapeGeometry::setIdPSetWrapper(std::shared_ptr<IdPSetWrapper> &mapIn)
{
  idPSetWrapper = mapIn;
}

void ShapeGeometry::setPSetVertexWrapper(std::shared_ptr<PSetVertexWrapper> &wrapperIn)
{
  pSetVertexWrapper = wrapperIn;
}

void ShapeGeometry::setColor(const Vec4& colorIn)
{
  //we are assuming that any callers have cleared any selection
  //before call this function. So don't worry about current color state.
  color = colorIn;
  osg::Vec4Array *colors = dynamic_cast<osg::Vec4Array *>(_colorArray.get());
  assert(colors);
  for (auto &c : *colors)
    c = color;
  _colorArray->dirty();
}

void ShapeGeometry::setColor(const boost::uuids::uuid &idIn, const osg::Vec4 &colorIn)
{
  if (!idPSetWrapper->hasId(idIn))
    return;
  std::size_t primitiveIndex = idPSetWrapper->findPSetFromId(idIn);
  
  assert(primitiveIndex < getNumPrimitiveSets());
  DrawElementsUInt *pSet = dynamic_cast<DrawElementsUInt *>(getPrimitiveSet(primitiveIndex));
  assert(pSet);
  osg::Vec4Array *colors = dynamic_cast<osg::Vec4Array *>(_colorArray.get());
  assert(colors);
  for (auto index : *pSet)
    colors->at(index) = colorIn;
  _colorArray->dirty();
}

void ShapeGeometry::setToColor(const boost::uuids::uuid &idIn)
{
  setColor(idIn, color);
}

void ShapeGeometry::setToPreHighlight(const boost::uuids::uuid &idIn)
{
  setColor(idIn, colorPreHighlight);
}

void ShapeGeometry::setToHighlight(const boost::uuids::uuid &idIn)
{
  setColor(idIn, colorHighlight);
}

boost::uuids::uuid ShapeGeometry::getId(std::size_t primitiveIndexIn) const
{
  assert(idPSetWrapper->hasPSet(primitiveIndexIn));
  return idPSetWrapper->findIdFromPSet(primitiveIndexIn);
}

std::size_t ShapeGeometry::getPSetFromVertex(std::size_t vertexIndexIn) const
{
  return pSetVertexWrapper->findPSetFromVertex(vertexIndexIn);
}

const osg::BoundingSphere& ShapeGeometry::getBSphereFromPSet(std::size_t primitiveIndexIn) const
{
  return idPSetWrapper->findBSphereFromPSet(primitiveIndexIn);
}

ShapeGeometryBuilder::ShapeGeometryBuilder(std::shared_ptr<ftr::SeerShape> seerShapeIn) : 
  originalShape(seerShapeIn->getRootOCCTShape()), copiedShape(seerShapeIn->getRootOCCTShape()),
  seerShape(seerShapeIn), bound(), edgeToFace(),
  processed(), idPSetWrapperFace(new IdPSetWrapper()), idPSetWrapperEdge(new IdPSetWrapper())
{
  BRepBndLib::Add(copiedShape, bound);
  TopExp::MapShapesAndAncestors(copiedShape, TopAbs_EDGE, TopAbs_FACE, edgeToFace);
  
  edgeDepth = new osg::Depth();
  edgeDepth->setRange(0.001, 1.001);
  
  lineWidth = new osg::LineWidth(2.0f);
  
  faceDepth = new osg::Depth();
  faceDepth->setRange(0.002, 1.002);
}

ShapeGeometryBuilder::~ShapeGeometryBuilder()
{

}

void ShapeGeometryBuilder::go(double deflection, double angle)
{
  assert(!copiedShape.IsNull());
  success = false;
  
  initialize();
  
  try
  {
    BRepTools::Clean(copiedShape);//might call this several times for same shape;
    processed.Clear();
    BRepMesh_IncrementalMesh(copiedShape, deflection, Standard_False, angle,Standard_True);

    processed.Add(copiedShape);
    if (copiedShape.ShapeType() == TopAbs_FACE)
        faceConstruct(TopoDS::Face(copiedShape));
    if (copiedShape.ShapeType() == TopAbs_EDGE)
        edgeConstruct(TopoDS::Edge(copiedShape));
    recursiveConstruct(copiedShape);
    success = true;
  }
  catch(Standard_Failure)
  {
    Handle(Standard_Failure) error = Standard_Failure::Caught();
    std::cout << "OCC Error: failure building model vizualization. Message: " <<
        error->GetMessageString() << std::endl;
  }
  catch(const std::exception &error)
  {
    std::cout << "Internal Error: failure building model vizualization. Message: " <<
        error.what() << std::endl;
  }
  catch(...)
  {
    std::cout << "Unknown Error: failure building model vizualization. Message: " << std::endl;
  }
}

void ShapeGeometryBuilder::initialize()
{
  out = new osg::Switch();
  
  if (shouldBuildFaces)
  {
    faceGeometry = new ShapeGeometry();
    faceGeometry->setNodeMask(mdv::face);
    faceGeometry->setName("faces");
    faceGeometry->getOrCreateStateSet()->setAttribute(faceDepth.get());
    
    faceGeometry->setVertexArray(new osg::Vec3Array());
    faceGeometry->setUseDisplayList(false);
    faceGeometry->setUseVertexBufferObjects(true);

    faceGeometry->setColorArray(new osg::Vec4Array());
    faceGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

    faceGeometry->setNormalArray(new osg::Vec3Array());
    faceGeometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    
    faceGeometry->setIdPSetWrapper(idPSetWrapperFace);
    faceGeometry->setColor(osg::Vec4(.1f, .7f, .1f, .5f));
    
    faceGeometry->seerShape = seerShape;
    
    pSetTriangleWrapper = std::shared_ptr<PSetVertexWrapper>(new PSetVertexWrapper());
    faceGeometry->setPSetVertexWrapper(pSetTriangleWrapper);
    
    out->addChild(faceGeometry.get());
  }
  else
    faceGeometry = osg::ref_ptr<ShapeGeometry>();
  if (shouldBuildEdges)
  {
    edgeGeometry = new ShapeGeometry();
    edgeGeometry->setNodeMask(mdv::edge);
    edgeGeometry->setName("edges");
    edgeGeometry->getOrCreateStateSet()->setAttribute(edgeDepth.get());
    edgeGeometry->getOrCreateStateSet()->setAttribute(lineWidth.get());
    edgeGeometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    
    edgeGeometry->setVertexArray(new osg::Vec3Array());
    edgeGeometry->setColorArray(new osg::Vec4Array());
    edgeGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    edgeGeometry->setIdPSetWrapper(idPSetWrapperEdge);
    edgeGeometry->setColor(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
    
    edgeGeometry->seerShape = seerShape;
    
    out->addChild(edgeGeometry.get());
  }
  else
    edgeGeometry = osg::ref_ptr<ShapeGeometry>();
  if (shouldBuildVertices)
  {
    //not using vertices at time of writing, so has bugs.
    vertexGeometry = new ShapeGeometry();
    vertexGeometry->setNodeMask(mdv::vertex);
    vertexGeometry->setName("vertices");
    vertexGeometry->getOrCreateStateSet()->setAttribute(new osg::Point(5.0));
    vertexGeometry->setVertexArray(new osg::Vec3Array());
    
//     vertexGeometry->setMapWrapper(mapWrapperFace);
    out->addChild(vertexGeometry.get());
  }
  else
    vertexGeometry = osg::ref_ptr<ShapeGeometry>();
}

void ShapeGeometryBuilder::recursiveConstruct(const TopoDS_Shape &shapeIn)
{
  for (TopoDS_Iterator it(shapeIn); it.More(); it.Next())
  {
    const TopoDS_Shape &currentShape = it.Value();
    TopAbs_ShapeEnum currentType = currentShape.ShapeType();
    if (processed.Contains(currentShape))
	continue;
    processed.Add(currentShape);
    if (!(seerShape->hasShapeIdRecord(currentShape)))
      continue; //probably seam edge.

    if
    (
      (currentType == TopAbs_COMPOUND) ||
      (currentType == TopAbs_COMPSOLID) ||
      (currentType == TopAbs_SOLID) ||
      (currentType == TopAbs_SHELL) ||
      (currentType == TopAbs_WIRE)
    )
    {
      recursiveConstruct(currentShape);
      continue;
    }
    try
    {
      if (currentType == TopAbs_FACE)
      {
        faceConstruct(TopoDS::Face(currentShape));
        recursiveConstruct(currentShape);
        continue;
      }
      if (currentType == TopAbs_EDGE)
      {
        edgeConstruct(TopoDS::Edge(currentShape));
        recursiveConstruct(currentShape); // for obsolete vertices?
      }
    }
    catch(const std::exception &error)
    {
      std::cout << "Warning! Problem building model vizualization. Message: " <<
		    error.what() << std::endl;
    }
  }
}

void ShapeGeometryBuilder::faceConstruct(const TopoDS_Face &faceIn)
{
  TopLoc_Location location;
  Handle(Poly_Triangulation) triangulation;
  triangulation = BRep_Tool::Triangulation(faceIn, location);

  if (triangulation.IsNull())
      throw std::runtime_error("null triangulation in face construction");

  bool signalOrientation(false);
  if (faceIn.Orientation() == TopAbs_FORWARD)
      signalOrientation = true;

  gp_Trsf transformation;
  bool identity = true;
  if(!location.IsIdentity())
  {
    identity = false;
    transformation = location.Transformation();
  }

  //vertices.
  const TColgp_Array1OfPnt& nodes = triangulation->Nodes();
  osg::Vec3Array *vertices = dynamic_cast<osg::Vec3Array *>(faceGeometry->getVertexArray());
  osg::Vec3Array *normals = dynamic_cast<osg::Vec3Array *>(faceGeometry->getNormalArray());
  osg::Vec4Array *colors = dynamic_cast<osg::Vec4Array *>(faceGeometry->getColorArray());
  std::size_t offset = vertices->size();
  for (int index(nodes.Lower()); index < nodes.Upper() + 1; ++index)
  {
    gp_Pnt point = nodes.Value(index);
    if(!identity)
	point.Transform(transformation);
    vertices->push_back(osg::Vec3(point.X(), point.Y(), point.Z()));
    normals->push_back(osg::Vec3(0.0, 0.0, 0.0));
    colors->push_back(faceGeometry->getColor());
  }

  const Poly_Array1OfTriangle& triangles = triangulation->Triangles();
  osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt
    (GL_TRIANGLES, triangulation->NbTriangles() * 3);

  for (int index(triangles.Lower()); index < triangles.Upper() + 1; ++index)
  {
    int N1, N2, N3;
    triangles(index).Get(N1, N2, N3);

    int factor = (index - 1) * 3;
    if (!signalOrientation)
    {
      (*indices)[factor] = N3 - 1  + offset;
      (*indices)[factor + 1] = N2 - 1 + offset;
      (*indices)[factor + 2] = N1 - 1 + offset;
    }
    else
    {
      (*indices)[factor] = N1 - 1 + offset;
      (*indices)[factor + 1] = N2 - 1 + offset;
      (*indices)[factor + 2] = N3 - 1 + offset;
    }
    
    //store primitiveset index and vertex indes into map.
    PSetVertexRecord record;
    record.primitiveSetIndex = faceGeometry->getNumPrimitiveSets();
    
    record.vertexIndex = (*indices)[factor];
    pSetTriangleWrapper->pSetVertexContainer.insert(record);
    
    record.vertexIndex = (*indices)[factor + 1];
    pSetTriangleWrapper->pSetVertexContainer.insert(record);
    
    record.vertexIndex = (*indices)[factor + 2];
    pSetTriangleWrapper->pSetVertexContainer.insert(record);
    
    //now that we are combining faces into one geometry, osgUtil SmoothingVisitor
    //wants to 'average' out normals across faces. so we go back to manual calculation
    //of surface normals.

    osg::Vec3 pointOne(vertices->at((*indices)[factor]));
    osg::Vec3 pointTwo(vertices->at((*indices)[factor + 1]));
    osg::Vec3 pointThree(vertices->at((*indices)[factor + 2]));

    osg::Vec3 axisOne(pointTwo - pointOne);
    osg::Vec3 axisTwo(pointThree - pointOne);
    osg::Vec3 currentNormal(axisOne ^ axisTwo);
    if (currentNormal.isNaN())
        continue;
    currentNormal.normalize();

    osg::Vec3 tempNormal;

    tempNormal = (*normals)[(*indices)[factor]];
    tempNormal += currentNormal;
    tempNormal.normalize();
    (*normals)[(*indices)[factor]] = tempNormal;

    tempNormal = (*normals)[(*indices)[factor + 1]];
    tempNormal += currentNormal;
    tempNormal.normalize();
    (*normals)[(*indices)[factor + 1]] = tempNormal;

    tempNormal = (*normals)[(*indices)[factor + 2]];
    tempNormal += currentNormal;
    tempNormal.normalize();
    (*normals)[(*indices)[factor + 2]] = tempNormal;
  }
  faceGeometry->addPrimitiveSet(indices.get());
  boost::uuids::uuid id = seerShape->findShapeIdRecord(faceIn).id;
  std::size_t lastPrimitiveIndex = faceGeometry->getNumPrimitiveSets() - 1;
  if (!idPSetWrapperFace->hasId(id))
  {
    IdPSetRecord record;
    record.id = id;
    record.primitiveSetIndex = lastPrimitiveIndex;
    idPSetWrapperFace->idPSetContainer.insert(record);
  }
  else
    //ensure that the faces have the same primitive index between lod calls.
    assert(lastPrimitiveIndex == idPSetWrapperFace->findPSetFromId(id));
}

void ShapeGeometryBuilder::edgeConstruct(const TopoDS_Edge &edgeIn)
{
  TopoDS_Face face = TopoDS::Face(edgeToFace.FindFromKey(edgeIn).First());
  if (face.IsNull())
    throw std::runtime_error("face is null in edge construction");

  TopLoc_Location location;
  Handle(Poly_Triangulation) triangulation;
  triangulation = BRep_Tool::Triangulation(face, location);

  const Handle(Poly_PolygonOnTriangulation) &segments =
      BRep_Tool::PolygonOnTriangulation(edgeIn, triangulation, location);
  if (segments.IsNull())
    throw std::runtime_error("edge triangulation is null in edge construction");

  gp_Trsf transformation;
  bool identity = true;
  if(!location.IsIdentity())
  {
    identity = false;
    transformation = location.Transformation();
  }

  const TColStd_Array1OfInteger& indexes = segments->Nodes();
  const TColgp_Array1OfPnt& nodes = triangulation->Nodes();
  osg::Vec3Array *vertices = dynamic_cast<osg::Vec3Array *>(edgeGeometry->getVertexArray());
  osg::Vec4Array *colors = dynamic_cast<osg::Vec4Array *>(edgeGeometry->getColorArray());
  osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt
    (GL_LINE_STRIP, indexes.Length());
  osg::BoundingSphere bSphere;
  
  for (int index(indexes.Lower()); index < indexes.Upper() + 1; ++index)
  {
    gp_Pnt point = nodes(indexes(index));
    if(!identity)
        point.Transform(transformation);
    vertices->push_back(osg::Vec3(point.X(), point.Y(), point.Z()));
    colors->push_back(edgeGeometry->getColor());
    (*indices)[index - 1] = vertices->size() - 1;
    
    if (!bSphere.valid()) //for first one.
    {
      bSphere.center() = vertices->back();
      bSphere.radius() = 0.0;
    }
    else
      bSphere.expandBy(vertices->back());
  }
  edgeGeometry->addPrimitiveSet(indices.get());
  boost::uuids::uuid id = seerShape->findShapeIdRecord(edgeIn).id;
  std::size_t lastPrimitiveIndex = edgeGeometry->getNumPrimitiveSets() - 1;
  if (!idPSetWrapperEdge->hasId(id))
  {
    IdPSetRecord record;
    record.id = id;
    record.primitiveSetIndex = lastPrimitiveIndex;
    record.bSphere = bSphere;
    idPSetWrapperEdge->idPSetContainer.insert(record);
  }
  else
    //ensure that edges have the same primitive index between lod calls.
    //asserts here prior to having lod implemented is probably duplicate ids
    //for different geometry.
    assert(lastPrimitiveIndex == idPSetWrapperEdge->findPSetFromId(id));
}


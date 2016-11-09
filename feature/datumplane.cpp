/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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

#include <limits>

#include <QDir>

#include <BRepAdaptor_Surface.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopExp.hxx>
#include <gp_Pln.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <BRep_Builder.hxx>

#include <osg/Geometry>
#include <osg/MatrixTransform>

#include <globalutilities.h>
#include <nodemaskdefs.h>
#include <library/ipgroup.h>
#include <modelviz/datumplane.h>
#include <feature/seershape.h>
#include <feature/datumplane.h>

using namespace ftr;

QIcon DatumPlane::icon;

static Bnd_Box getBoundingBox(const TopoDS_Shape &shapeIn)
{
  Bnd_Box out;
  BRepBndLib::Add(shapeIn, out);
  return out;
}

static void centerRadius(const Bnd_Box &boxIn, gp_Pnt &centerOut, gp_Pnt &cornerOut)
{
  cornerOut = boxIn.CornerMin();
  
  gp_Vec min(boxIn.CornerMin().Coord());
  gp_Vec max(boxIn.CornerMax().Coord());
  gp_Vec projection(max - min);
  projection = projection.Normalized() * (projection.Magnitude() / 2.0);
  gp_Vec centerVec = min + projection;
  centerOut = gp_Pnt(centerVec.XYZ());
}

//throws std::runtime_error
static osg::Matrixd getFaceSystem(const TopoDS_Shape &faceShape)
{
  assert(faceShape.ShapeType() == TopAbs_FACE);
  BRepAdaptor_Surface adaptor(TopoDS::Face(faceShape));
  if (adaptor.GetType() != GeomAbs_Plane)
    throw std::runtime_error("DatumPlane: wrong surface type");
  gp_Ax2 tempSystem = adaptor.Plane().Position().Ax2();
  if (faceShape.Orientation() == TopAbs_REVERSED)
    tempSystem.SetDirection(tempSystem.Direction().Reversed());
  osg::Matrixd faceSystem = gu::toOsg(tempSystem);
  
  gp_Pnt centerPoint, cornerPoint;
  centerRadius(getBoundingBox(faceShape), centerPoint, cornerPoint);
  double centerDistance = adaptor.Plane().Distance(centerPoint);
  osg::Vec3d workVector = gu::getZVector(faceSystem);
  osg::Vec3d centerVec = gu::toOsg(centerPoint) + (workVector * centerDistance);
  faceSystem.setTrans(centerVec);
  
  return faceSystem;
};

DatumPlanePlanarOffset::DatumPlanePlanarOffset():
  offset(new Parameter(ParameterNames::Offset, 1.0))
{
  offset->setCanBeNegative(true);
  
  offsetIP = new lbr::IPGroup(offset.get());
  offsetIP->setMatrixDims(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(1.0, 0.0, 0.0)));
  offsetIP->noAutoRotateDragger();
  offsetIP->setRotationAxis(osg::Vec3d(0.0, 0.0, 1.0), osg::Vec3d(0.0, 1.0, 0.0));
  offsetIP->valueHasChanged();
  offsetIP->constantHasChanged();
}

DatumPlanePlanarOffset::~DatumPlanePlanarOffset()
{}

osg::Matrixd DatumPlanePlanarOffset::solve(const UpdateMap &mapIn)
{
  if (mapIn.size() != 1)
    throw std::runtime_error("DatumPlanePlanarOffset: wrong number of inputs");
  
  UpdateMap::const_iterator it = mapIn.find(InputTypes::datumPlanarOffset);
  if (it == mapIn.end())
    throw std::runtime_error("DatumPlanePlanarOffset: no input feature");
  
  osg::Matrixd faceSystem, datumSystem;
  faceSystem = datumSystem = osg::Matrixd::identity();
  if (it->second->getType() == Type::DatumPlane)
  {
    const DatumPlane *inputPlane = dynamic_cast<const DatumPlane*>(it->second);
    assert(inputPlane);
    faceSystem = inputPlane->getSystem();
    
    // just size this plane to the source plane.
    radius = inputPlane->getRadius();
  }
  else //assuming seer shape.
  {
    if (faceId.is_nil())
      throw std::runtime_error("DatumPlanePlanarOffset: nil faceId");
    const SeerShape &shape = it->second->getSeerShape();
    if (shape.isNull())
      throw std::runtime_error("DatumPlanePlanarOffset: null seer shape");
    if (!shape.hasShapeIdRecord(faceId))
      throw std::runtime_error("DatumPlanePlanarOffset: no faceId in seer shape");
    const TopoDS_Shape &faceShape = shape.getOCCTShape(faceId);
    if (faceShape.ShapeType() != TopAbs_FACE)
      throw std::runtime_error("DatumPlanePlanarOffset: wrong shape type");
    
    //get coordinate system
    BRepAdaptor_Surface adaptor(TopoDS::Face(faceShape));
    if (adaptor.GetType() != GeomAbs_Plane)
      throw std::runtime_error("DatumPlanePlanarOffset: wrong surface type");
    gp_Ax2 tempSystem = adaptor.Plane().Position().Ax2();
    if (faceShape.Orientation() == TopAbs_REVERSED)
      tempSystem.SetDirection(tempSystem.Direction().Reversed());
    faceSystem = gu::toOsg(tempSystem);
    
    //calculate parameter boundaries and project onto plane.
    gp_Pnt centerPoint, cornerPoint;
    centerRadius(getBoundingBox(faceShape), centerPoint, cornerPoint);
    double centerDistance = adaptor.Plane().Distance(centerPoint);
    double cornerDistance = adaptor.Plane().Distance(cornerPoint);
    osg::Vec3d workVector = gu::getZVector(faceSystem);
    osg::Vec3d centerVec = gu::toOsg(centerPoint) + (workVector * centerDistance);
    osg::Vec3d cornerVec = gu::toOsg(cornerPoint) + (workVector * cornerDistance);
    faceSystem.setTrans(centerVec);
    
    osg::Vec3d offsetVec = centerVec * osg::Matrixd::inverse(faceSystem);
    double tempRadius = (centerVec - cornerVec).length();
    radius = offsetVec.x() + tempRadius;
  }
  datumSystem = faceSystem;
  osg::Vec3d normal = gu::getZVector(faceSystem) * offset->getValue();
  datumSystem.setTrans(faceSystem.getTrans() + normal);
  offsetIP->setMatrix(faceSystem); //update the interactive parameter.
  return datumSystem;
}

bool DatumPlanePlanarOffset::canDoTypes(const slc::Containers &containersIn)
{
  if (containersIn.size() != 1)
    return false;
  if (containersIn.at(0).featureType == Type::DatumPlane)
    return true;
  if (containersIn.at(0).selectionType == slc::Type::Face)
    return true;
  
  return false;
}

lbr::IPGroup* DatumPlanePlanarOffset::getIPGroup()
{
  return offsetIP.get();
}

void DatumPlanePlanarOffset::connect(Base *baseIn)
{
  offset->connectValue(boost::bind(&Base::setModelDirty, baseIn));
}

DatumPlaneConnections DatumPlanePlanarOffset::setUpFromSelection(const slc::Containers &containersIn)
{
  //assuming containersIn has been through 'canDoTypes'.
  assert(containersIn.size() == 1);
  assert
  (
    (containersIn.at(0).featureType == Type::DatumPlane) ||
    (containersIn.at(0).selectionType == slc::Type::Face)
  );
  
  offset->setValue(2.0);
  faceId = containersIn.at(0).shapeId;
  
  DatumPlaneConnections out;
  DatumPlaneConnection connection;
  connection.inputType = ftr::InputTypes::datumPlanarOffset;
  connection.parentId = containersIn.at(0).featureId;
  out.push_back(connection);
  
  return out;
}


DatumPlanePlanarCenter::DatumPlanePlanarCenter()
  {}

DatumPlanePlanarCenter::~DatumPlanePlanarCenter()
  {}

osg::Matrixd DatumPlanePlanarCenter::solve(const UpdateMap &mapIn)
{
  osg::Matrixd face1System, face2System;
  face1System = face2System = osg::Matrixd::identity();
  double tempRadius = 1.0;
  
  if (mapIn.size() == 1)
  {
    //note: can't do a center with only 1 datum plane. so we know this condition must be a shape.
    UpdateMap::const_iterator it = mapIn.find(InputTypes::datumPlanarCenterBoth);
    if (it == mapIn.end())
      throw std::runtime_error("DatumPlanePlanarCenter: Conflict between map size and count");
  
    const SeerShape &shape = it->second->getSeerShape();
    if (shape.isNull())
      throw std::runtime_error("DatumPlanePlanarCenter: null seer shape");
    if (!shape.hasShapeIdRecord(faceId1))
      throw std::runtime_error("DatumPlanePlanarCenter: no faceId1 in seer shape");
    if (!shape.hasShapeIdRecord(faceId2))
      throw std::runtime_error("DatumPlanePlanarCenter: no faceId2 in seer shape");
  
    TopoDS_Shape face1 = shape.getOCCTShape(faceId1); assert(!face1.IsNull());
    TopoDS_Shape face2 = shape.getOCCTShape(faceId2); assert(!face2.IsNull());
    if (face1.IsNull() || face2.IsNull())
      throw std::runtime_error("DatumPlanePlanarCenter: null faces");
    face1System = getFaceSystem(face1);
    face2System = getFaceSystem(face2);
    
    //calculate size.
    
    tempRadius = std::max(std::sqrt(getBoundingBox(face1).SquareExtent()), std::sqrt(getBoundingBox(face2).SquareExtent())) / 2.0;
  }
  else if (mapIn.size() == 2)
  {
    //with 2 inputs, either one can be a face of a datum.
    double radius1, radius2;
    UpdateMap::const_iterator it = mapIn.find(InputTypes::datumPlanarCenter1);
    if (it == mapIn.end())
      throw std::runtime_error("DatumPlanePlanarCenter: Can't find datumPlanarCenter1");
    if (it->second->getType() == Type::DatumPlane)
    {
      const DatumPlane *inputPlane = dynamic_cast<const DatumPlane*>(it->second);
      assert(inputPlane);
      face1System = inputPlane->getSystem();
      radius1 = inputPlane->getRadius();
    }
    else
    {
      //assume seer shape.
      const SeerShape &shape1 = it->second->getSeerShape();
      if (shape1.isNull())
	throw std::runtime_error("DatumPlanePlanarCenter: null seer shape1");
      if (!shape1.hasShapeIdRecord(faceId1))
	throw std::runtime_error("DatumPlanePlanarCenter: no faceId1 in seer shape1");
      TopoDS_Shape face1 = shape1.getOCCTShape(faceId1); assert(!face1.IsNull());
      if (face1.ShapeType() != TopAbs_FACE)
	throw std::runtime_error("DatumPlanePlanarCenter: face1 is not of type face");
      face1System = getFaceSystem(face1);
      radius1 = std::sqrt(getBoundingBox(face1).SquareExtent()) / 2.0;
    }
    
    it = mapIn.find(InputTypes::datumPlanarCenter2);
    if (it == mapIn.end())
      throw std::runtime_error("DatumPlanePlanarCenter: Can't find datumPlanarCenter2");
    if (it->second->getType() == Type::DatumPlane)
    {
      const DatumPlane *inputPlane = dynamic_cast<const DatumPlane*>(it->second);
      assert(inputPlane);
      face2System = inputPlane->getSystem();
      radius2 = inputPlane->getRadius();
    }
    else
    {
      //assume seer shape.
      const SeerShape &shape2 = it->second->getSeerShape();
      if (shape2.isNull())
	throw std::runtime_error("DatumPlanePlanarCenter: null seer shape2");
      if (!shape2.hasShapeIdRecord(faceId2))
	throw std::runtime_error("DatumPlanePlanarCenter: no faceId2 in seer shape");
      TopoDS_Shape face2 = shape2.getOCCTShape(faceId2); assert(!face2.IsNull());
      if (face2.ShapeType() != TopAbs_FACE)
	throw std::runtime_error("DatumPlanePlanarCenter: face2 is not of type face");
      face2System = getFaceSystem(face2);
      radius2 = std::sqrt(getBoundingBox(face2).SquareExtent()) / 2.0;
    }
    tempRadius = std::max(radius1, radius2);
  }
  else
    throw std::runtime_error("DatumPlanePlanarCenter: wrong number of inputs");
  
  osg::Vec3d normal1 = gu::getZVector(face1System);
  osg::Vec3d normal2 = gu::getZVector(face2System);
  
  if (!(gu::toOcc(normal1).IsParallel(gu::toOcc(normal2), Precision::Angular())))
    throw std::runtime_error("DatumPlanePlanarCenter: planes not parallel");
  
  osg::Vec3d projection = face2System.getTrans() - face1System.getTrans();
  double mag = projection.length() / 2.0;
  projection.normalize();
  projection *= mag;
  osg::Vec3d freshOrigin = face1System.getTrans() + projection;
  
  osg::Matrixd newSystem = face1System;
  newSystem.setTrans(freshOrigin);
  
  //update bounding size.
  radius = tempRadius;
  
  return newSystem;
}

bool DatumPlanePlanarCenter::canDoTypes(const slc::Containers &containersIn)
{
  if (containersIn.size() != 2)
    return false;
  if
  (
    (containersIn.at(0).selectionType != slc::Type::Face) && 
    (containersIn.at(0).featureType != Type::DatumPlane)
  )
    return false;
    
  if
  (
    (containersIn.at(1).selectionType != slc::Type::Face) && 
    (containersIn.at(1).featureType != Type::DatumPlane)
  )
    return false;
    
  return true;
}

DatumPlaneConnections DatumPlanePlanarCenter::setUpFromSelection(const slc::Containers &containersIn)
{
  //assume we have been through 'canDoTypes'
  assert(containersIn.size() == 2);
  assert((containersIn.at(0).selectionType == slc::Type::Face) || (containersIn.at(0).featureType == Type::DatumPlane));
  assert((containersIn.at(1).selectionType == slc::Type::Face) || (containersIn.at(1).featureType == Type::DatumPlane));
  
  //note if a datum is selected then the shapeId is nil. Which is what we want.
  faceId1 = containersIn.at(0).shapeId;
  faceId2 = containersIn.at(1).shapeId;
  
  DatumPlaneConnection connection;
  DatumPlaneConnections out;
  if (containersIn.at(0).featureId == containersIn.at(1).featureId)
  {
    connection.inputType = ftr::InputTypes::datumPlanarCenterBoth;
    connection.parentId = containersIn.at(0).featureId;
    out.push_back(connection);
  }
  else
  {
    connection.inputType = ftr::InputTypes::datumPlanarCenter1;
    connection.parentId = containersIn.at(0).featureId;
    out.push_back(connection);
    connection.inputType = ftr::InputTypes::datumPlanarCenter2;
    connection.parentId = containersIn.at(1).featureId;
    out.push_back(connection);
  }
  return out;
}

DatumPlanePlanarParallelThroughEdge::DatumPlanePlanarParallelThroughEdge(){}

DatumPlanePlanarParallelThroughEdge::~DatumPlanePlanarParallelThroughEdge(){}

osg::Matrixd DatumPlanePlanarParallelThroughEdge::solve(const UpdateMap &mapIn)
{
  auto getEdgeVector = [](const TopoDS_Shape &edgeShapeIn)
  {
    assert(edgeShapeIn.ShapeType() == TopAbs_EDGE);
    TopoDS_Edge edge = TopoDS::Edge(edgeShapeIn);
    TopoDS_Vertex v1 = TopExp::FirstVertex(edge);
    TopoDS_Vertex v2 = TopExp::LastVertex(edge);
    
    return gu::toOsg(v2) - gu::toOsg(v1);
  };
  
  auto getOrigin = [](const TopoDS_Shape &edgeShapeIn)
  {
    assert(edgeShapeIn.ShapeType() == TopAbs_EDGE);
    TopoDS_Edge edge = TopoDS::Edge(edgeShapeIn);
    TopoDS_Vertex v1 = TopExp::FirstVertex(edge);
    TopoDS_Vertex v2 = TopExp::LastVertex(edge);
    
    osg::Vec3d point1 = gu::toOsg(v1);
    osg::Vec3d point2 = gu::toOsg(v2);
    osg::Vec3d direction = point2 - point1;
    double mag = direction.length();
    mag *= .5;
    direction.normalize();
    direction *= mag;
    return point1 + direction;
  };
  
  osg::Matrixd outSystem = osg::Matrixd::identity();
  osg::Vec3d origin, direction;
  double tempRadius = 1.0;
  
  if (mapIn.size() == 1)
  {
    //if only one 'in' connection that means we have a face and an edge belonging to
    //the same object.
    UpdateMap::const_iterator it = mapIn.find(InputTypes::datumPlanarParallelThroughEdgeBoth);
    if (it == mapIn.end())
      throw std::runtime_error("DatumPlanarParallelThroughEdge: Conflict between map size and count");
    if (faceId.is_nil() || edgeId.is_nil())
      throw std::runtime_error("DatumPlanarParallelThroughEdge: nil id");
    it = mapIn.find(InputTypes::datumPlanarParallelThroughEdgeBoth);
    if (it == mapIn.end())
      throw std::runtime_error("DatumPlanarParallelThroughEdge: map size out of sync with graph edge type");
    
    const SeerShape &shape = it->second->getSeerShape();
    if (shape.isNull())
      throw std::runtime_error("DatumPlanarParallelThroughEdge: null seer shape");
    if (!shape.hasShapeIdRecord(faceId))
      throw std::runtime_error("DatumPlanarParallelThroughEdge: no faceId in seer shape");
    if (!shape.hasShapeIdRecord(edgeId))
      throw std::runtime_error("DatumPlanarParallelThroughEdge: no edgeId in seer shape");
  
    const TopoDS_Shape &face = shape.getOCCTShape(faceId); assert(!face.IsNull());
    const TopoDS_Shape &edge = shape.getOCCTShape(edgeId); assert(!edge.IsNull()); assert(edge.ShapeType() == TopAbs_EDGE);
    if (face.IsNull() || edge.IsNull())
      throw std::runtime_error("DatumPlanarParallelThroughEdge: null edge or face");
    outSystem = getFaceSystem(face);
    direction = getEdgeVector(edge);
    origin = getOrigin(edge);
    
    if (std::fabs((gu::getZVector(outSystem) * direction)) > std::numeric_limits<float>::epsilon())
      throw std::runtime_error("DatumPlanarParallelThroughEdge: edge and face are not orthogonal");
    
    tempRadius = std::sqrt(getBoundingBox(face).SquareExtent()) / 2.0;
    
    outSystem.setTrans(origin);
  }
  else if (mapIn.size() == 2)
  {
    //might be linked to another datum plane.
    UpdateMap::const_iterator it = mapIn.find(InputTypes::datumPlanarParallelThroughEdgeFace);
    if (it->second->getType() == Type::DatumPlane)
    {
      const DatumPlane *dPlane = dynamic_cast<const DatumPlane*>(it->second);
      assert(dPlane);
      outSystem = dPlane->getSystem();
      tempRadius = dPlane->getRadius();
    }
    else
    {
      const SeerShape &shape = it->second->getSeerShape();
      if (shape.isNull())
	throw std::runtime_error("DatumPlanarParallelThroughEdge: null seer shape for face");
      if (!shape.hasShapeIdRecord(faceId))
	throw std::runtime_error("DatumPlanarParallelThroughEdge: no faceId in seer shape");
      
      const TopoDS_Shape &face = shape.getOCCTShape(faceId); assert(!face.IsNull());
      outSystem = getFaceSystem(face);
      tempRadius = std::sqrt(getBoundingBox(face).SquareExtent()) / 2.0;
    }
    
    it = mapIn.find(InputTypes::datumPlanarParallelThroughEdgeEdge);
    if (it == mapIn.end())
      throw std::runtime_error("DatumPlanarParallelThroughEdge: No edge found");
    const SeerShape &shape = it->second->getSeerShape();
    if (shape.isNull())
      throw std::runtime_error("DatumPlanarParallelThroughEdge: null seer shape for edge");
    if (!shape.hasShapeIdRecord(edgeId))
      throw std::runtime_error("DatumPlanarParallelThroughEdge: no edgeId in seer shape");
    
    const TopoDS_Shape &edge = shape.getOCCTShape(edgeId); assert(!edge.IsNull());
    
    direction = getEdgeVector(edge);
    origin = getOrigin(edge);
    
    if (std::fabs((gu::getZVector(outSystem) * direction)) > std::numeric_limits<float>::epsilon())
      throw std::runtime_error("DatumPlanarParallelThroughEdge: edge and face are not orthogonal");
    
    outSystem.setTrans(origin);
  }
  
  //update bounding size.
  radius = tempRadius;
  
  return outSystem;
}

bool DatumPlanePlanarParallelThroughEdge::canDoTypes(const slc::Containers &containersIn)
{
  if (containersIn.size() != 2)
    return false;
  if
  (
    (containersIn.at(0).selectionType != slc::Type::Face) && 
    (containersIn.at(0).selectionType != slc::Type::Edge) &&
    (containersIn.at(0).featureType != Type::DatumPlane)
  )
    return false;
    
  if
  (
    (containersIn.at(1).selectionType != slc::Type::Face) && 
    (containersIn.at(1).selectionType != slc::Type::Edge) && 
    (containersIn.at(1).featureType != Type::DatumPlane)
  )
    return false;
    
  return true;
}

DatumPlaneConnections DatumPlanePlanarParallelThroughEdge::setUpFromSelection(const slc::Containers &containersIn)
{
  //assume we have through 'canDoTypes'
  assert(containersIn.size() == 2);
  assert
  (
    (containersIn.at(0).selectionType == slc::Type::Face) || 
    (containersIn.at(0).selectionType == slc::Type::Edge) ||
    (containersIn.at(0).featureType == Type::DatumPlane)
  );
  assert
  (
    (containersIn.at(1).selectionType == slc::Type::Face) || 
    (containersIn.at(1).selectionType == slc::Type::Edge) ||
    (containersIn.at(1).featureType == Type::DatumPlane)
  );
  
  DatumPlaneConnection connection;
  DatumPlaneConnections out;
  if (containersIn.at(0).featureId == containersIn.at(1).featureId)
  {
    connection.inputType = InputTypes::datumPlanarParallelThroughEdgeBoth;
    connection.parentId = containersIn.at(0).featureId;
    out.push_back(connection);
    
    //we know we don't have a datum because we would have to have different feature ids.
    if (containersIn.at(0).selectionType == slc::Type::Face)
      faceId = containersIn.at(0).shapeId;
    if (containersIn.at(0).selectionType == slc::Type::Edge)
      edgeId = containersIn.at(0).shapeId;
    if (containersIn.at(1).selectionType == slc::Type::Face)
      faceId = containersIn.at(1).shapeId;
    if (containersIn.at(1).selectionType == slc::Type::Edge)
      edgeId = containersIn.at(1).shapeId;
    
    return out;
  }
  
  for (const auto &container : containersIn)
  {
    if (container.selectionType == slc::Type::Face)
    {
      faceId = container.shapeId;
      connection.inputType = InputTypes::datumPlanarParallelThroughEdgeFace;
      connection.parentId = container.featureId;
      out.push_back(connection);
    }
    else if(container.selectionType == slc::Type::Edge)
    {
      edgeId = container.shapeId;
      connection.inputType = InputTypes::datumPlanarParallelThroughEdgeEdge;
      connection.parentId = container.featureId;
      out.push_back(connection);
    }
    else if(container.featureType == Type::DatumPlane)
    {
      faceId = container.shapeId; //will be nil
      connection.inputType = InputTypes::datumPlanarParallelThroughEdgeFace;
      connection.parentId = container.featureId;
      out.push_back(connection);
    }
  }
  
  return out;
}


//default constructed plane is 2.0 x 2.0
DatumPlane::DatumPlane() : Base()
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionDatumPlane.svg");
  
  name = QObject::tr("Datum Plane");
  mainSwitch->setUserValue(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  display = new mdv::DatumPlane();
  
  transform = new osg::MatrixTransform();
  transform->addChild(display.get());
  
  mainSwitch->addChild(transform.get());
  updateGeometry();
}

DatumPlane::~DatumPlane()
{

}

void DatumPlane::setSolver(std::shared_ptr<DatumPlaneGenre> solverIn)
{
  solver = solverIn;
  if (solver->getIPGroup())
    overlaySwitch->addChild(solver->getIPGroup());
  solver->connect(this);
}

void DatumPlane::updateModel(const UpdateMap &mapIn)
{
  osg::Matrixd matrix(osg::Matrixd::identity());
  osg::Vec3d oldXAxis(1.0, 0.0, 0.0);
  osg::Vec3d newXAxis(1.0, 1.0, 1.0);
  newXAxis.normalize();
  matrix *= osg::Matrixd::rotate(oldXAxis, newXAxis);
  transform->setMatrix(matrix);
  
  setFailure();
  try
  {
    if (!solver)
      throw std::runtime_error("no solver");
    
    transform->setMatrix(solver->solve(mapIn));
    radius = solver->radius;
    
    setSuccess();
  }
  catch (std::exception &e)
  {
    std::cout << std::endl << "Error in datum plane update. " << e.what() << std::endl;
  }
  
  setModelClean();
}

void DatumPlane::updateVisual()
{
  if (autoSize)
    updateGeometry();
  setVisualClean();
}

//serial support for datum plane needs to be done.
void DatumPlane::serialWrite(const QDir &)//directoryIn)
{
//   ftr::Base::serialWrite(directoryIn);
}

void DatumPlane::updateGeometry()
{
  display->setParameters(-radius, radius, -radius, radius);
  
}

std::vector<std::shared_ptr<DatumPlaneGenre> > DatumPlane::solversFromSelection(const slc::Containers &containersIn)
{
  //Most common types first. genre tests only test the types of object not if can actually solve. For example
  //planar offset only tests that it has a face, it doesn't test if that face is planar. So solving could still
  //fail.
  
  std::vector<std::shared_ptr<DatumPlaneGenre> > out;
  
  if (DatumPlanePlanarOffset::canDoTypes(containersIn))
    out.push_back(std::shared_ptr<DatumPlanePlanarOffset>(new DatumPlanePlanarOffset()));
  if (DatumPlanePlanarCenter::canDoTypes(containersIn))
    out.push_back(std::shared_ptr<DatumPlanePlanarCenter>(new DatumPlanePlanarCenter()));
  if (DatumPlanePlanarParallelThroughEdge::canDoTypes(containersIn))
    out.push_back(std::shared_ptr<DatumPlanePlanarParallelThroughEdge>(new DatumPlanePlanarParallelThroughEdge()));
  
  return out;
}

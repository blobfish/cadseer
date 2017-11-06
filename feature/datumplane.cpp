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
#include <QTextStream>

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

#include <tools/infotools.h>
#include <globalutilities.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <modelviz/nodemaskdefs.h>
#include <library/ipgroup.h>
#include <modelviz/datumplane.h>
#include <feature/seershape.h>
#include <feature/datumplane.h>
#include <project/serial/xsdcxxoutput/featuredatumplane.h>

using namespace ftr;

QIcon DatumPlane::icon;

using boost::uuids::uuid;

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
  offset(new prm::Parameter(prm::Names::Offset, prf::manager().rootPtr->features().datumPlane().get().offset()))
{
  offsetIP = new lbr::IPGroup(offset.get());
  offsetIP->setMatrixDims(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(1.0, 0.0, 0.0)));
  offsetIP->noAutoRotateDragger();
  offsetIP->setRotationAxis(osg::Vec3d(0.0, 0.0, 1.0), osg::Vec3d(0.0, 1.0, 0.0));
  offsetIP->valueHasChanged();
  offsetIP->constantHasChanged();
}

DatumPlanePlanarOffset::~DatumPlanePlanarOffset()
{}

osg::Matrixd DatumPlanePlanarOffset::solve(const UpdatePayload::UpdateMap &mapIn)
{
  if (mapIn.size() != 1)
    throw std::runtime_error("DatumPlanePlanarOffset: wrong number of inputs");
  
  UpdatePayload::UpdateMap::const_iterator it = mapIn.find(InputType::create);
  if (it == mapIn.end())
    throw std::runtime_error("DatumPlanePlanarOffset: no input feature");
  
  const Base* parentFeature = it->second;
  
  osg::Matrixd faceSystem, datumSystem;
  faceSystem = datumSystem = osg::Matrixd::identity();
  if (parentFeature->getType() == Type::DatumPlane)
  {
    const DatumPlane *inputPlane = dynamic_cast<const DatumPlane*>(parentFeature);
    assert(inputPlane);
    faceSystem = inputPlane->getSystem();
    
    // just size this plane to the source plane.
    radius = inputPlane->getRadius();
  }
  else //assuming seer shape.
  {
    if (facePick.id.is_nil())
      throw std::runtime_error("DatumPlanePlanarOffset: nil faceId");
    assert(parentFeature->hasSeerShape());
    const SeerShape &shape = parentFeature->getSeerShape();
    
    std::vector<uuid> resolvedFaceIds = shape.resolvePick(facePick.shapeHistory);
    if (resolvedFaceIds.empty())
    {
      std::ostringstream stream;
      stream << "DatumPlanePlanarOffset: can't find target face id. Skipping id: " << gu::idToString(facePick.id);
      throw std::runtime_error(stream.str());
    }
    assert(resolvedFaceIds.size() == 1);
    
    const TopoDS_Shape &faceShape = shape.getOCCTShape(resolvedFaceIds.front());
    assert(faceShape.ShapeType() == TopAbs_FACE);
    
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
  osg::Vec3d normal = gu::getZVector(faceSystem) * static_cast<double>(*offset);
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

DatumPlaneConnections DatumPlanePlanarOffset::setUpFromSelection(const slc::Containers &containersIn, const ShapeHistory &historyIn)
{
  //assuming containersIn has been through 'canDoTypes'.
  assert(containersIn.size() == 1);
  assert
  (
    (containersIn.at(0).featureType == Type::DatumPlane) ||
    (containersIn.at(0).selectionType == slc::Type::Face)
  );
  
  facePick.id = containersIn.at(0).shapeId;
  if (containersIn.at(0).selectionType == slc::Type::Face)
    facePick.shapeHistory = historyIn.createDevolveHistory(facePick.id);
  
  DatumPlaneConnections out;
  DatumPlaneConnection connection;
  connection.inputType = ftr::InputType{ftr::InputType::create};
  connection.parentId = containersIn.at(0).featureId;
  out.push_back(connection);
  
  return out;
}

void DatumPlanePlanarOffset::serialOut(prj::srl::SolverChoice& solverChoice)
{
  prj::srl::DatumPlanePlanarOffset srlOffset
  (
    facePick.serialOut(),
    offset->serialOut()
  );
  
  solverChoice.offset() = srlOffset;
}



DatumPlanePlanarCenter::DatumPlanePlanarCenter()
  {}

DatumPlanePlanarCenter::~DatumPlanePlanarCenter()
  {}

osg::Matrixd DatumPlanePlanarCenter::solve(const UpdatePayload::UpdateMap &mapIn)
{
  osg::Matrixd face1System, face2System;
  face1System = face2System = osg::Matrixd::identity();
  double tempRadius = 1.0;
  
  if (mapIn.size() == 1)
  {
    //note: can't do a center with only 1 datum plane. so we know this condition must be a shape.
    UpdatePayload::UpdateMap::const_iterator it = mapIn.find(InputType::create);
    if (it == mapIn.end())
      throw std::runtime_error("DatumPlanePlanarCenter: Conflict between map size and count");
    if (!it->second->hasSeerShape())
      throw std::runtime_error("DatumPlanePlanarCenter: null seer shape");
    const Base* parentFeature = it->second;
    const SeerShape &shape = parentFeature->getSeerShape();
    
    std::vector<uuid> resolvedFaceIds1 = shape.resolvePick(facePick1.shapeHistory);
    if (resolvedFaceIds1.empty())
    {
      std::ostringstream stream;
      stream << "DatumPlanePlanarCenter: can't find target face1 id. Skipping id: " << gu::idToString(facePick1.id);
      throw std::runtime_error(stream.str());
    }
    assert(resolvedFaceIds1.size() == 1);
    
    std::vector<uuid> resolvedFaceIds2 = shape.resolvePick(facePick2.shapeHistory);
    if (resolvedFaceIds2.empty())
    {
      std::ostringstream stream;
      stream << "DatumPlanePlanarCenter: can't find target face2 id. Skipping id: " << gu::idToString(facePick2.id);
      throw std::runtime_error(stream.str());
    }
    assert(resolvedFaceIds2.size() == 1);
    
    TopoDS_Shape face1 = shape.getOCCTShape(resolvedFaceIds1.front()); assert(!face1.IsNull());
    TopoDS_Shape face2 = shape.getOCCTShape(resolvedFaceIds2.front()); assert(!face2.IsNull());
    if (face1.IsNull() || face2.IsNull())
      throw std::runtime_error("DatumPlanePlanarCenter: null faces");
    face1System = getFaceSystem(face1);
    face2System = getFaceSystem(face2);
    
    //calculate size.
    
    tempRadius = std::max(std::sqrt(getBoundingBox(face1).SquareExtent()), std::sqrt(getBoundingBox(face2).SquareExtent())) / 2.0;
  }
  else if (mapIn.size() == 2)
  {
    //with 2 inputs, either one can be a face or a datum.
    double radius1, radius2;
    auto it = mapIn.equal_range(InputType::create);
    assert(std::distance(it.first, it.second) == 2);
    const Base *feature1 = it.first->second;
    it.first++;
    const Base *feature2 = it.first->second;
    
    
    if (feature1->getType() == Type::DatumPlane)
    {
      const DatumPlane *inputPlane = dynamic_cast<const DatumPlane*>(feature1);
      assert(inputPlane);
      face1System = inputPlane->getSystem();
      radius1 = inputPlane->getRadius();
    }
    else
    {
      if (!feature1->hasSeerShape())
        throw std::runtime_error("DatumPlanePlanarCenter: expected seer shape not found");
      const SeerShape &shape1 = feature1->getSeerShape();
      
      std::vector<uuid> resolvedFaceIds1 = shape1.resolvePick(facePick1.shapeHistory);
      if (resolvedFaceIds1.empty())
      {
        resolvedFaceIds1 = shape1.resolvePick(facePick2.shapeHistory);
        if (resolvedFaceIds1.empty())
        {
          std::ostringstream stream;
          stream << "DatumPlanePlanarCenter: can't find target face1";
          throw std::runtime_error(stream.str());
        }
      }
      assert(resolvedFaceIds1.size() == 1);
      
      TopoDS_Shape face1 = shape1.getOCCTShape(resolvedFaceIds1.front());
      if (face1.IsNull())
        throw std::runtime_error("DatumPlanePlanarCenter: input 1 has null shape");
      if (face1.ShapeType() != TopAbs_FACE)
        throw std::runtime_error("DatumPlanePlanarCenter: shape is not of type face");
      face1System = getFaceSystem(face1);
      radius1 = std::sqrt(getBoundingBox(face1).SquareExtent()) / 2.0;
    }
    
    if (feature2->getType() == Type::DatumPlane)
    {
      const DatumPlane *inputPlane = dynamic_cast<const DatumPlane*>(feature2);
      assert(inputPlane);
      face2System = inputPlane->getSystem();
      radius2 = inputPlane->getRadius();
    }
    else
    {
      if (!feature2->hasSeerShape())
        throw std::runtime_error("DatumPlanePlanarCenter: expected seer shape not found");
      const SeerShape &shape2 = feature2->getSeerShape();
      
      std::vector<uuid> resolvedFaceIds2 = shape2.resolvePick(facePick1.shapeHistory);
      if (resolvedFaceIds2.empty())
      {
        resolvedFaceIds2 = shape2.resolvePick(facePick2.shapeHistory);
        if (resolvedFaceIds2.empty())
        {
          std::ostringstream stream;
          stream << "DatumPlanePlanarCenter: can't find target face2";
          throw std::runtime_error(stream.str());
        }
      }
      assert(resolvedFaceIds2.size() == 1);
      
      TopoDS_Shape face2 = shape2.getOCCTShape(resolvedFaceIds2.front());
      if (face2.IsNull())
        throw std::runtime_error("DatumPlanePlanarCenter: input 2 has null shape");
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

DatumPlaneConnections DatumPlanePlanarCenter::setUpFromSelection(const slc::Containers &containersIn, const ShapeHistory &historyIn)
{
  //assume we have been through 'canDoTypes'
  assert(containersIn.size() == 2);
  assert((containersIn.at(0).selectionType == slc::Type::Face) || (containersIn.at(0).featureType == Type::DatumPlane));
  assert((containersIn.at(1).selectionType == slc::Type::Face) || (containersIn.at(1).featureType == Type::DatumPlane));
  
  //note if a datum is selected then the shapeId is nil. Which is what we want.
  facePick1.id = containersIn.at(0).shapeId;
  if (containersIn.at(0).selectionType == slc::Type::Face)
    facePick1.shapeHistory = historyIn.createDevolveHistory(facePick1.id);
  facePick2.id = containersIn.at(1).shapeId;
  if (containersIn.at(1).selectionType == slc::Type::Face)
    facePick2.shapeHistory = historyIn.createDevolveHistory(facePick2.id);
  
  DatumPlaneConnection connection;
  DatumPlaneConnections out;
  if (containersIn.at(0).featureId == containersIn.at(1).featureId)
  {
    connection.inputType = ftr::InputType{ftr::InputType::create};
    connection.parentId = containersIn.at(0).featureId;
    out.push_back(connection);
  }
  else
  {
    connection.inputType = ftr::InputType{ftr::InputType::create};
    connection.parentId = containersIn.at(0).featureId;
    out.push_back(connection);
    connection.inputType = ftr::InputType{ftr::InputType::create};
    connection.parentId = containersIn.at(1).featureId;
    out.push_back(connection);
  }
  return out;
}

void DatumPlanePlanarCenter::serialOut(prj::srl::SolverChoice& solverChoice)
{
  prj::srl::DatumPlanePlanarCenter srlCenter
  (
    facePick1.serialOut(),
    facePick2.serialOut()
  );
  
  solverChoice.center() = srlCenter;
}


DatumPlanePlanarParallelThroughEdge::DatumPlanePlanarParallelThroughEdge(){}

DatumPlanePlanarParallelThroughEdge::~DatumPlanePlanarParallelThroughEdge(){}

osg::Matrixd DatumPlanePlanarParallelThroughEdge::solve(const UpdatePayload::UpdateMap &mapIn)
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
    UpdatePayload::UpdateMap::const_iterator it = mapIn.find(InputType::create);
    if (it == mapIn.end())
      throw std::runtime_error("DatumPlanarParallelThroughEdge: Conflict between map size and count");
    
    if (!it->second->hasSeerShape())
      throw std::runtime_error("DatumPlanarParallelThroughEdge: no seer shape");
    const SeerShape &shape = it->second->getSeerShape();
    
    std::vector<uuid> resolvedFaceIds = shape.resolvePick(facePick.shapeHistory);
    if (resolvedFaceIds.empty())
    {
      std::ostringstream stream;
      stream << "DatumPlanarParallelThroughEdge: can't resolve target face id. Skipping id: " << gu::idToString(facePick.id);
      throw std::runtime_error(stream.str());
    }
    assert(resolvedFaceIds.size() == 1);
    
    std::vector<uuid> resolvedEdgeIds = shape.resolvePick(edgePick.shapeHistory);
    if (resolvedEdgeIds.empty())
    {
      std::ostringstream stream;
      stream << "DatumPlanarParallelThroughEdge: can't resolve target edge id. Skipping id: " << gu::idToString(edgePick.id);
      throw std::runtime_error(stream.str());
    }
    assert(resolvedEdgeIds.size() == 1);
    
    const TopoDS_Shape &face = shape.getOCCTShape(resolvedFaceIds.front()); assert(!face.IsNull()); assert(face.ShapeType() == TopAbs_FACE);
    const TopoDS_Shape &edge = shape.getOCCTShape(resolvedEdgeIds.front()); assert(!edge.IsNull()); assert(edge.ShapeType() == TopAbs_EDGE);
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
    bool foundPlane = false;
    bool foundEdge = false;
    auto it = mapIn.equal_range(InputType::create);
    assert(std::distance(it.first, it.second) == 2);
    const Base *feature1 = it.first->second;
    it.first++;
    const Base *feature2 = it.first->second;
    
    if (feature1->getType() == Type::DatumPlane)
    {
      const DatumPlane *dPlane = dynamic_cast<const DatumPlane*>(feature1);
      assert(dPlane);
      outSystem = dPlane->getSystem();
      tempRadius = dPlane->getRadius();
      foundPlane = true;
    }
    else
    {
      if (!feature1->hasSeerShape())
        throw std::runtime_error("DatumPlanarParallelThroughEdge: expected seer shape not found");
      const SeerShape &shape = feature1->getSeerShape();
      
      std::vector<uuid> resolvedId1;
      if (!facePick.id.is_nil()) //when datum is selected shape id is nil.
        resolvedId1 = shape.resolvePick(facePick.shapeHistory);
      if (resolvedId1.empty())
      {
        if (!edgePick.id.is_nil()) //future datum axis will have nil id.
        {
          resolvedId1 = shape.resolvePick(edgePick.shapeHistory);
          if (resolvedId1.empty())
          {
            std::ostringstream stream;
            stream << "DatumPlanarParallelThroughEdge: can't resolve id 1.";
            throw std::runtime_error(stream.str());
          }
          else
          {
            assert(resolvedId1.size() == 1);
            const TopoDS_Shape &edge = shape.getOCCTShape(resolvedId1.front()); assert(!edge.IsNull());
            direction = getEdgeVector(edge);
            origin = getOrigin(edge);
            foundEdge = true;
          }
        }
      }
      else
      {
        assert(resolvedId1.size() == 1);
        const TopoDS_Shape &face = shape.getOCCTShape(resolvedId1.front()); assert(!face.IsNull());
        outSystem = getFaceSystem(face);
        tempRadius = std::sqrt(getBoundingBox(face).SquareExtent()) / 2.0;
        foundPlane = true;
      }
    }
    
    if (feature2->getType() == Type::DatumPlane)
    {
      const DatumPlane *dPlane = dynamic_cast<const DatumPlane*>(feature2);
      assert(dPlane);
      outSystem = dPlane->getSystem();
      tempRadius = dPlane->getRadius();
      foundPlane = true;
    }
    else
    {
      if (!feature2->hasSeerShape())
        throw std::runtime_error("DatumPlanarParallelThroughEdge: no seer shape");
      const SeerShape &shape = feature2->getSeerShape();
      
      std::vector<uuid> resolvedId2;
      if (!facePick.id.is_nil()) //when datum is selected shape id is nil.
        resolvedId2 = shape.resolvePick(facePick.shapeHistory);
      if (resolvedId2.empty())
      {
        if (!edgePick.id.is_nil()) //future datum axis will have nil id.
        {
          resolvedId2 = shape.resolvePick(edgePick.shapeHistory);
          if (resolvedId2.empty())
          {
            std::ostringstream stream;
            stream << "DatumPlanarParallelThroughEdge: can't resolve id 2.";
            throw std::runtime_error(stream.str());
          }
          else
          {
            assert(resolvedId2.size() == 1);
            const TopoDS_Shape &edge = shape.getOCCTShape(resolvedId2.front()); assert(!edge.IsNull());
            direction = getEdgeVector(edge);
            origin = getOrigin(edge);
            foundEdge = true;
          }
        }
      }
      else
      {
        assert(resolvedId2.size() == 1);
        const TopoDS_Shape &face = shape.getOCCTShape(resolvedId2.front()); assert(!face.IsNull());
        outSystem = getFaceSystem(face);
        tempRadius = std::sqrt(getBoundingBox(face).SquareExtent()) / 2.0;
        foundPlane = true;
      }
    }
    
    if (!foundPlane || !foundEdge)
      throw std::runtime_error("DatumPlanarParallelThroughEdge: couldn't find edge or plane");
    
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

DatumPlaneConnections DatumPlanePlanarParallelThroughEdge::setUpFromSelection(const slc::Containers &containersIn, const ShapeHistory &historyIn)
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
    connection.inputType = ftr::InputType{InputType::create};
    connection.parentId = containersIn.at(0).featureId;
    out.push_back(connection);
    
    //we know we don't have a datum because we would have to have different feature ids.
    if (containersIn.at(0).selectionType == slc::Type::Face)
      facePick.id = containersIn.at(0).shapeId;
    if (containersIn.at(0).selectionType == slc::Type::Edge)
      edgePick.id = containersIn.at(0).shapeId;
    if (containersIn.at(1).selectionType == slc::Type::Face)
      facePick.id = containersIn.at(1).shapeId;
    if (containersIn.at(1).selectionType == slc::Type::Edge)
      edgePick.id = containersIn.at(1).shapeId;
    
    facePick.shapeHistory = historyIn.createDevolveHistory(facePick.id);
    edgePick.shapeHistory = historyIn.createDevolveHistory(edgePick.id);
    
    return out;
  }
  
  for (const auto &container : containersIn)
  {
    if (container.selectionType == slc::Type::Face)
    {
      facePick.id = container.shapeId;
      facePick.shapeHistory = historyIn.createDevolveHistory(facePick.id);
      connection.inputType = ftr::InputType{InputType::create};
      connection.parentId = container.featureId;
      out.push_back(connection);
    }
    else if(container.selectionType == slc::Type::Edge)
    {
      edgePick.id = container.shapeId;
      edgePick.shapeHistory = historyIn.createDevolveHistory(edgePick.id);
      connection.inputType = ftr::InputType{InputType::create};
      connection.parentId = container.featureId;
      out.push_back(connection);
    }
    else if(container.featureType == Type::DatumPlane)
    {
      facePick.id = container.shapeId; //will be nil
      //no shape history?
      connection.inputType = ftr::InputType{InputType::create};
      connection.parentId = container.featureId;
      out.push_back(connection);
    }
  }
  
  return out;
}

void DatumPlanePlanarParallelThroughEdge::serialOut(prj::srl::SolverChoice& solverChoice)
{
  prj::srl::DatumPlanePlanarParallelThroughEdge srlParallel
  (
    facePick.serialOut(),
    edgePick.serialOut()
  );
  
  solverChoice.parallelThroughEdge() = srlParallel;
}

//default constructed plane is 2.0 x 2.0
DatumPlane::DatumPlane() : Base()
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionDatumPlane.svg");
  
  name = QObject::tr("Datum Plane");
  mainSwitch->setUserValue(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  radius = std::make_unique<prm::Parameter>(QObject::tr("Radius"), 1.0);
  radius->connectValue(boost::bind(&DatumPlane::setVisualDirty, this));
  parameterVector.push_back(radius.get());
  
  autoSize = std::make_unique<prm::Parameter>(QObject::tr("Auto Size"), true);
  autoSize->connectValue(boost::bind(&DatumPlane::setVisualDirty, this));
  parameterVector.push_back(autoSize.get());
  
  display = new mdv::DatumPlane();
  
  transform = new osg::MatrixTransform();
  transform->addChild(display.get());
  
  mainSwitch->addChild(transform.get());
  updateGeometry();
  
  seerShape.reset();
}

DatumPlane::~DatumPlane()
{

}

void DatumPlane::setSolver(std::shared_ptr<DatumPlaneGenre> solverIn)
{
  solver = solverIn;
  lbr::IPGroup *g = solver->getIPGroup();
  if (g)
  {
    overlaySwitch->addChild(g);
    parameterVector.push_back(g->getParameter());
  }
  solver->connect(this);
}

void DatumPlane::updateModel(const UpdatePayload &payloadIn)
{
  osg::Matrixd matrix(osg::Matrixd::identity());
  osg::Vec3d oldXAxis(1.0, 0.0, 0.0);
  osg::Vec3d newXAxis(1.0, 1.0, 1.0);
  newXAxis.normalize();
  matrix *= osg::Matrixd::rotate(oldXAxis, newXAxis);
  transform->setMatrix(matrix);
  
  setFailure();
  lastUpdateLog.clear();
  try
  {
    if (!solver)
      throw std::runtime_error("no solver");
    
    transform->setMatrix(solver->solve(payloadIn.updateMap));
    radius->setValue(solver->radius);
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in datum plane update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in datum plane update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in datum plane update." << std::endl;
    lastUpdateLog += s.str();
  }
  
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void DatumPlane::updateVisual()
{
  if (static_cast<bool>(*autoSize))
    updateGeometry();
  setVisualClean();
}

//serial support for datum plane needs to be done.
void DatumPlane::serialWrite(const QDir &dIn)
{
  osg::Matrixd m = transform->getMatrix();
  prj::srl::Matrixd matrixOut
  (
    m(0,0), m(0,1), m(0,2), m(0,3),
    m(1,0), m(1,1), m(1,2), m(1,3),
    m(2,0), m(2,1), m(2,2), m(2,3),
    m(3,0), m(3,1), m(3,2), m(3,3)
  );
  
  prj::srl::SolverChoice solverChoice;
  solver->serialOut(solverChoice);
  
  prj::srl::FeatureDatumPlane datumPlaneOut
  (
    Base::serialOut(),
    radius->serialOut(),
    autoSize->serialOut(),
    matrixOut,
    solverChoice
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::datumPlane(stream, datumPlaneOut, infoMap);
}

void DatumPlane::serialRead(const prj::srl::FeatureDatumPlane &datumPlaneIn)
{
  Base::serialIn(datumPlaneIn.featureBase());
  radius->serialIn(datumPlaneIn.radius());
  autoSize->serialIn(datumPlaneIn.autoSize());
  
  const prj::srl::Matrixd &s = datumPlaneIn.matrix();
  osg::Matrixd m;
  m(0,0) = s.i0j0(); m(0,1) = s.i0j1(); m(0,2) = s.i0j2(); m(0,3) = s.i0j3();
  m(1,0) = s.i1j0(); m(1,1) = s.i1j1(); m(1,2) = s.i1j2(); m(1,3) = s.i1j3();
  m(2,0) = s.i2j0(); m(2,1) = s.i2j1(); m(2,2) = s.i2j2(); m(2,3) = s.i2j3();
  m(3,0) = s.i3j0(); m(3,1) = s.i3j1(); m(3,2) = s.i3j2(); m(3,3) = s.i3j3();
  
  if (datumPlaneIn.solverChoice().offset().present())
  {
    std::shared_ptr<DatumPlanePlanarOffset> offsetSolver(new DatumPlanePlanarOffset);
    offsetSolver->facePick.serialIn(datumPlaneIn.solverChoice().offset().get().facePick());
    offsetSolver->offset->serialIn(datumPlaneIn.solverChoice().offset().get().offset());
    setSolver(offsetSolver);
    
    //update the interactive parameter.
    osg::Matrixd base(m);
    osg::Vec3d normal = -gu::getZVector(m) * static_cast<double>(*(offsetSolver->offset));
    base.setTrans(m.getTrans() + normal);
    offsetSolver->offsetIP->setMatrix(base);
    offsetSolver->offsetIP->valueHasChanged();
    offsetSolver->offsetIP->constantHasChanged();
  }
  else if(datumPlaneIn.solverChoice().center().present())
  {
    std::shared_ptr<DatumPlanePlanarCenter> centerSolver(new DatumPlanePlanarCenter);
    centerSolver->facePick1.serialIn(datumPlaneIn.solverChoice().center().get().facePick1());
    centerSolver->facePick2.serialIn(datumPlaneIn.solverChoice().center().get().facePick2());
    setSolver(centerSolver);
  }
  else if(datumPlaneIn.solverChoice().parallelThroughEdge().present())
  {
    std::shared_ptr<DatumPlanePlanarParallelThroughEdge> parallelSolver(new DatumPlanePlanarParallelThroughEdge);
    parallelSolver->facePick.serialIn(datumPlaneIn.solverChoice().parallelThroughEdge().get().facePick());
    parallelSolver->edgePick.serialIn(datumPlaneIn.solverChoice().parallelThroughEdge().get().edgePick());
    setSolver(parallelSolver);
  }
  
  transform->setMatrix(m);
  updateGeometry();
}

void DatumPlane::updateGeometry()
{
  double r = static_cast<double>(*radius);
  display->setParameters(-r, r, -r, r);
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

QTextStream& DatumPlane::getInfo(QTextStream &streamIn) const 
{
    Inherited::getInfo(streamIn);
    
    streamIn << "Solver type is: " << getDatumPlaneTypeString(solver->getType()) << endl;
    gu::osgMatrixOut(streamIn, transform->getMatrix());
    
    return streamIn;
}

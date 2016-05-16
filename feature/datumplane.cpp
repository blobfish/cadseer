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

#include <QDir>

#include <BRepAdaptor_Surface.hxx>
#include <TopoDS.hxx>
#include <gp_Pln.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>

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
  if (faceId.is_nil())
    throw std::runtime_error("DatumPlanePlanarOffset: nil faceId");
  
  UpdateMap::const_iterator it = mapIn.find(InputTypes::DatumPlanarOffset);
  if (it == mapIn.end())
    throw std::runtime_error("DatumPlanePlanarOffset: no input feature");
  
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
  osg::Matrixd faceSystem = gu::toOsg(tempSystem);
  
  //calculate parameter boundaries and project onto plane.
  gp_Pnt centerPoint, cornerPoint;
  centerRadius(getBoundingBox(faceShape), centerPoint, cornerPoint);
  double centerDistance = adaptor.Plane().Distance(centerPoint);
  double cornerDistance = adaptor.Plane().Distance(cornerPoint);
  osg::Vec3d workVector = gu::getZVector(faceSystem);
  osg::Vec3d centerVec = gu::toOsg(centerPoint) + (workVector * centerDistance);
  osg::Vec3d cornerVec = gu::toOsg(cornerPoint) + (workVector * cornerDistance);
  faceSystem.setTrans(centerVec);
  
  osg::Matrixd datumSystem = faceSystem;
  osg::Vec3d normal = gu::getZVector(faceSystem) * offset->getValue();
  datumSystem.setTrans(faceSystem.getTrans() + normal);
  
  osg::Vec3d offsetVec = centerVec * osg::Matrixd::inverse(faceSystem);
  double radius = (centerVec - cornerVec).length();
  xmin = offsetVec.x() - radius;
  xmax = offsetVec.x() + radius;
  ymin = offsetVec.y() - radius;
  ymax = offsetVec.y() + radius;
  
  //update the interactive parameter.
  offsetIP->setMatrix(faceSystem);
  
//   osg::Matrix lMatrix(osg::Matrixd::identity());
//   lMatrix.setRotate(osg::Quat(osg::PI_2, osg::Vec3d(0.0, 0.0, 1.0)));
//   offsetIP->setMatrixDims(lMatrix);
  
  return datumSystem;
}

lbr::IPGroup* DatumPlanePlanarOffset::getIPGroup()
{
  return offsetIP.get();
}

void DatumPlanePlanarOffset::connect(Base *baseIn)
{
  offset->connectValue(boost::bind(&Base::setModelDirty, baseIn));
}

//default constructed plane is 2.0 x 2.0
DatumPlane::DatumPlane() : Base(), xmin(-0.5), xmax(0.5), ymin(-0.5), ymax(0.5)
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionDatumPlane.svg");
  
  name = QObject::tr("Datum Plane");
  
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
  //temp for testing.
  xmin = -5.0;
  xmax = 5.0;
  ymin = -5.0;
  ymax = 5.0;
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
    xmin = solver->xmin;
    xmax = solver->xmax;
    ymin = solver->ymin;
    ymax = solver->ymax;
    
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
  updateGeometry();
  setVisualClean();
}

void DatumPlane::serialWrite(const QDir &directoryIn)
{
//   ftr::Base::serialWrite(directoryIn);
}

void DatumPlane::updateGeometry()
{
  display->setParameters(xmin, xmax, ymin, ymax);
  
}

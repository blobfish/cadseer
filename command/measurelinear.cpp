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

#include <QTextStream>

#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>

#include <osg/AutoTransform>

#include <globalutilities.h>
#include <application/mainwindow.h>
#include <viewer/widget.h>
#include <project/project.h>
#include <message/observer.h>
#include <message/message.h>
#include <selection/eventhandler.h>
#include <feature/base.h>
#include <feature/seershape.h>
#include <library/lineardimension.h>
#include <command/measurelinear.h>

using namespace cmd;

MeasureLinear::MeasureLinear() : Base()
{
  setupDispatcher();
  observer->name = "cmd::MeasureLinear";
}

MeasureLinear::~MeasureLinear(){}

std::string MeasureLinear::getStatusMessage()
{
  return QObject::tr("Select 2 objects for measure linear").toStdString();
}

void MeasureLinear::activate()
{
  isActive = true;
  go();
}

void MeasureLinear::deactivate()
{
  isActive = false;
}

void MeasureLinear::go()
{
  auto getShape = [&](const slc::Container &cIn) -> TopoDS_Shape
  {
    if (slc::isPointType(cIn.selectionType))
    {
      TopoDS_Vertex v = BRepBuilderAPI_MakeVertex(gp_Pnt(gu::toOcc(cIn.pointLocation).XYZ()));
      return v;
    }
    
    ftr::Base *fb = project->findFeature(cIn.featureId);
    if (!fb->hasSeerShape() || fb->getSeerShape().isNull())
      return TopoDS_Shape();
    if (cIn.shapeId.is_nil())
      return fb->getSeerShape().getRootOCCTShape();
    return fb->getSeerShape().getOCCTShape(cIn.shapeId);
  };
  
  const slc::Containers &containers = eventHandler->getSelections();
  if (containers.size() != 2)
    return;
  TopoDS_Shape s1 = getShape(containers.front());
  TopoDS_Shape s2 = getShape(containers.back());
  if(s1.IsNull() || s2.IsNull())
    return;
  
  BRepExtrema_DistShapeShape ext(s1, s2, Extrema_ExtFlag_MIN);
  if (!ext.IsDone() || ext.NbSolution() < 1)
  {
    std::cout << "extrema failed in MeasureLinear::go" << std::endl;
    return;
  }
  observer->outBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  osg::Vec3d point1(gu::toOsg(ext.PointOnShape1(1)));
  osg::Vec3d point2(gu::toOsg(ext.PointOnShape2(1)));
  build(point1, point2);
  //don't send done. this causes a loop and continues to measure until user hits esc.
//   sendDone();
}

void MeasureLinear::build(const osg::Vec3d &point1, const osg::Vec3d &point2)
{
  double distance = (point2 - point1).length();
    
  QString infoMessage;
  QTextStream stream(&infoMessage);
  stream << endl;
  forcepoint(stream)
      << qSetRealNumberPrecision(12)
      << "Point1 location: ["
      << point1.x() << ", "
      << point1.y() << ", "
      << point1.z() << "]"
      << endl
      << "Point2 location: ["
      << point2.x() << ", "
      << point2.y() << ", "
      << point2.z() << "]"
      << endl
      <<"Length: "
      << distance
      <<endl;
  msg::Message viewInfoMessage(msg::Request | msg::Info | msg::Text);
  app::Message appMessage;
  appMessage.infoMessage = infoMessage;
  viewInfoMessage.payload = appMessage;
  observer->outBlocked(viewInfoMessage);
  
  if (distance < std::numeric_limits<float>::epsilon())
    return;
  
  //get the view matrix for orientation.
  osg::Matrixd viewMatrix = mainWindow->getViewer()->getViewSystem();
  osg::Vec3d yVector = point2 - point1; yVector.normalize();
  osg::Vec3d zVectorView = gu::getZVector(viewMatrix); zVectorView.normalize();
  osg::Vec3d xVector = zVectorView ^ yVector;
  if (xVector.isNaN())
  {
    observer->outBlocked(msg::buildStatusMessage(
      QObject::tr("Can't make dimension with current view direction").toStdString()));
    return;
  }
  xVector.normalize();
  //got to be an easier way!
  osg::Vec3d zVector  = xVector ^ yVector;
  zVector.normalize();
  osg::Matrixd transform
  (
      xVector.x(), xVector.y(), xVector.z(), 0.0,
      yVector.x(), yVector.y(), yVector.z(), 0.0,
      zVector.x(), zVector.y(), zVector.z(), 0.0,
      0.0, 0.0, 0.0, 1.0
  );
  
  //probably should be somewhere else.
  osg::ref_ptr<osg::AutoTransform> autoTransform = new osg::AutoTransform();
  autoTransform->setPosition(point1);
  autoTransform->setAutoRotateMode(osg::AutoTransform::ROTATE_TO_AXIS);
  autoTransform->setAxis(yVector);
  autoTransform->setNormal(-zVector);
  
  osg::ref_ptr<lbr::LinearDimension> dim = new lbr::LinearDimension();
  dim->setMatrix(transform);
  dim->setColor(osg::Vec4d(0.8, 0.0, 0.0, 1.0));
  dim->setSpread((point2 - point1).length());
  autoTransform->addChild(dim.get());
  
  msg::Message message(msg::Request | msg::Add | msg::Overlay);
  vwr::Message vwrMessage;
  vwrMessage.node = autoTransform;
  message.payload = vwrMessage;
  observer->outBlocked(message);
}

void MeasureLinear::setupDispatcher()
{
  msg::Mask lm;
  
  lm = msg::Response | msg::Post | msg::Selection | msg::Add;
  observer->dispatcher.insert(std::make_pair(lm, boost::bind
    (&MeasureLinear::selectionAdditionDispatched, this, _1)));
}

void MeasureLinear::selectionAdditionDispatched(const msg::Message&)
{
  if (!isActive)
    return;
  
  go();
}

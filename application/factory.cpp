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

#include <iostream>
#include <sstream>
#include <assert.h>

#include <QFileDialog>
#include <QTextStream>
#include <QUrl>
#include <QDesktopServices>
#include <QMessageBox>

#include <boost/uuid/uuid.hpp>
#include <boost/timer/timer.hpp>
#include <boost/variant.hpp>
#include <boost/filesystem.hpp>

#include <BRepTools.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopoDS.hxx>
#include <STEPControl_Writer.hxx>
#include <STEPControl_Reader.hxx>
#include <APIHeaderSection_MakeHeader.hxx>

#include <osgDB/WriteFile>

#include <tools/idtools.h>
#include <message/dispatch.h>
#include <message/message.h>
#include <message/observer.h>
#include <project/project.h>
#include <application/application.h>
#include <application/mainwindow.h>
#include <viewer/widget.h>
#include <viewer/message.h>
#include <dialogs/preferences.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <annex/seershape.h>
#include <feature/types.h>
#include <feature/box.h>
#include <feature/oblong.h>
#include <feature/cylinder.h>
#include <feature/sphere.h>
#include <feature/cone.h>
#include <feature/union.h>
#include <feature/subtract.h>
#include <feature/intersect.h>
#include <feature/chamfer.h>
#include <feature/draft.h>
#include <feature/datumplane.h>
#include <feature/hollow.h>
#include <feature/inert.h>
#include <library/lineardimension.h>
#include <selection/visitors.h>
#include <application/factory.h>

using namespace app;
using boost::uuids::uuid;

Factory::Factory()
{
  observer = std::move(std::unique_ptr<msg::Observer>(new msg::Observer()));
  observer->name = "app::Factory";
  setupDispatcher();
}

void Factory::setupDispatcher()
{
  msg::Mask mask;
  
  //main dispatcher.
  mask = msg::Response | msg::Post | msg::New | msg::Project;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::newProjectDispatched, this, _1)));
  
  mask = msg::Response | msg::Post | msg::Open | msg::Project;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::openProjectDispatched, this, _1)));
  
  mask = msg::Response | msg::Pre | msg::Close | msg::Project;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::closeProjectDispatched, this, _1)));
  
  mask = msg::Response | msg::Post | msg::Selection | msg::Add;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::selectionAdditionDispatched, this, _1)));
  
  mask = msg::Response | msg::Pre | msg::Selection | msg::Remove;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::selectionSubtractionDispatched, this, _1)));
  
  mask = msg::Request | msg::Construct | msg::Box;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::newBoxDispatched, this, _1)));
  
  mask = msg::Request | msg::Construct | msg::Oblong;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::newOblongDispatched, this, _1)));
  
  mask = msg::Request | msg::Construct | msg::Cylinder;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::newCylinderDispatched, this, _1)));
  
  mask = msg::Request | msg::Construct | msg::Sphere;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::newSphereDispatched, this, _1)));
  
  mask = msg::Request | msg::Construct | msg::Cone;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::newConeDispatched, this, _1)));
  
  mask = msg::Request | msg::Construct | msg::Union;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::newUnionDispatched, this, _1)));
  
  mask = msg::Request | msg::Construct | msg::Subtract;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::newSubtractDispatched, this, _1)));
  
  mask = msg::Request | msg::Construct | msg::Intersect;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::newIntersectDispatched, this, _1)));
  
  mask = msg::Request | msg::Construct | msg::Chamfer;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::newChamferDispatched, this, _1)));
  
  mask = msg::Request | msg::Construct | msg::Draft;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::newDraftDispatched, this, _1)));
  
  mask = msg::Request | msg::Construct | msg::DatumPlane;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::newDatumPlaneDispatched, this, _1)));
  
  mask = msg::Request | msg::Import | msg::OCC;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::importOCCDispatched, this, _1)));
  
  mask = msg::Request | msg::Export | msg::OCC;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::exportOCCDispatched, this, _1)));
  
  mask = msg::Request | msg::Import | msg::Step;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::importStepDispatched, this, _1)));
  
  mask = msg::Request | msg::Export | msg::Step;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::exportStepDispatched, this, _1)));
  
  mask = msg::Request | msg::Preferences;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::preferencesDispatched, this, _1)));
  
  mask = msg::Request | msg::Remove;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::removeDispatched, this, _1)));
  
  mask = msg::Request | msg::DebugDump;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::debugDumpDispatched, this, _1)));
  
  mask = msg::Request | msg::DebugShapeTrackUp;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::debugShapeTrackUpDispatched, this, _1)));
  
  mask = msg::Request | msg::DebugShapeTrackDown;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::debugShapeTrackDownDispatched, this, _1)));
  
  mask = msg::Request | msg::DebugShapeGraph;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::debugShapeGraphDispatched, this, _1)));
  
  mask = msg::Request | msg::Info;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::viewInfoDispatched, this, _1)));
  
  mask = msg::Request | msg::Feature | msg::Model | msg::Dirty;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::featureModelDirtyDispatched, this, _1)));
  
  mask = msg::Request | msg::Construct | msg::Hollow;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::newHollowDispatched, this, _1)));
  
  mask = msg::Request | msg::DebugInquiry;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::osgToDotTestDispatched, this, _1)));
}

void Factory::newProjectDispatched(const msg::Message& /*messageIn*/)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  app::Application *application = dynamic_cast<app::Application *>(qApp);
  assert(application);
  project = application->getProject();
}

void Factory::openProjectDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  app::Application *application = dynamic_cast<app::Application *>(qApp);
  assert(application);
  project = application->getProject();
}

void Factory::closeProjectDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  project = nullptr;
}

void Factory::selectionAdditionDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  slc::Message sMessage = boost::get<slc::Message>(messageIn.payload);
  slc::Container aContainer;
  aContainer.selectionType = sMessage.type;
  aContainer.featureId = sMessage.featureId;
  aContainer.featureType = sMessage.featureType;
  aContainer.shapeId = sMessage.shapeId;
  aContainer.pointLocation = sMessage.pointLocation;
  containers.push_back(aContainer);
}

void Factory::selectionSubtractionDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  slc::Message sMessage = boost::get<slc::Message>(messageIn.payload);
  slc::Container aContainer;
  aContainer.selectionType = sMessage.type;
  aContainer.featureId = sMessage.featureId;
  aContainer.featureType = sMessage.featureType;
  aContainer.shapeId = sMessage.shapeId;
  aContainer.pointLocation = sMessage.pointLocation;
  
  slc::Containers::iterator it = std::find(containers.begin(), containers.end(), aContainer);
  assert(it != containers.end());
  containers.erase(it);
}

void Factory::newBoxDispatched(const msg::Message &)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  assert(project);
  
  app::Application *application = dynamic_cast<app::Application *>(qApp);
  assert(application);
  const osg::Matrixd &currentSystem = application->getMainWindow()->getViewer()->getCurrentSystem();
  
  std::shared_ptr<ftr::Box> boxPtr(new ftr::Box());
  boxPtr->setCSys(currentSystem);
  project->addFeature(boxPtr);
  
  observer->out(msg::Mask(msg::Request | msg::Update));
}

void Factory::newOblongDispatched(const msg::Message &)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  assert(project);
  
  app::Application *application = dynamic_cast<app::Application *>(qApp);
  assert(application);
  const osg::Matrixd &currentSystem = application->getMainWindow()->getViewer()->getCurrentSystem();
  
  std::shared_ptr<ftr::Oblong> oblongPtr(new ftr::Oblong());
  oblongPtr->setCSys(currentSystem);
  project->addFeature(oblongPtr);
  
  observer->out(msg::Mask(msg::Request | msg::Update));
}

void Factory::newCylinderDispatched(const msg::Message &)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  assert(project);
  
  app::Application *application = dynamic_cast<app::Application *>(qApp);
  assert(application);
  const osg::Matrixd &currentSystem = application->getMainWindow()->getViewer()->getCurrentSystem();
  
  std::shared_ptr<ftr::Cylinder> cylinder(new ftr::Cylinder());
  cylinder->setCSys(currentSystem);
  project->addFeature(cylinder);
  
  observer->out(msg::Mask(msg::Request | msg::Update));
}

void Factory::newSphereDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  assert(project);
  
  app::Application *application = dynamic_cast<app::Application *>(qApp);
  assert(application);
  const osg::Matrixd &currentSystem = application->getMainWindow()->getViewer()->getCurrentSystem();
  
  std::shared_ptr<ftr::Sphere> sphere(new ftr::Sphere());
  sphere->setCSys(currentSystem);
  project->addFeature(sphere);
  
  observer->out(msg::Mask(msg::Request | msg::Update));
}

void Factory::newConeDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  assert(project);
  
  app::Application *application = dynamic_cast<app::Application *>(qApp);
  assert(application);
  const osg::Matrixd &currentSystem = application->getMainWindow()->getViewer()->getCurrentSystem();
  
  std::shared_ptr<ftr::Cone> cone(new ftr::Cone());
  cone->setCSys(currentSystem);
  project->addFeature(cone);
  
  observer->out(msg::Mask(msg::Request | msg::Update));
}

void Factory::newUnionDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  if (containers.size() < 2)
  {
    observer->out(msg::buildStatusMessage(
      QObject::tr("Wrong selection count for operation").toStdString()));
    return;
  }
  
  std::vector<uuid> featureIds;
  //for now only accept objects.
  for (const auto &selection : containers)
  {
    featureIds.push_back(selection.featureId);
    if (selection.selectionType != slc::Type::Object)
    {
      observer->out(msg::buildStatusMessage(
        QObject::tr("Wrong selection type for operation").toStdString()));
      return;
    }
  }
    
  assert(project);
  
  for (const auto &id : featureIds)
  {
    observer->outBlocked(msg::buildHideThreeD(id));
    observer->outBlocked(msg::buildHideOverlay(id));
  }
  
  //union keyword. whoops
  std::shared_ptr<ftr::Union> onion(new ftr::Union());
  project->addFeature(onion);
  project->connect(featureIds.at(0), onion->getId(), ftr::InputType{ftr::InputType::target});
  for (auto it = featureIds.begin() + 1; it != featureIds.end(); ++it)
    project->connect(*it, onion->getId(), ftr::InputType{ftr::InputType::tool});
  
  onion->setColor(project->findFeature(featureIds.at(0))->getColor());
  
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
  observer->out(msg::Mask(msg::Request | msg::Update));
}

void Factory::newSubtractDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  if (containers.size() < 2)
  {
    observer->out(msg::buildStatusMessage(
      QObject::tr("Wrong selection count for operation").toStdString()));
    return;
  }
  
  std::vector<uuid> featureIds;
  //for now only accept objects.
  for (const auto &selection : containers)
  {
    featureIds.push_back(selection.featureId);
    if (selection.selectionType != slc::Type::Object)
    {
      observer->out(msg::buildStatusMessage(
        QObject::tr("Wrong selection type for operation").toStdString()));
      return;
    }
  }
    
  assert(project);
  
  for (const auto &id : featureIds)
  {
    observer->outBlocked(msg::buildHideThreeD(id));
    observer->outBlocked(msg::buildHideOverlay(id));
  }
  
  std::shared_ptr<ftr::Subtract> subtract(new ftr::Subtract());
  project->addFeature(subtract);
  project->connect(featureIds.at(0), subtract->getId(), ftr::InputType{ftr::InputType::target});
  for (auto it = featureIds.begin() + 1; it != featureIds.end(); ++it)
    project->connect(*it, subtract->getId(), ftr::InputType{ftr::InputType::tool});
  
  subtract->setColor(project->findFeature(featureIds.at(0))->getColor());
  
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
  observer->out(msg::Mask(msg::Request | msg::Update));
}

void Factory::newIntersectDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  if (containers.size() < 2)
  {
    observer->out(msg::buildStatusMessage(
      QObject::tr("Wrong selection count for operation").toStdString()));
    return;
  }
  
  std::vector<uuid> featureIds;
  //for now only accept objects.
  for (const auto &selection : containers)
  {
    featureIds.push_back(selection.featureId);
    if (selection.selectionType != slc::Type::Object)
    {
      observer->out(msg::buildStatusMessage(
        QObject::tr("Wrong selection type for operation").toStdString()));
      return;
    }
  }
    
  assert(project);
  
  for (const auto &id : featureIds)
  {
    observer->outBlocked(msg::buildHideThreeD(id));
    observer->outBlocked(msg::buildHideOverlay(id));
  }
  
  std::shared_ptr<ftr::Intersect> intersect(new ftr::Intersect());
  project->addFeature(intersect);
  project->connect(featureIds.at(0), intersect->getId(), ftr::InputType{ftr::InputType::target});
  for (auto it = featureIds.begin() + 1; it != featureIds.end(); ++it)
    project->connect(*it, intersect->getId(), ftr::InputType{ftr::InputType::tool});
  
  intersect->setColor(project->findFeature(featureIds.at(0))->getColor());
  
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
  observer->out(msg::Mask(msg::Request | msg::Update));
}

void Factory::newChamferDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  assert(project);
  
  if (containers.empty())
    return;
  uuid targetFeatureId = containers.at(0).featureId;
  
  ftr::SymChamfer symChamfer;
  symChamfer.distance = ftr::Chamfer::buildSymParameter();
  const ann::SeerShape &targetSeerShape = project->findFeature(targetFeatureId)->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
  for (const auto &currentSelection : containers)
  {
    if
    (
      currentSelection.featureId != targetFeatureId ||
      currentSelection.selectionType != slc::Type::Edge //just edges for now.
    )
      continue;
      
    TopoDS_Edge edge = TopoDS::Edge(targetSeerShape.findShapeIdRecord(currentSelection.shapeId).shape);  
    ftr::ChamferPick pick;
    pick.edgePick.id = currentSelection.shapeId;
    pick.edgePick.setParameter(edge, currentSelection.pointLocation);
    pick.edgePick.shapeHistory = project->getShapeHistory().createDevolveHistory(pick.edgePick.id);
    pick.facePick.id = ftr::Chamfer::referenceFaceId(targetSeerShape, pick.edgePick.id);
    //for now user doesn't specify face so we don't worry about u, v of facePick.
    pick.facePick.shapeHistory = project->getShapeHistory().createDevolveHistory(pick.facePick.id);
    symChamfer.picks.push_back(pick);
  }
  
  std::shared_ptr<ftr::Chamfer> chamfer(new ftr::Chamfer());
  chamfer->addSymChamfer(symChamfer);
  project->addFeature(chamfer);
  project->connect(targetFeatureId, chamfer->getId(), ftr::InputType{ftr::InputType::target});
  
  ftr::Base *targetFeature = project->findFeature(targetFeatureId);
  chamfer->setColor(targetFeature->getColor());
  
  observer->outBlocked(msg::buildHideThreeD(targetFeatureId));
  observer->outBlocked(msg::buildHideOverlay(targetFeatureId));
  
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
  observer->out(msg::Mask(msg::Request | msg::Update));
}

void Factory::newDraftDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  assert(project);
  
  if (containers.empty())
    return;
  uuid targetFeatureId = containers.at(0).featureId;
  
  ftr::DraftConvey convey;
  const ann::SeerShape &targetSeerShape = project->findFeature(targetFeatureId)->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
  for (const auto &currentSelection : containers)
  {
    if
    (
      currentSelection.featureId != targetFeatureId ||
      currentSelection.selectionType != slc::Type::Face //just edges for now.
    )
      continue;
      
    TopoDS_Face face = TopoDS::Face(targetSeerShape.findShapeIdRecord(currentSelection.shapeId).shape);  
    ftr::Pick pick;
    pick.id = currentSelection.shapeId;
    pick.setParameter(face, currentSelection.pointLocation);
    pick.shapeHistory = project->getShapeHistory().createDevolveHistory(pick.id);
    convey.targets.push_back(pick);
  }
  if (convey.targets.empty())
    return;
  
  //for now last pick is the neutral plane.
  convey.neutralPlane = convey.targets.back();
  convey.targets.pop_back();
  
  std::shared_ptr<ftr::Draft> draft(new ftr::Draft());
  draft->setDraft(convey);
  project->addFeature(draft);
  project->connect(targetFeatureId, draft->getId(), ftr::InputType{ftr::InputType::target});
  
  ftr::Base *targetFeature = project->findFeature(targetFeatureId);
  draft->setColor(targetFeature->getColor());
  
  observer->outBlocked(msg::buildHideThreeD(targetFeatureId));
  observer->outBlocked(msg::buildHideOverlay(targetFeatureId));
  
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
  observer->out(msg::Mask(msg::Request | msg::Update));
}

void Factory::newDatumPlaneDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  assert(project);
  
  if (containers.empty())
    return;
  
  std::vector<std::shared_ptr<ftr::DatumPlaneGenre> > solvers = ftr::DatumPlane::solversFromSelection(containers);
  if (solvers.empty()) //temp. really we should build the feature and go into edit mode.
    return;
  
  std::shared_ptr<ftr::DatumPlane> dPlane(new ftr::DatumPlane());
  project->addFeature(dPlane);
  
  //just use the first one.
  std::shared_ptr<ftr::DatumPlaneGenre> solver = solvers.front();
  dPlane->setSolver(solver);
  ftr::DatumPlaneConnections connections = solver->setUpFromSelection(containers, project->getShapeHistory());
  
  for (const auto &connection : connections)
    project->connect(connection.parentId, dPlane->getId(), connection.inputType);

  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
  observer->out(msg::Mask(msg::Request | msg::Update));
}

void Factory::newHollowDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  assert(project);
  
  if (containers.empty())
    return;
  
  uuid targetFeatureId = containers.at(0).featureId;
  ftr::Base *targetFeature = project->findFeature(targetFeatureId);
  assert(targetFeature->hasAnnex(ann::Type::SeerShape));
  const ann::SeerShape &targetSeerShape = targetFeature->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
  
  ftr::Picks hollowPicks;
  for (const auto &currentSelection : containers)
  {
    if
    (
      currentSelection.featureId != targetFeatureId ||
      currentSelection.selectionType != slc::Type::Face
    )
      continue;
      
    ftr::Pick hPick;
    hPick.id = currentSelection.shapeId;
    hPick.shapeHistory = project->getShapeHistory().createDevolveHistory(hPick.id);
    TopoDS_Face face = TopoDS::Face(targetSeerShape.findShapeIdRecord(currentSelection.shapeId).shape);
    hPick.setParameter(face, currentSelection.pointLocation);
    hollowPicks.push_back(hPick);
  }
  if (hollowPicks.empty())
    return;
  
  std::shared_ptr<ftr::Hollow> hollow(new ftr::Hollow());
  hollow->setHollowPicks(hollowPicks);
  project->addFeature(hollow);
  project->connect(targetFeatureId, hollow->getId(), ftr::InputType{ftr::InputType::target});
  hollow->setColor(targetFeature->getColor());
  
  observer->outBlocked(msg::buildHideThreeD(targetFeatureId));
  observer->outBlocked(msg::buildHideOverlay(targetFeatureId));
  
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
  observer->out(msg::Mask(msg::Request | msg::Update));
}

void Factory::importOCCDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  app::Application *application = dynamic_cast<app::Application *>(qApp);
  assert(application);
  
  QString fileName = QFileDialog::getOpenFileName
  (
    application->getMainWindow(),
    QObject::tr("Open File"),
    QString::fromStdString(prf::manager().rootPtr->project().lastDirectory().get()),
    QObject::tr("brep (*.brep *.brp)")
  );
  if (fileName.isEmpty())
      return;
  
  assert(project);
  project->readOCC(fileName.toStdString());
  
  boost::filesystem::path p = fileName.toStdString();
  prf::manager().rootPtr->project().lastDirectory() = p.remove_filename().string();
  prf::manager().saveConfig();
  
  observer->out(msg::Mask(msg::Request | msg::Update));
}

void Factory::exportOCCDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  if 
  (
    (containers.empty()) ||
    (containers.at(0).selectionType != slc::Type::Object)
  )
    return;
  
  app::Application *application = dynamic_cast<app::Application *>(qApp);
  assert(application);
  QString fileName = QFileDialog::getSaveFileName
  (
    application->getMainWindow(),
    QObject::tr("Save File"),
    QString::fromStdString(prf::manager().rootPtr->project().lastDirectory().get()),
    QObject::tr("brep (*.brep *.brp)")
  );
  if (fileName.isEmpty())
      return;
  if
  (
    (!fileName.endsWith(QObject::tr(".brep"))) &&
    (!fileName.endsWith(QObject::tr(".brp")))
  )
    fileName += QObject::tr(".brep");
    
  boost::filesystem::path p = fileName.toStdString();
  prf::manager().rootPtr->project().lastDirectory() = p.remove_filename().string();
  prf::manager().saveConfig();
    
  assert(project);
  
  ftr::Base *f = project->findFeature(containers.at(0).featureId);
  if (f->hasAnnex(ann::Type::SeerShape))
  {
    const ann::SeerShape &sShape = f->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
    if (!sShape.isNull())
      BRepTools::Write(sShape.getRootOCCTShape(), fileName.toStdString().c_str());
  }
    
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

void Factory::importStepDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  //get file name
  app::Application *application = dynamic_cast<app::Application *>(qApp);
  assert(application);
  assert(project);
  
  QString fileName = QFileDialog::getOpenFileName
  (
    application->getMainWindow(),
    QObject::tr("Import Step File"),
    QString::fromStdString(prf::manager().rootPtr->project().lastDirectory().get()),
    QObject::tr("step (*.step *.stp)")
  );
  if (fileName.isEmpty())
    return;
  
  boost::filesystem::path p = fileName.toStdString();
  prf::manager().rootPtr->project().lastDirectory() = p.remove_filename().string();
  prf::manager().saveConfig();
  
  app::WaitCursor wc; //show busy.
  
  STEPControl_Reader scr; //step control reader.
  if (scr.ReadFile(fileName.toUtf8().constData()) != IFSelect_RetDone)
  {
    QMessageBox::critical(application->getMainWindow(), QObject::tr("Error"), QObject::tr("failed reading step file"));
    return;
  }
  
  //todo check units!
  
  scr.TransferRoots();
  
  int nos = scr.NbShapes(); //number of shapes.
  if (nos < 1)
  {
    QMessageBox::critical(application->getMainWindow(), QObject::tr("Error"), QObject::tr("no shapes in step file"));
    return;
  }
  int si = 0; //shapes imported
  for (int i = 1; i < nos + 1; ++i)
  {
    TopoDS_Shape s = scr.Shape(i);
    if (s.IsNull())
      continue;
    
    //not sure how the reader is determining what is a 'root' shape.
    //but the sample I have is 2 independent solids, but is coming through
    //as 1 root. so iterate and create different features.
    for (TopoDS_Iterator it(s); it.More(); it.Next())
    {
      std::shared_ptr<ftr::Inert> inert(new ftr::Inert(it.Value()));
      project->addFeature(inert);
      si++;
    }
  }
  
  observer->out(msg::Mask(msg::Request | msg::Update));
  
  std::ostringstream m;
  m << si << " shapes imported";
  observer->out(msg::buildStatusMessage(m.str()));
}

void Factory::exportStepDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  if 
  (
    (containers.empty()) ||
    (containers.at(0).selectionType != slc::Type::Object)
  )
  {
    observer->out(msg::buildStatusMessage("Incorrect selection"));
    return;
  }
    
  app::Application *application = dynamic_cast<app::Application *>(qApp);
  assert(application);
  
  QString fileName = QFileDialog::getSaveFileName
  (
    application->getMainWindow(),
    QObject::tr("Save File"),
    QString::fromStdString(prf::manager().rootPtr->project().lastDirectory().get()),
    QObject::tr("step (*.step *.stp)")
  );
  if (fileName.isEmpty())
    return;
  if
  (
    (!fileName.endsWith(QObject::tr(".step"))) &&
    (!fileName.endsWith(QObject::tr(".stp")))
  )
    fileName += QObject::tr(".step");
    
  boost::filesystem::path p = fileName.toStdString();
  prf::manager().rootPtr->project().lastDirectory() = p.remove_filename().string();
  prf::manager().saveConfig();
    
  assert(project);
  
  //for now just get the first shape from selection.
  ftr::Base *feature = project->findFeature(containers.at(0).featureId);
  if (!feature->hasAnnex(ann::Type::SeerShape))
  {
    observer->out(msg::buildStatusMessage("feature doesn't have SeerShape"));
    return;
  }
  const ann::SeerShape &sShape = feature->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
  if (sShape.isNull())
  {
    observer->out(msg::buildStatusMessage("SeerShape is null"));
    return;
  }
  const TopoDS_Shape &shape = sShape.getRootOCCTShape();
  if (shape.IsNull())
  {
    observer->out(msg::buildStatusMessage("OCCT Shape is null"));
    return;
  }
  
  STEPControl_Writer stepOut;
  if (stepOut.Transfer(shape, STEPControl_AsIs) != IFSelect_RetDone)
  {
    observer->out(msg::buildStatusMessage("Step translation failed"));
    return;
  }
  
  std::string author = prf::manager().rootPtr->project().gitName();
  APIHeaderSection_MakeHeader header(stepOut.Model());
  header.SetName(new TCollection_HAsciiString(fileName.toUtf8().data()));
  header.SetOriginatingSystem(new TCollection_HAsciiString("CadSeer"));
  header.SetAuthorValue (1, new TCollection_HAsciiString(author.c_str()));
  header.SetOrganizationValue (1, new TCollection_HAsciiString(author.c_str()));
  header.SetDescriptionValue(1, new TCollection_HAsciiString(feature->getName().toUtf8().data()));
  header.SetAuthorisation(new TCollection_HAsciiString(author.c_str()));
  
  if (stepOut.Write(fileName.toStdString().c_str()) != IFSelect_RetDone)
  {
    observer->out(msg::buildStatusMessage("Step write failed"));
    return;
  }
  
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

void Factory::preferencesDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  app::Application *application = dynamic_cast<app::Application *>(qApp);
  assert(application);
  std::unique_ptr<dlg::Preferences> dialog(new dlg::Preferences(&prf::manager(), application->getMainWindow()));
  dialog->setModal(true);
  if (!dialog->exec())
    return;
  if (dialog->isVisualDirty())
  {
    assert(project);
    project->setAllVisualDirty();
    project->updateVisual();
  }
  else if(dialog->isHiddenLinesDirty())
  {
    if (prf::manager().rootPtr->visual().display().showHiddenLines())
      observer->outBlocked(msg::Message(msg::Request | msg::View | msg::Show | msg::HiddenLine));
    else
      observer->outBlocked(msg::Message(msg::Request | msg::View | msg::Hide | msg::HiddenLine));
  }
  
  msg::Message prfResponse(msg::Response | msg::Preferences);
  observer->outBlocked(prfResponse);
}

void Factory::removeDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  //containers is wired up message so it will be changing as we delete(remove from selection)
  //so cache a copy to work with first.
  slc::Containers selection = containers;
  
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  for (const auto &current : selection)
  {
    if (current.selectionType != slc::Type::Object)
      continue;
    msg::Message removeMessage;
    removeMessage.mask = msg::Request | msg::Remove | msg::Feature;
    prj::Message payload;
    payload.featureIds.push_back(current.featureId);
    removeMessage.payload = payload;
    observer->out(removeMessage);
  }
  
  observer->out(msg::Mask(msg::Request | msg::Update));
}

void Factory::debugDumpDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  assert(project);
  if (containers.empty())
    return;
  
  std::cout << std::endl << std::endl << "begin debug dump:" << std::endl;
  
  for (const auto &container : containers)
  {
    if (container.selectionType != slc::Type::Object)
      continue;
    ftr::Base *feature = project->findFeature(container.featureId);
    assert(feature);
    if (!feature->hasAnnex(ann::Type::SeerShape))
      continue;
    const ann::SeerShape &seerShape = feature->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
    std::cout << std::endl;
    std::cout << "feature name: " << feature->getName().toStdString() << "    feature id: " << gu::idToString(feature->getId()) << std::endl;
    std::cout << "shape id container:" << std::endl; seerShape.dumpShapeIdContainer(std::cout); std::cout << std::endl;
    std::cout << "shape evolve container:" << std::endl; seerShape.dumpEvolveContainer(std::cout); std::cout << std::endl;
    std::cout << "feature tag container:" << std::endl; seerShape.dumpFeatureTagContainer(std::cout); std::cout << std::endl;
  }
  
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

void Factory::debugShapeTrackUpDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  assert(project);
  if (containers.empty())
    return;
  
  for (const auto &container : containers)
  {
    if (container.shapeId.is_nil())
      continue;
    project->shapeTrackUp(container.shapeId);
  }
  
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

void Factory::debugShapeTrackDownDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  assert(project);
  if (containers.empty())
    return;
  
  for (const auto &container : containers)
  {
    if (container.shapeId.is_nil())
      continue;
    project->shapeTrackDown(container.shapeId);
  }
  
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

void Factory::debugShapeGraphDispatched(const msg::Message&)
{
    std::ostringstream debug;
    debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
    msg::dispatch().dumpString(debug.str());
    
    assert(project);
    if (containers.empty())
        return;
    
    QString fileNameBase = static_cast<app::Application*>(qApp)->
        getApplicationDirectory().absolutePath() + QDir::separator();
    for (const auto &container : containers)
    {
        if (container.selectionType != slc::Type::Object)
            continue;
        ftr::Base *feature = project->findFeature(container.featureId);
        if (!feature->hasAnnex(ann::Type::SeerShape))
            continue;
        QString fileName = fileNameBase + QString::fromStdString(gu::idToString(feature->getId())) + ".dot";
        const ann::SeerShape &shape = feature->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
        shape.dumpGraph(fileName.toStdString());
        
        QDesktopServices::openUrl(QUrl(fileName));
    }
    
    observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

void Factory::viewInfoDispatched(const msg::Message &)
{
  QString infoMessage;
  QTextStream stream(&infoMessage);
  stream.setRealNumberNotation(QTextStream::FixedNotation);
  forcepoint(stream) << qSetRealNumberPrecision(12);
  stream << endl;
  
  vwr::Widget *viewer = static_cast<app::Application*>(qApp)->getMainWindow()->getViewer();
  osg::Matrixd ics = osg::Matrixd::inverse(viewer->getCurrentSystem()); //inverted current system.
  
  auto streamPoint = [&](const osg::Vec3d &p)
  {
    stream
      << "Absolute Point location: "
      << "["
      << p.x() << ", "
      << p.y() << ", "
      << p.z() << "]"
      << endl;
      
    osg::Vec3d tp = p * ics;
    stream
      << "Current System Point location: "
      << "["
      << tp.x() << ", "
      << tp.y() << ", "
      << tp.z() << "]"
      << endl;
  };
  
  
  
    //maybe if selection is empty we will dump out application information.
    //or better yet project information or even better yet both. careful
    //we might want to turn the window on and off keeping information as a 
    //reference and we don't to fill the window with shit. we will be alright, 
    //we will have a different facility for show and hiding the info window
    //other than this command.
    assert(project);
    if (containers.empty())
    {
      //nothing selected so get app and project information.
      //TODO get git hash for application version.
      project->getInfo(stream);
      static_cast<app::Application*>(qApp)->getMainWindow()->getViewer()->getInfo(stream);
    }
    else
    {
      for (const auto &container : containers)
      {
          ftr::Base *feature = project->findFeature(container.featureId);
          stream <<  endl;
          if
          (
              (container.selectionType == slc::Type::Object) ||
              (container.selectionType == slc::Type::Feature)
          )
          {
              feature->getInfo(stream);
          }
          else if
          (
              (container.selectionType == slc::Type::Solid) ||
              (container.selectionType == slc::Type::Shell) ||
              (container.selectionType == slc::Type::Face) ||
              (container.selectionType == slc::Type::Wire) ||
              (container.selectionType == slc::Type::Edge)
          )
          {
              feature->getShapeInfo(stream, container.shapeId);
          }
          else if (container.selectionType == slc::Type::StartPoint)
          {
              const ann::SeerShape &s = feature->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
              feature->getShapeInfo(stream, s.useGetStartVertex(container.shapeId));
              streamPoint(container.pointLocation);
          }
          else if (container.selectionType == slc::Type::EndPoint)
          {
            const ann::SeerShape &s = feature->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
            feature->getShapeInfo(stream, s.useGetEndVertex(container.shapeId));
            streamPoint(container.pointLocation);
          }
          else //all other points.
          {
            streamPoint(container.pointLocation);
          }
      }
    }
    
    msg::Message viewInfoMessage(msg::Request | msg::Info | msg::Text);
    app::Message appMessage;
    appMessage.infoMessage = infoMessage;
    viewInfoMessage.payload = appMessage;
    observer->out(viewInfoMessage);
}

void Factory::featureModelDirtyDispatched(const msg::Message&)
{
  assert(project);
  for (const auto &container : containers)
    project->findFeature(container.featureId)->setModelDirty();
}

//! a testing function to analyze run time cost of messages.
void Factory::messageStressTestDispatched(const msg::Message&)
{
//   msg::dispatch().dumpConnectionCount(); //has 30 at this time.
  
  std::cout << std::endl;
  //test mask. shouldn't match any observer.
  msg::Mask test = msg::Request | msg::Response | msg::Pre | msg::Post;
  std::vector<std::unique_ptr<msg::Observer>> observers;
  
  {
    std::cout << std::endl << "approx. 1000 observers" << std::endl;
    for (std::size_t index = 0; index < 1000; ++index)
      observers.push_back(std::unique_ptr<msg::Observer>(new msg::Observer()));
    boost::timer::auto_cpu_timer t;
    observer->out(msg::Message(test));
  }
  
  {
    std::cout << std::endl << "approx. 10000 observers" << std::endl;
    for (std::size_t index = 0; index < 9000; ++index)
      observers.push_back(std::unique_ptr<msg::Observer>(new msg::Observer()));
    boost::timer::auto_cpu_timer t;
    observer->out(msg::Message(test));
  }
  
  {
    std::cout << std::endl << "approx. 100000 observers" << std::endl;
    for (std::size_t index = 0; index < 90000; ++index)
      observers.push_back(std::unique_ptr<msg::Observer>(new msg::Observer()));
    boost::timer::auto_cpu_timer t;
    observer->out(msg::Message(test));
  }
  
  {
    std::cout << std::endl << "approx. 1000000 observers" << std::endl;
    for (std::size_t index = 0; index < 900000; ++index)
      observers.push_back(std::unique_ptr<msg::Observer>(new msg::Observer()));
    boost::timer::auto_cpu_timer t;
    observer->out(msg::Message(test));
  }
  
  /*
   * output:
   
   approx. 1000 observers
   0 .000766s wall, 0.000000s user + 0.000000s system = 0.000000s CPU (n/a%)
   
   approx. 10000 observers
   0.008040s wall, 0.010000s user + 0.000000s system = 0.010000s CPU (124.4%)
   
   approx. 100000 observers
   0.072139s wall, 0.070000s user + 0.000000s system = 0.070000s CPU (97.0%)
   
   approx. 1000000 observers
   0.758043s wall, 0.760000s user + 0.000000s system = 0.760000s CPU (100.3%)
   
   This looks linear. Keep in mind these test observers have no function dispatching.
   This is OK, because the main reason behind this test is for individual feature
   observers, which I don't expect to have any function dispatching.
   */
}

void Factory::osgToDotTestDispatched(const msg::Message&)
{
  assert(project);
  if (containers.empty())
    return;
  
  ftr::Base *feature = project->findFeature(containers.front().featureId);
  
  QString fileName = static_cast<app::Application *>(qApp)->getApplicationDirectory().path();
  fileName += QDir::separator();
  fileName += "osg";
  osgDB::Options options;
  options.setOptionString("rankdir = TB;");
  
  osg::Switch *overlay = feature->getOverlaySwitch();
  std::vector<lbr::LinearDimension*> dimensions;
  slc::TypedAccrueVisitor<lbr::LinearDimension> visitor(dimensions);
  overlay->accept(visitor);
  
  for (unsigned int i = 0; i < dimensions.size(); ++i)
  {
    QString cFileName = fileName + "_" + QString::number(i) + ".dot";
    osgDB::writeNodeFile(*(dimensions.at(i)), cFileName.toStdString(), &options);
  }
  
//   QDesktopServices::openUrl(QUrl(fileName));
}

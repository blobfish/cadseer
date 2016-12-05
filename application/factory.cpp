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

#include <boost/uuid/uuid.hpp>

#include <BRepTools.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS.hxx>

#include <tools/idtools.h>
#include <message/dispatch.h>
#include <message/message.h>
#include <message/observer.h>
#include <project/project.h>
#include <application/application.h>
#include <application/mainwindow.h>
#include <viewer/viewerwidget.h>
#include <preferences/dialog.h>
#include <application/factory.h>
#include <preferences/manager.h>
#include <feature/seershape.h>
#include <feature/types.h>
#include <feature/box.h>
#include <feature/cylinder.h>
#include <feature/sphere.h>
#include <feature/cone.h>
#include <feature/union.h>
#include <feature/subtract.h>
#include <feature/intersect.h>
#include <feature/blend.h>
#include <feature/chamfer.h>
#include <feature/draft.h>
#include <feature/datumplane.h>

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
  mask = msg::Response | msg::Post | msg::NewProject;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::newProjectDispatched, this, _1)));
  
  mask = msg::Response | msg::Post | msg::OpenProject;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::openProjectDispatched, this, _1)));
  
  mask = msg::Response | msg::Pre | msg::CloseProject;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::closeProjectDispatched, this, _1)));
  
  mask = msg::Response | msg::Post | msg::Selection | msg::Addition;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::selectionAdditionDispatched, this, _1)));
  
  mask = msg::Response | msg::Pre | msg::Selection | msg::Subtraction;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::selectionSubtractionDispatched, this, _1)));
  
  mask = msg::Request | msg::Construct | msg::Box;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::newBoxDispatched, this, _1)));
  
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
  
  mask = msg::Request | msg::Construct | msg::Blend;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::newBlendDispatched, this, _1)));
  
  mask = msg::Request | msg::Construct | msg::Chamfer;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::newChamferDispatched, this, _1)));
  
  mask = msg::Request | msg::Construct | msg::Draft;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::newDraftDispatched, this, _1)));
  
  mask = msg::Request | msg::Construct | msg::DatumPlane;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::newDatumPlaneDispatched, this, _1)));
  
  mask = msg::Request | msg::ImportOCC;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::importOCCDispatched, this, _1)));
  
  mask = msg::Request | msg::ExportOCC;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::exportOCCDispatched, this, _1)));
  
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
  
  mask = msg::Request | msg::ViewInfo;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::viewInfoDispatched, this, _1)));
}

void Factory::triggerUpdate()
{
  msg::Message updateMessage;
  updateMessage.mask = msg::Request | msg::Update;
  observer->messageOutSignal(updateMessage);
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
  boxPtr->setSystem(currentSystem);
  boxPtr->updateDragger();
  boxPtr->setParameters(20.0, 10.0, 2.0);
  project->addFeature(boxPtr);
  
  triggerUpdate();
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
  cylinder->setRadius(2.0);
  cylinder->setHeight(8.0);
  cylinder->setSystem(currentSystem);
  cylinder->updateDragger();
  project->addFeature(cylinder);
  
  triggerUpdate();
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
  sphere->setRadius(4.0);
  sphere->setSystem(currentSystem);
  sphere->updateDragger();
  project->addFeature(sphere);
  
  triggerUpdate();
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
  cone->setRadius1(2.0);
  cone->setRadius2(0.5);
  cone->setHeight(8.0);
  cone->setSystem(currentSystem);
  cone->updateDragger();
  project->addFeature(cone);
  
  triggerUpdate();
}

void Factory::newUnionDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  if (containers.size() < 2)
  {
    msg::Message wrongTypeMessage;
    wrongTypeMessage.mask = msg::Request | msg::StatusText;
    vwr::Message statusMessage;
    statusMessage.text = QObject::tr("Wrong selection count for operation").toStdString();
    wrongTypeMessage.payload = statusMessage;
    observer->messageOutSignal(wrongTypeMessage);
    return;
  }
  
  std::vector<uuid> featureIds;
  //for now only accept objects.
  for (const auto &selection : containers)
  {
    featureIds.push_back(selection.featureId);
    if (selection.selectionType != slc::Type::Object)
    {
      msg::Message wrongTypeMessage;
      wrongTypeMessage.mask = msg::Request | msg::StatusText;
      vwr::Message statusMessage;
      statusMessage.text = QObject::tr("Wrong selection type for operation").toStdString();
      wrongTypeMessage.payload = statusMessage;
      observer->messageOutSignal(wrongTypeMessage);
      return;
    }
  }
    
  assert(project);
  
  for (const auto &id : featureIds)  
    project->findFeature(id)->hide3D();
  
  //union keyword. whoops
  std::shared_ptr<ftr::Union> onion(new ftr::Union());
  project->addFeature(onion);
  project->connect(featureIds.at(0), onion->getId(), ftr::InputTypes::target);
  for (auto it = featureIds.begin() + 1; it != featureIds.end(); ++it)
    project->connect(*it, onion->getId(), ftr::InputTypes::tool);
  
  msg::Message clearSelectionMessage;
  clearSelectionMessage.mask = msg::Request | msg::Selection | msg::Clear;
  observer->messageOutSignal(clearSelectionMessage);
  
  triggerUpdate();
}

void Factory::newSubtractDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  if (containers.size() < 2)
  {
    msg::Message wrongTypeMessage;
    wrongTypeMessage.mask = msg::Request | msg::StatusText;
    vwr::Message statusMessage;
    statusMessage.text = QObject::tr("Wrong selection count for operation").toStdString();
    wrongTypeMessage.payload = statusMessage;
    observer->messageOutSignal(wrongTypeMessage);
    return;
  }
  
  std::vector<uuid> featureIds;
  //for now only accept objects.
  for (const auto &selection : containers)
  {
    featureIds.push_back(selection.featureId);
    if (selection.selectionType != slc::Type::Object)
    {
      msg::Message wrongTypeMessage;
      wrongTypeMessage.mask = msg::Request | msg::StatusText;
      vwr::Message statusMessage;
      statusMessage.text = QObject::tr("Wrong selection type for operation").toStdString();
      wrongTypeMessage.payload = statusMessage;
      observer->messageOutSignal(wrongTypeMessage);
      return;
    }
  }
    
  assert(project);
  
  for (const auto &id : featureIds)  
    project->findFeature(id)->hide3D();
  
  std::shared_ptr<ftr::Subtract> subtract(new ftr::Subtract());
  project->addFeature(subtract);
  project->connect(featureIds.at(0), subtract->getId(), ftr::InputTypes::target);
  for (auto it = featureIds.begin() + 1; it != featureIds.end(); ++it)
    project->connect(*it, subtract->getId(), ftr::InputTypes::tool);
  
  msg::Message clearSelectionMessage;
  clearSelectionMessage.mask = msg::Request | msg::Selection | msg::Clear;
  observer->messageOutSignal(clearSelectionMessage);
  
  triggerUpdate();
}

void Factory::newIntersectDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  if (containers.size() < 2)
  {
    msg::Message wrongTypeMessage;
    wrongTypeMessage.mask = msg::Request | msg::StatusText;
    vwr::Message statusMessage;
    statusMessage.text = QObject::tr("Wrong selection count for operation").toStdString();
    wrongTypeMessage.payload = statusMessage;
    observer->messageOutSignal(wrongTypeMessage);
    return;
  }
  
  std::vector<uuid> featureIds;
  //for now only accept objects.
  for (const auto &selection : containers)
  {
    featureIds.push_back(selection.featureId);
    if (selection.selectionType != slc::Type::Object)
    {
      msg::Message wrongTypeMessage;
      wrongTypeMessage.mask = msg::Request | msg::StatusText;
      vwr::Message statusMessage;
      statusMessage.text = QObject::tr("Wrong selection type for operation").toStdString();
      wrongTypeMessage.payload = statusMessage;
      observer->messageOutSignal(wrongTypeMessage);
      return;
    }
  }
    
  assert(project);
  
  for (const auto &id : featureIds)  
    project->findFeature(id)->hide3D();
  
  std::shared_ptr<ftr::Intersect> intersect(new ftr::Intersect());
  project->addFeature(intersect);
  project->connect(featureIds.at(0), intersect->getId(), ftr::InputTypes::target);
  for (auto it = featureIds.begin() + 1; it != featureIds.end(); ++it)
    project->connect(*it, intersect->getId(), ftr::InputTypes::tool);
  
  msg::Message clearSelectionMessage;
  clearSelectionMessage.mask = msg::Request | msg::Selection | msg::Clear;
  observer->messageOutSignal(clearSelectionMessage);
  
  triggerUpdate();
}

void Factory::newBlendDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  assert(project);
  
  if (containers.empty())
    return;
  
  //get targetId and filter out edges not belonging to first target.
  uuid targetFeatureId = containers.at(0).featureId;
  const ftr::SeerShape &targetSeerShape = project->findFeature(targetFeatureId)->getSeerShape();
  ftr::SimpleBlend simpleBlend;
  ftr::VariableBlend vBlend;
  for (const auto &currentSelection : containers)
  {
    if
    (
      currentSelection.featureId != targetFeatureId ||
      currentSelection.selectionType != slc::Type::Edge //just edges for now.
    )
      continue;
    
    TopoDS_Edge edge = TopoDS::Edge(targetSeerShape.getOCCTShape(currentSelection.shapeId));  
    ftr::BlendPick pick;
    pick.id = currentSelection.shapeId;
    pick.u = ftr::Blend::calculateUParameter(edge, currentSelection.pointLocation);
    
    //simple radius test  
    simpleBlend.picks.push_back(pick);
    auto simpleRadius = ftr::Blend::buildRadiusParameter();
    simpleRadius->setValue(2.0);
    simpleBlend.radius = simpleRadius;
    
    //variable blend radius test. really shouldn't be in loop.
//     vBlend = ftr::Blend::buildDefaultVariable(project->findFeature(targetFeatureId)->getResultContainer(), pick);
  }
  if (simpleBlend.picks.empty() && vBlend.entries.empty())
    return;
  
  assert(project);
  
  std::shared_ptr<ftr::Blend> blend(new ftr::Blend());
  project->addFeature(blend);
  project->connect(targetFeatureId, blend->getId(), ftr::InputTypes::target);
  if (!simpleBlend.picks.empty())
    blend->addSimpleBlend(simpleBlend);
  if (!vBlend.entries.empty())
    blend->addVariableBlend(vBlend);
  
  ftr::Base *targetFeature = project->findFeature(targetFeatureId);
  targetFeature->hide3D();
  
  msg::Message clearSelectionMessage;
  clearSelectionMessage.mask = msg::Request | msg::Selection | msg::Clear;
  observer->messageOutSignal(clearSelectionMessage);
  
  triggerUpdate();
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
  const ftr::SeerShape &targetSeerShape = project->findFeature(targetFeatureId)->getSeerShape();
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
    pick.edgeId = currentSelection.shapeId;
    pick.u = ftr::Chamfer::calculateUParameter(edge, currentSelection.pointLocation);
    pick.faceId = ftr::Chamfer::referenceFaceId(targetSeerShape, pick.edgeId);
    symChamfer.picks.push_back(pick);
  }
  
  std::shared_ptr<ftr::Chamfer> chamfer(new ftr::Chamfer());
  chamfer->addSymChamfer(symChamfer);
  project->addFeature(chamfer);
  project->connect(targetFeatureId, chamfer->getId(), ftr::InputTypes::target);
  
  ftr::Base *targetFeature = project->findFeature(targetFeatureId);
  targetFeature->hide3D();
  
  msg::Message clearSelectionMessage;
  clearSelectionMessage.mask = msg::Request | msg::Selection | msg::Clear;
  observer->messageOutSignal(clearSelectionMessage);
  
  triggerUpdate();
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
  const ftr::SeerShape &targetSeerShape = project->findFeature(targetFeatureId)->getSeerShape();
  for (const auto &currentSelection : containers)
  {
    if
    (
      currentSelection.featureId != targetFeatureId ||
      currentSelection.selectionType != slc::Type::Face //just edges for now.
    )
      continue;
      
    TopoDS_Face face = TopoDS::Face(targetSeerShape.findShapeIdRecord(currentSelection.shapeId).shape);  
    ftr::DraftPick pick;
    pick.faceId = currentSelection.shapeId;
    ftr::Draft::calculateUVParameter(face, currentSelection.pointLocation, pick.u, pick.v);
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
  project->connect(targetFeatureId, draft->getId(), ftr::InputTypes::target);
  
  ftr::Base *targetFeature = project->findFeature(targetFeatureId);
  targetFeature->hide3D();
  
  msg::Message clearSelectionMessage;
  clearSelectionMessage.mask = msg::Request | msg::Selection | msg::Clear;
  observer->messageOutSignal(clearSelectionMessage);
  
  triggerUpdate();
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
  ftr::DatumPlaneConnections connections = solver->setUpFromSelection(containers);
  
  for (const auto &connection : connections)
    project->connect(connection.parentId, dPlane->getId(), connection.inputType);

  msg::Message clearSelectionMessage;
  clearSelectionMessage.mask = msg::Request | msg::Selection | msg::Clear;
  observer->messageOutSignal(clearSelectionMessage);
  
  triggerUpdate();
}

void Factory::importOCCDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  app::Application *application = dynamic_cast<app::Application *>(qApp);
  assert(application);
  
  QString fileName = QFileDialog::getOpenFileName(application->getMainWindow(), QObject::tr("Open File"),
						QDir::homePath(), QObject::tr("brep (*.brep *.brp)"));
  if (fileName.isEmpty())
      return;
  
  assert(project);
  project->readOCC(fileName.toStdString());
  
  triggerUpdate();
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
  QString fileName = QFileDialog::getSaveFileName(application->getMainWindow(), QObject::tr("Save File"),
			  QDir::homePath(), QObject::tr("brep (*.brep *.brp)"));
  if (fileName.isEmpty())
      return;
  if
  (
    (!fileName.endsWith(QObject::tr(".brep"))) &&
    (!fileName.endsWith(QObject::tr(".brp")))
  )
    fileName += QObject::tr(".brep");
    
  assert(project);
    
  BRepTools::Write(project->findFeature(containers.at(0).featureId)->getShape(), fileName.toStdString().c_str());
  msg::Message clearSelectionMessage;
  clearSelectionMessage.mask = msg::Request | msg::Selection | msg::Clear;
  observer->messageOutSignal(clearSelectionMessage);
}

void Factory::preferencesDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  app::Application *application = dynamic_cast<app::Application *>(qApp);
  assert(application);
  std::unique_ptr<prf::Dialog> dialog(new prf::Dialog(&prf::manager(), application->getMainWindow()));
  dialog->setModal(true);
  if (!dialog->exec())
    return;
  if (dialog->isVisualDirty())
  {
    assert(project);
    project->setAllVisualDirty();
    project->updateVisual();
  }
  
  msg::Message prfResponse;
  prfResponse.mask = msg::Response | msg::Preferences;
  observer->messageOutSignal(prfResponse);
}

void Factory::removeDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  //containers is wired up message so it will be changing as we delete(remove from selection)
  //so cache a copy to work with first.
  slc::Containers selection = containers;
  
  msg::Message clearSelectionMessage;
  clearSelectionMessage.mask = msg::Request | msg::Selection | msg::Clear;
  observer->messageOutSignal(clearSelectionMessage);
  
  for (const auto &current : selection)
  {
    if (current.selectionType != slc::Type::Object)
      continue;
    msg::Message removeMessage;
    removeMessage.mask = msg::Request  | msg::RemoveFeature;
    prj::Message payload;
    payload.featureId = current.featureId;
    removeMessage.payload = payload;
    observer->messageOutSignal(removeMessage);
  }
  
  triggerUpdate();
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
    if (!feature->hasSeerShape())
      continue;
    const ftr::SeerShape &seerShape = feature->getSeerShape();
    std::cout << std::endl;
    std::cout << "feature name: " << feature->getName().toStdString() << "    feature id: " << gu::idToString(feature->getId()) << std::endl;
    std::cout << "shape id container:" << std::endl; seerShape.dumpShapeIdContainer(std::cout); std::cout << std::endl;
    std::cout << "shape evolve container:" << std::endl; seerShape.dumpEvolveContainer(std::cout); std::cout << std::endl;
    std::cout << "feature tag container:" << std::endl; seerShape.dumpFeatureTagContainer(std::cout); std::cout << std::endl;
  }
  
  msg::Message clearSelectionMessage;
  clearSelectionMessage.mask = msg::Request | msg::Selection | msg::Clear;
  observer->messageOutSignal(clearSelectionMessage);
}

void Factory::debugShapeTrackUpDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  assert(project);
  if (containers.empty())
    return;
  
  std::cout << std::endl << "track shape up" << std::endl;
  
  for (const auto &container : containers)
  {
    if (container.featureId.is_nil() || container.shapeId.is_nil())
      continue;
    project->shapeTrackUp(container.featureId, container.shapeId);
  }
  
  msg::Message clearSelectionMessage;
  clearSelectionMessage.mask = msg::Request | msg::Selection | msg::Clear;
  observer->messageOutSignal(clearSelectionMessage);
}

void Factory::debugShapeTrackDownDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  assert(project);
  if (containers.empty())
    return;
  
  std::cout << std::endl << "track shape down" << std::endl;
  
  for (const auto &container : containers)
  {
    if (container.featureId.is_nil() || container.shapeId.is_nil())
      continue;
    project->shapeTrackDown(container.featureId, container.shapeId);
  }
  
  msg::Message clearSelectionMessage;
  clearSelectionMessage.mask = msg::Request | msg::Selection | msg::Clear;
  observer->messageOutSignal(clearSelectionMessage);
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
        if (!feature->hasSeerShape())
            continue;
        QString fileName = fileNameBase + QString::fromStdString(gu::idToString(feature->getId())) + ".dot";
        const ftr::SeerShape &shape = feature->getSeerShape();
        shape.dumpGraph(fileName.toStdString());
    }
    
    msg::Message clearSelectionMessage;
    clearSelectionMessage.mask = msg::Request | msg::Selection | msg::Clear;
    observer->messageOutSignal(clearSelectionMessage);
}

void Factory::viewInfoDispatched(const msg::Message &)
{
    //maybe if selection is empty we will dump out application information.
    //or better yet project information or even better yet both. careful
    //we might want to turn the window on and off keeping information as a 
    //reference and we don't to fill the window with shit. we will be alright, 
    //we will have a different facility for show and hiding the info window
    //other than this command.
    assert(project);
    if (containers.empty())
        return;
    
    QString infoMessage;
    QTextStream stream(&infoMessage);
    stream << endl;
    for (const auto &container : containers)
    {
        stream <<  endl;
        if
        (
            (container.selectionType == slc::Type::Object) ||
            (container.selectionType == slc::Type::Feature)
        )
        {
            ftr::Base *feature = project->findFeature(container.featureId);
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
            ftr::Base *feature = project->findFeature(container.featureId);
            feature->getShapeInfo(stream, container.shapeId);
        }
        else
        {
            forcepoint(stream)
                << qSetRealNumberPrecision(12)
                << "Point location: ["
                << container.pointLocation.x() << ", "
                << container.pointLocation.y() << ", "
                << container.pointLocation.z() << "]";
        }
    }
    
    msg::Message viewInfoMessage(msg::Response | msg::ViewInfo);
    app::Message appMessage;
    appMessage.infoMessage = infoMessage;
    viewInfoMessage.payload = appMessage;
    observer->messageOutSignal(viewInfoMessage);
}

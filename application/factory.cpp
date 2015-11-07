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

#include <boost/uuid/uuid.hpp>

#include <BRepTools.hxx>

#include <message/dispatch.h>
#include <project/project.h>
#include <application/application.h>
#include <application/mainwindow.h>
#include <preferences/dialog.h>
#include <application/factory.h>
#include <feature/types.h>
#include <feature/box.h>
#include <feature/cylinder.h>
#include <feature/sphere.h>
#include <feature/cone.h>
#include <feature/union.h>
#include <feature/blend.h>

using namespace app;
using boost::uuids::uuid;

Factory::Factory()
{
  setupDispatcher();
  
  this->connectMessageOut(boost::bind(&msg::Dispatch::messageInSlot, &msg::dispatch(), _1));
  msg::dispatch().connectMessageOut(boost::bind(&Factory::messageInSlot, this, _1));
}

void Factory::messageInSlot(const msg::Message &messageIn)
{
  msg::MessageDispatcher::iterator it = dispatcher.find(messageIn.mask);
  if (it == dispatcher.end())
    return;
  
  it->second(messageIn);
}

void Factory::setupDispatcher()
{
  msg::Mask mask;
  
  //main dispatcher.
  mask = msg::Response | msg::Post | msg::NewProject;
  dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::newProjectDispatched, this, _1)));
  
  mask = msg::Response | msg::Post | msg::Selection | msg::Addition;
  dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::selectionAdditionDispatched, this, _1)));
  
  mask = msg::Response | msg::Pre | msg::Selection | msg::Subtraction;
  dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::selectionSubtractionDispatched, this, _1)));
  
  mask = msg::Request | msg::Construct | msg::Box;
  dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::newBoxDispatched, this, _1)));
  
  mask = msg::Request | msg::Construct | msg::Cylinder;
  dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::newCylinderDispatched, this, _1)));
  
  mask = msg::Request | msg::Construct | msg::Sphere;
  dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::newSphereDispatched, this, _1)));
  
  mask = msg::Request | msg::Construct | msg::Cone;
  dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::newConeDispatched, this, _1)));
  
  mask = msg::Request | msg::Construct | msg::Union;
  dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::newUnionDispatched, this, _1)));
  
  mask = msg::Request | msg::Construct | msg::Blend;
  dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::newBlendDispatched, this, _1)));
  
  mask = msg::Request | msg::ImportOCC;
  dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::importOCCDispatched, this, _1)));
  
  mask = msg::Request | msg::ExportOCC;
  dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::exportOCCDispatched, this, _1)));
  
  mask = msg::Request | msg::Preferences;
  dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::preferencesDispatched, this, _1)));
  
  mask = msg::Request | msg::Remove;
  dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::removeDispatched, this, _1)));
}

void Factory::triggerUpdate()
{
  msg::Message updateMessage;
  updateMessage.mask = msg::Request | msg::UpdateModel;
  msg::dispatch().messageInSlot(updateMessage);
  updateMessage.mask = msg::Request | msg::UpdateVisual;
  msg::dispatch().messageInSlot(updateMessage);
}

void Factory::newProjectDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  app::Application *application = dynamic_cast<app::Application *>(qApp);
  assert(application);
  project = application->getProject();
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
  aContainer.shapeId = sMessage.shapeId;
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
  aContainer.shapeId = sMessage.shapeId;
  
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
  
  std::shared_ptr<ftr::Box> boxPtr(new ftr::Box());
  gp_Ax2 location;
  location.SetLocation(gp_Pnt(1.0, 1.0, 1.0));
  boxPtr->setSystem(location);
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
  
  std::shared_ptr<ftr::Cylinder> cylinder(new ftr::Cylinder());
  cylinder->setRadius(2.0);
  cylinder->setHeight(8.0);
  gp_Ax2 location;
  location.SetLocation(gp_Pnt(16.0, 6.0, 2.5));
  cylinder->setSystem(location);
  project->addFeature(cylinder);
  
  triggerUpdate();
}

void Factory::newSphereDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  assert(project);
  
  std::shared_ptr<ftr::Sphere> sphere(new ftr::Sphere());
  sphere->setRadius(4.0);
  gp_Ax2 location;
  location.SetLocation(gp_Pnt(11.0, 6.0, -1.5));
  sphere->setSystem(location);
  project->addFeature(sphere);
  
  triggerUpdate();
}

void Factory::newConeDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  assert(project);
  
  std::shared_ptr<ftr::Cone> cone(new ftr::Cone());
  cone->setRadius1(2.0);
  cone->setRadius2(0.5);
  cone->setHeight(8.0);
  gp_Ax2 location;
  location.SetLocation(gp_Pnt(6.0, 6.0, 2.5));
  cone->setSystem(location);
  project->addFeature(cone);
  
  triggerUpdate();
}

void Factory::newUnionDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  if (containers.size() < 2)
    return;
  
  //for now only accept objects.
  if
  (
    containers.at(0).selectionType != slc::Type::Object ||
    containers.at(1).selectionType != slc::Type::Object
  )
    return;
    
  assert(project);
    
  uuid targetFeatureId = containers.at(0).featureId;
  uuid toolFeatureId = containers.at(1).featureId; //only 1 tool right now.
  
  project->findFeature(targetFeatureId)->hide3D();
  project->findFeature(toolFeatureId)->hide3D();
  
  //union keyword. whoops
  std::shared_ptr<ftr::Union> onion(new ftr::Union());
  project->addFeature(onion);
  project->connect(targetFeatureId, onion->getId(), ftr::InputTypes::target);
  project->connect(toolFeatureId, onion->getId(), ftr::InputTypes::tool);
  
  msg::Message clearSelectionMessage;
  clearSelectionMessage.mask = msg::Request | msg::Selection | msg::Clear;
  messageOutSignal(clearSelectionMessage);
  
  triggerUpdate();
}

void Factory::newBlendDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  if (containers.empty())
    return;
  
  //get targetId and filter out edges not belonging to first target.
  uuid targetFeatureId = containers.at(0).featureId;
  std::vector<uuid> edgeIds;
  for (const auto &currentSelection : containers)
  {
    if
    (
      currentSelection.featureId != targetFeatureId ||
      currentSelection.selectionType != slc::Type::Edge //just edges for now.
    )
      continue;
    
    edgeIds.push_back(currentSelection.shapeId);
  }
  if (edgeIds.empty())
    return;
  
  assert(project);
  
  std::shared_ptr<ftr::Blend> blend(new ftr::Blend());
  project->addFeature(blend);
  project->connect(targetFeatureId, blend->getId(), ftr::InputTypes::target);
  blend->setEdgeIds(edgeIds);
  
  ftr::Base *targetFeature = project->findFeature(targetFeatureId);
  targetFeature->hide3D();
  
  msg::Message clearSelectionMessage;
  clearSelectionMessage.mask = msg::Request | msg::Selection | msg::Clear;
  messageOutSignal(clearSelectionMessage);
  
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
  messageOutSignal(clearSelectionMessage);
}

void Factory::preferencesDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  app::Application *application = dynamic_cast<app::Application *>(qApp);
  assert(application);
  std::unique_ptr<prf::Dialog> dialog(new prf::Dialog(application->getPreferencesManager(), application->getMainWindow()));
  dialog->setModal(true);
  if (!dialog->exec())
    return;
  if (dialog->isVisualDirty())
  {
    assert(project);
    project->setAllVisualDirty();
    project->updateVisual();
  }
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
  messageOutSignal(clearSelectionMessage);
  
  for (const auto &current : selection)
  {
    if (current.selectionType != slc::Type::Object)
      continue;
    msg::Message removeMessage;
    removeMessage.mask = msg::Request  | msg::RemoveFeature;
    prj::Message payload;
    payload.featureId = current.featureId;
    removeMessage.payload = payload;
    messageOutSignal(removeMessage);
  }
  
  triggerUpdate();
}

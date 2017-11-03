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

#include <project/project.h>
#include <feature/csysbase.h>
#include <application/mainwindow.h>
#include <selection/eventhandler.h>
#include <viewer/widget.h>
#include <command/featuretosystem.h>

using namespace cmd;

FeatureToSystem::FeatureToSystem() : Base()
{
//   setupDispatcher();
}

FeatureToSystem::~FeatureToSystem(){}

std::string FeatureToSystem::getStatusMessage()
{
  return QObject::tr("Select feature to reposition to the current coordinate system").toStdString();
}

// void FeatureToSystem::setupDispatcher()
// {
//   msg::Mask mask;
//   
//   mask = msg::Response | msg::Post | msg::Selection | msg::Addition;
//   dispatcher.insert(std::make_pair(mask, boost::bind(&FeatureToSystem::selectionAdditionDispatched, this, _1)));
//   
//   mask = msg::Response | msg::Pre | msg::Selection | msg::Subtraction;
//   dispatcher.insert(std::make_pair(mask, boost::bind(&FeatureToSystem::selectionSubtractionDispatched, this, _1)));
// }
// 
// void FeatureToSystem::selectionAdditionDispatched(const msg::Message &messageIn)
// {
//   if (!isActive)
//     return;
//   
//   slc::Message sMessage = boost::get<slc::Message>(messageIn.payload);
//   
//   if (!slc::has(messages, sMessage))
//     slc::add(messages, sMessage);
//   
//   analyzeSelections();
// }
// 
// void FeatureToSystem::selectionSubtractionDispatched(const msg::Message &messageIn)
// {
//   if (!isActive)
//     return;
//   
//   slc::Message sMessage = boost::get<slc::Message>(messageIn.payload);
//   
//   if (slc::has(messages, sMessage))
//     slc::remove(messages, sMessage);
//   
//   analyzeSelections();
// }

void FeatureToSystem::activate()
{
  isActive = true;
  
  const osg::Matrixd& currentSystem = mainWindow->getViewer()->getCurrentSystem();
  const slc::Containers &containers = eventHandler->getSelections();
  for (const auto &container : containers)
  {
    if (container.selectionType != slc::Type::Object)
      continue;
    ftr::Base *baseFeature = project->findFeature(container.featureId);
    assert(baseFeature);
    ftr::CSysBase *csysFeature = dynamic_cast<ftr::CSysBase*>(baseFeature);
    if (!csysFeature)
      continue;
    csysFeature->setSystem(currentSystem);
    csysFeature->updateDragger();
  }
  
  sendDone();
}

void FeatureToSystem::deactivate()
{
  isActive = false;
}

// void FeatureToSystem::analyzeSelections()
// {
//   ftr::CSysBase *feature = dynamic_cast<ftr::CSysBase*>(project->findFeature(id));
//   assert(feature);
//   feature->setModelDirty();
// }

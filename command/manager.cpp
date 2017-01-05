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

#include <functional>

#include <osg/Geometry> //need this for containers.

#include <command/featuretosystem.h>
#include <command/systemtofeature.h>
#include <command/featuretodragger.h>
#include <command/draggertofeature.h>
#include <command/checkgeometry.h>
#include <command/editcolor.h>
#include <message/dispatch.h>
#include <message/observer.h>
#include <selection/message.h>
#include <selection/definitions.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <viewer/message.h>
#include <command/manager.h>

using namespace cmd;

Manager& cmd::manager()
{
  static Manager localManager;
  return localManager;
}

Manager::Manager()
{
  observer = std::move(std::unique_ptr<msg::Observer>(new msg::Observer()));
  observer->name = "cmd::Manager";
  setupDispatcher();
  
  selectionMask = slc::AllEnabled;
}

void Manager::setupDispatcher()
{
  msg::Mask mask;
  
  mask = msg::Request | msg::Command | msg::Cancel;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::cancelCommandDispatched, this, _1)));
  
  mask = msg::Request | msg::Command | msg::Clear;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::cancelCommandDispatched, this, _1)));
  
  mask = msg::Request | msg::Command | msg::Done;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::doneCommandDispatched, this, _1)));
  
  mask = msg::Response | msg::Selection | msg::SetMask;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::selectionMaskDispatched, this, _1)));
  
  mask = msg::Request | msg::FeatureToSystem;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::featureToSystemDispatched, this, _1)));
  
  mask = msg::Request | msg::SystemToFeature;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::systemToFeatureDispatched, this, _1)));
  
  mask = msg::Request | msg::FeatureToDragger;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::featureToDraggerDispatched, this, _1)));
  
  mask = msg::Request | msg::DraggerToFeature;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::draggerToFeatureDispatched, this, _1)));
  
  mask = msg::Request | msg::CheckGeometry;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::checkGeometryDispatched, this, _1)));
  
  mask = msg::Request | msg::Edit | msg::Color;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::editColorDispatched, this, _1)));
}

void Manager::cancelCommandDispatched(const msg::Message &)
{
  //we might not want the update triggered inside done slot
  //from this handler? but leave for now.
  doneSlot();
}

void Manager::doneCommandDispatched(const msg::Message&)
{
  //same as above for now, but might be different in the future.
  doneSlot();
}

void Manager::clearCommandDispatched(const msg::Message &)
{
  while(!stack.empty())
    doneSlot();
}

void Manager::addCommand(BasePtr pointerIn)
{
  //preselection will only work if the command stack is empty.
  if (!stack.empty())
  {
    stack.top()->deactivate();
    clearSelection();
  }
  stack.push(pointerIn);
  activateTop();
}

void Manager::doneSlot()
{
  //only active command should trigger it is done.
  if (!stack.empty())
  {
    stack.top()->deactivate();
    stack.pop();
  }
  clearSelection();
  observer->messageOutSignal(msg::buildStatusMessage(""));
  
  if (prf::manager().rootPtr->dragger().triggerUpdateOnFinish())
  {
    observer->messageOutSignal(msg::Mask(msg::Request | msg::Update));
  }
  
  if (!stack.empty())
    activateTop();
  else
  {
    sendCommandMessage("Active command count: 0");
    observer->messageOutSignal(msg::buildSelectionMask(selectionMask));
  }
}

void Manager::activateTop()
{
  observer->messageOutSignal(msg::buildStatusMessage(stack.top()->getStatusMessage()));
  
  std::ostringstream stream;
  stream << 
    "Active command count: " << stack.size() << std::endl <<
    "Command: " << stack.top()->getCommandName();
  sendCommandMessage(stream.str());
  
  stack.top()->activate();
}

void Manager::sendCommandMessage(const std::string& messageIn)
{
  msg::Message statusMessage(msg::Request | msg::Command | msg::Text);
  vwr::Message statusVMessage;
  statusVMessage.text = messageIn;
  statusMessage.payload = statusVMessage;
  observer->messageOutSignal(statusMessage);
}

void Manager::clearSelection()
{
  observer->messageOutSignal(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

void Manager::selectionMaskDispatched(const msg::Message &messageIn)
{
  if (!stack.empty()) //only when no commands 
    return;
  
  slc::Message sMsg = boost::get<slc::Message>(messageIn.payload);
  selectionMask = sMsg.selectionMask;
}

void Manager::featureToSystemDispatched(const msg::Message&)
{
  std::shared_ptr<cmd::FeatureToSystem> featureToSystem(new cmd::FeatureToSystem());
  addCommand(featureToSystem);
}

void Manager::systemToFeatureDispatched(const msg::Message&)
{
  std::shared_ptr<cmd::SystemToFeature> systemToFeature(new cmd::SystemToFeature());
  addCommand(systemToFeature);
}

void Manager::draggerToFeatureDispatched(const msg::Message&)
{
  std::shared_ptr<cmd::DraggerToFeature> draggerToFeature(new cmd::DraggerToFeature());
  addCommand(draggerToFeature);
}

void Manager::featureToDraggerDispatched(const msg::Message&)
{
  std::shared_ptr<cmd::FeatureToDragger> featureToDragger(new cmd::FeatureToDragger());
  addCommand(featureToDragger);
}

void Manager::checkGeometryDispatched(const msg::Message&)
{
  std::shared_ptr<CheckGeometry> checkGeometry(new CheckGeometry());
  addCommand(checkGeometry);
}

void Manager::editColorDispatched(const msg::Message&)
{
  std::shared_ptr<EditColor> editColor(new EditColor());
  addCommand(editColor);
}

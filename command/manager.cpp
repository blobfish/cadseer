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

#include <message/dispatch.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <viewer/message.h>
#include <command/manager.h>

using namespace cmd;

Manager::Manager()
{
  this->connectMessageOut(boost::bind(&msg::Dispatch::messageInSlot, &msg::dispatch(), _1));
  msg::dispatch().connectMessageOut(boost::bind(&Manager::messageInSlot, this, _1));
  
  setupDispatcher();
}

void Manager::messageInSlot(const msg::Message &messageIn)
{
  msg::MessageDispatcher::iterator it = dispatcher.find(messageIn.mask);
  if (it == dispatcher.end())
    return;
  
  it->second(messageIn);
}

void Manager::setupDispatcher()
{
  msg::Mask mask;
  
  mask = msg::Request | msg::Command | msg::Cancel;
  dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::cancelCommandDispatched, this, _1)));
  
  mask = msg::Request | msg::Command | msg::Done;
  dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::doneCommandDispatched, this, _1)));
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

void Manager::addCommand(BasePtr pointerIn)
{
  if (!stack.empty())
    stack.top()->deactivate();
  stack.push(pointerIn);
  
  msg::Message clearMessage;
  clearMessage.mask = msg::Request | msg::Selection | msg::Clear;
  messageOutSignal(clearMessage);
  
  activateTop();
}

void Manager::doneSlot()
{
  //only active command should trigger it is done.
  if (!stack.empty())
    stack.top()->deactivate();
  stack.pop();
  
  msg::Message clearMessage;
  clearMessage.mask = msg::Request | msg::Selection | msg::Clear;
  messageOutSignal(clearMessage);
  
  sendStatusMessage("");
  
  if (prf::manager().rootPtr->dragger().triggerUpdateOnFinish())
  {
    msg::Message uMessage;
    uMessage.mask = msg::Request | msg::Update;
    messageOutSignal(uMessage);
  }
  
  if (!stack.empty())
    activateTop();
  else
    sendCommandMessage("Active command count: 0");
}

void Manager::activateTop()
{
  sendStatusMessage(stack.top()->getStatusMessage());
  
  std::ostringstream stream;
  stream << 
    "Active command count: " << stack.size() << std::endl <<
    "Command: " << stack.top()->getCommandName();
  sendCommandMessage(stream.str());
  
  stack.top()->activate();
}

void Manager::sendStatusMessage(const std::string& messageIn)
{
  msg::Message statusMessage;
  statusMessage.mask = msg::Request | msg::StatusText;
  vwr::Message statusVMessage;
  statusVMessage.text = messageIn;
  statusMessage.payload = statusVMessage;
  messageOutSignal(statusMessage);
}

void Manager::sendCommandMessage(const std::string& messageIn)
{
  msg::Message statusMessage;
  statusMessage.mask = msg::Request | msg::CommandText;
  vwr::Message statusVMessage;
  statusVMessage.text = messageIn;
  statusMessage.payload = statusVMessage;
  messageOutSignal(statusMessage);
}

Manager& cmd::manager()
{
  static Manager localManager;
  return localManager;
}
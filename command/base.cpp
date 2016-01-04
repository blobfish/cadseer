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

#include <cassert>

#include <application/application.h>
#include <application/mainwindow.h>
#include <project/project.h>
#include <selection/manager.h>
#include <selection/eventhandler.h>
#include <selection/message.h>
#include <viewer/viewerwidget.h>
#include <message/dispatch.h>
#include <command/base.h>

using namespace cmd;

Base::Base()
{
  application = static_cast<app::Application*>(qApp); assert(application);
  mainWindow = application->getMainWindow(); assert(mainWindow);
  project = application->getProject(); assert(project);
  selectionManager = mainWindow->getSelectionManager(); assert(selectionManager);
  eventHandler = mainWindow->getViewer()->getSelectionEventHandler(); assert(eventHandler);
  
  connection = msg::dispatch().connectMessageOut(boost::bind(&Base::messageInSlot, this, _1));
  this->connectMessageOut(boost::bind(&msg::Dispatch::messageInSlot, &msg::dispatch(), _1));
  
  isActive = false;
}

Base::~Base()
{
}

void Base::messageInSlot(const msg::Message &messageIn)
{
  if (!isActive) //only dispatch if command is active
    return;
  msg::MessageDispatcher::iterator it = dispatcher.find(messageIn.mask);
  if (it == dispatcher.end())
    return;
  
  it->second(messageIn);
}

void Base::sendDone()
{
  msg::Message doneMessage;
  doneMessage.mask = msg::Request | msg::Command | msg::Done;
  messageOutSignal(doneMessage);
}


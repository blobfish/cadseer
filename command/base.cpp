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
#include <viewer/widget.h>
#include <project/project.h>
#include <selection/manager.h>
#include <selection/eventhandler.h>
#include <selection/message.h>
#include <message/observer.h>
#include <message/dispatch.h>
#include <command/base.h>

using namespace cmd;

Base::Base()
{
  observer = std::move(std::unique_ptr<msg::Observer>(new msg::Observer()));
  observer->name = "cmd::Base";
  application = static_cast<app::Application*>(qApp); assert(application);
  mainWindow = application->getMainWindow(); assert(mainWindow);
  project = application->getProject(); assert(project);
  selectionManager = mainWindow->getSelectionManager(); assert(selectionManager);
  eventHandler = mainWindow->getViewer()->getSelectionEventHandler(); assert(eventHandler);
  
  isActive = false;
}

Base::~Base()
{
}

void Base::sendDone()
{
  observer->out(msg::Message(msg::Request | msg::Command | msg::Done));
}


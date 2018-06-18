/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2018  Thomas S. Anderson blobfish.at.gmx.com
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

#include <message/observer.h>
#include <project/project.h>
#include <feature/thread.h>
#include <command/thread.h>

using boost::uuids::uuid;
using namespace cmd;

Thread::Thread() : Base() {}
Thread::~Thread() {}

std::string Thread::getStatusMessage()
{
  return QObject::tr("Enter parameters for thread feature").toStdString();
}

void Thread::activate()
{
  isActive = true;
  go();
  sendDone();
}

void Thread::deactivate()
{
  isActive = false;
}

void Thread::go()
{
  std::shared_ptr<ftr::Thread> t(new ftr::Thread());
  project->addFeature(t);
 
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

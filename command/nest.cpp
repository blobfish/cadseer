/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017  tanderson <email>
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
 */

#include <message/observer.h>
#include <project/project.h>
#include <selection/eventhandler.h>
#include <feature/nest.h>
#include <command/nest.h>

using namespace cmd;

using boost::uuids::uuid;

Nest::Nest()
{

}

Nest::~Nest()
{

}

std::string Nest::getStatusMessage()
{
  return QObject::tr("Select features for blank").toStdString();
}

void Nest::activate()
{
  isActive = true;
  go();
  sendDone();
}

void Nest::deactivate()
{
  isActive = false;
}

void Nest::go()
{
  //only works with preselection for now.
  const slc::Containers &containers = eventHandler->getSelections();
  if (containers.size() != 1)
  {
    observer->out(msg::buildStatusMessage("Incorrect preselection for nest feature"));
    return;
  }
  uuid bId = containers.at(0).featureId;
  
  std::shared_ptr<ftr::Nest> nest(new ftr::Nest());
  project->addFeature(nest);
  project->connect(bId, nest->getId(), ftr::InputType{ftr::Nest::blank});
  
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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
#include <selection/eventhandler.h>
#include <feature/dieset.h>
#include <command/dieset.h>

using namespace cmd;

using boost::uuids::uuid;

DieSet::DieSet() : Base()
{
}

DieSet::~DieSet()
{
}

std::string DieSet::getStatusMessage()
{
  return QObject::tr("Select features for strip").toStdString();
}

void DieSet::activate()
{
  isActive = true;
  go();
  sendDone();
}

void DieSet::deactivate()
{
  isActive = false;
}

void DieSet::go()
{
  //only works with preselection for now.
  const slc::Containers &containers = eventHandler->getSelections();
  if (containers.size() != 1)
  {
    observer->out(msg::buildStatusMessage("Incorrect preselection for DieSet feature"));
    return;
  }
  uuid bId = containers.at(0).featureId;
  
  std::shared_ptr<ftr::DieSet> ds(new ftr::DieSet());
  project->addFeature(ds);
  project->connect(bId, ds->getId(), ftr::InputType{ftr::DieSet::strip});
  
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

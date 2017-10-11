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

#include <osg/Geometry> //yuck

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
  uuid stripId = gu::createNilId();
  
  //grab first selected strip feature.
  const slc::Containers &containers = eventHandler->getSelections();
  for (const auto c : containers)
  {
    if (c.featureType == ftr::Type::Strip)
    {
      stripId = c.featureId;
      break;
    }
  }
  
  if (stripId.is_nil())
  {
    auto ids = project->getAllFeatureIds();
    for (const auto &id : ids)
    {
      ftr::Base *bf = project->findFeature(id);
      if (bf->getType() == ftr::Type::Strip)
      {
        stripId = id;
        break;
      }
    }
  }
  
  if (stripId.is_nil())
  {
    observer->out(msg::buildStatusMessage("Couldn't infer strip id for DieSet feature"));
    return;
  }
  
  std::shared_ptr<ftr::DieSet> ds(new ftr::DieSet());
  project->addFeature(ds);
  project->connect(stripId, ds->getId(), ftr::InputType{ftr::DieSet::strip});
  
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

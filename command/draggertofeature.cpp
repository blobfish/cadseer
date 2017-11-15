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
#include <application/mainwindow.h>
#include <selection/eventhandler.h>
#include <annex/csysdragger.h>
#include <feature/base.h>
#include <command/draggertofeature.h>

using namespace cmd;

DraggerToFeature::DraggerToFeature() : Base()
{
}

DraggerToFeature::~DraggerToFeature(){}

std::string DraggerToFeature::getStatusMessage()
{
  return QObject::tr("Select feature to reset dragger to feature").toStdString();
}

void DraggerToFeature::activate()
{
  isActive = true;
  
  const slc::Containers &containers = eventHandler->getSelections();
  for (const auto &container : containers)
  {
    if (container.selectionType != slc::Type::Object)
      continue;
    
    ftr::Base *baseFeature = project->findFeature(container.featureId);
    assert(baseFeature);
    
    if (!baseFeature->hasAnnex(ann::Type::CSysDragger))
      continue;
    ann::CSysDragger &da = baseFeature->getAnnex<ann::CSysDragger>(ann::Type::CSysDragger);
    da.draggerUpdate();
  }
  
  sendDone();
}

void DraggerToFeature::deactivate()
{
  isActive = false;
}

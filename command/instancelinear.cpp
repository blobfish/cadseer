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

#include <memory>

#include <project/project.h>
#include <message/observer.h>
#include <selection/eventhandler.h>
#include <feature/instancelinear.h>
#include <command/instancelinear.h>

using namespace cmd;

InstanceLinear::InstanceLinear() : Base() {}
InstanceLinear::~InstanceLinear() {}

std::string InstanceLinear::getStatusMessage()
{
  return QObject::tr("Select feature or geometry for linear instance").toStdString();
}

void InstanceLinear::activate()
{
  isActive = true;
  go();
  sendDone();
}

void InstanceLinear::deactivate()
{
  isActive = false;
}

void InstanceLinear::go()
{
  const slc::Containers &containers = eventHandler->getSelections();
  for (const auto &c : containers)
  {
    ftr::Base *bf = project->findFeature(c.featureId);
    if (!bf->hasAnnex(ann::Type::SeerShape))
      continue;
    
    std::shared_ptr<ftr::InstanceLinear> instance(new ftr::InstanceLinear());
    
    ftr::Pick pick;
    pick.id = c.shapeId;
    if (!pick.id.is_nil())
      pick.shapeHistory = project->getShapeHistory().createDevolveHistory(pick.id);
    instance->setPick(pick);
    
    project->addFeature(instance);
    project->connect(c.featureId, instance->getId(), ftr::InputType{ftr::InputType::target});
    
    observer->outBlocked(msg::buildHideThreeD(c.featureId));
    observer->outBlocked(msg::buildHideOverlay(c.featureId));
    
    observer->outBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
    
    break;
  }
  
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
}


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
#include <viewer/widget.h>
#include <feature/parameter.h>
#include <feature/instancemirror.h>
#include <command/instancemirror.h>

using namespace cmd;

InstanceMirror::InstanceMirror() : Base() {}
InstanceMirror::~InstanceMirror() {}

std::string InstanceMirror::getStatusMessage()
{
  return QObject::tr("Select feature or geometry for mirrored instance").toStdString();
}

void InstanceMirror::activate()
{
  isActive = true;
  go();
  sendDone();
}

void InstanceMirror::deactivate()
{
  isActive = false;
}

void InstanceMirror::go()
{
  const slc::Containers &containers = eventHandler->getSelections();
  if (containers.empty() || (containers.size() > 2))
  {
    observer->outBlocked(msg::buildStatusMessage("wrong selection for instance mirror"));
    return;
  }
  
  ftr::Base *bf = project->findFeature(containers.front().featureId);
  if (!bf->hasAnnex(ann::Type::SeerShape))
  {
    observer->outBlocked(msg::buildStatusMessage("first selection should have shape for instance mirror"));
    return;
  }
  std::shared_ptr<ftr::InstanceMirror> instance(new ftr::InstanceMirror());
  
  ftr::Pick shapePick;
  shapePick.id = containers.front().shapeId;
  if (!shapePick.id.is_nil())
    shapePick.shapeHistory = project->getShapeHistory().createDevolveHistory(shapePick.id);
  instance->setShapePick(shapePick);
  
  project->addFeature(instance);
  project->connect(bf->getId(), instance->getId(), ftr::InputType{ftr::InputType::target});
  
  observer->outBlocked(msg::buildHideThreeD(bf->getId()));
  observer->outBlocked(msg::buildHideOverlay(bf->getId()));
  
  if (containers.size() == 1)
  {
    instance->setCSys(viewer->getCurrentSystem());
  }
  else //size == 2
  {
    boost::uuids::uuid fId = containers.back().featureId;
    ftr::Pick planePick;
    planePick.id = containers.back().shapeId;
    if (!planePick.id.is_nil())
      planePick.shapeHistory = project->getShapeHistory().createDevolveHistory(planePick.id);
    instance->setPlanePick(planePick);
    project->connect(fId, instance->getId(), ftr::InputType{ftr::InstanceMirror::mirrorPlane});
    
    //should we hide these?
    observer->outBlocked(msg::buildHideThreeD(fId));
    observer->outBlocked(msg::buildHideOverlay(fId));
  }
  
//   observer->outBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  observer->outBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

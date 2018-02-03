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

#include <project/project.h>
#include <message/observer.h>
#include <selection/eventhandler.h>
#include <feature/refine.h>
#include <command/refine.h>

using namespace cmd;

Refine::Refine() : Base() {}
Refine::~Refine(){}

std::string Refine::getStatusMessage()
{
  return QObject::tr("Select feature for refine").toStdString();
}

void Refine::activate()
{
  isActive = true;
  go();
  sendDone();
}

void Refine::deactivate()
{
  isActive = false;
}

void Refine::go()
{
  const slc::Containers &containers = eventHandler->getSelections();
  for (const auto &c : containers)
  {
    if (c.selectionType != slc::Type::Object)
      continue;
    
    ftr::Base *bf = project->findFeature(c.featureId);
    if (!bf->hasAnnex(ann::Type::SeerShape))
      continue;
    
    std::shared_ptr<ftr::Refine> refine(new ftr::Refine());
    project->addFeature(refine);
    project->connectInsert(c.featureId, refine->getId(), ftr::InputType{ftr::InputType::target});
    
    observer->outBlocked(msg::buildHideThreeD(c.featureId));
    observer->outBlocked(msg::buildHideOverlay(c.featureId));
    
    observer->outBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
    
    break;
  }
  
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

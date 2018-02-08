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

#include <osg/Geometry>

#include <project/project.h>
#include <message/observer.h>
#include <selection/eventhandler.h>
#include <annex/seershape.h>
#include <feature/parameter.h>
#include <feature/sew.h>
#include <command/sew.h>

using boost::uuids::uuid;

using namespace cmd;

Sew::Sew() : Base() {}
Sew::~Sew() {}

std::string Sew::getStatusMessage()
{
  return QObject::tr("Select feature or geometry for sew feature").toStdString();
}

void Sew::activate()
{
  isActive = true;
  go();
  sendDone();
}

void Sew::deactivate()
{
  isActive = false;
}

void Sew::go()
{
  slc::Containers filtered;
  const slc::Containers &containers = eventHandler->getSelections();
  for (const auto &c : containers)
  {
    if (c.selectionType == slc::Type::Object)
      filtered.push_back(c);
  }
  if (filtered.size() < 2)
  {
    observer->outBlocked(msg::buildStatusMessage("Wrong pre selection for sew"));
    return;
  }
  
  std::shared_ptr<ftr::Sew> sew(new ftr::Sew());
  project->addFeature(sew);
  for (const auto &c : filtered)
  {
    project->connect(c.featureId, sew->getId(), ftr::InputType{ftr::InputType::tool});
    observer->outBlocked(msg::buildHideThreeD(c.featureId));
    observer->outBlocked(msg::buildHideOverlay(c.featureId));
  }
  
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
}


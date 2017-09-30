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

#include <application/mainwindow.h>
#include <project/project.h>
#include <message/observer.h>
#include <selection/eventhandler.h>
#include <feature/strip.h>
#include <dialogs/strip.h>
#include <command/strip.h>

using namespace cmd;
using boost::uuids::uuid;

Strip::Strip() : Base() {}

Strip::~Strip()
{
  if (dialog)
    dialog->deleteLater();
}

std::string Strip::getStatusMessage()
{
  return QObject::tr("Select features for part and blank").toStdString();
}

void Strip::activate()
{
  isActive = true;
  
  if (!dialog)
    go();
  
  dialog->show();
  dialog->raise();
  dialog->activateWindow();
}

void Strip::deactivate()
{
  isActive = false;
  
  dialog->hide();
}

void Strip::go()
{
  //only works with preselection for now.
  const slc::Containers &containers = eventHandler->getSelections();
  if (containers.size() < 2)
  {
    observer->out(msg::buildStatusMessage("Incorrect preselection for strip feature"));
    return;
  }
  uuid pId = containers.at(0).featureId;
  uuid bId = containers.at(1).featureId;
  
  if (pId == bId)
  {
    observer->out(msg::buildStatusMessage("Part and blank can't belong to same feature"));
    return;
  }
  
  std::shared_ptr<ftr::Strip> strip(new ftr::Strip());
  project->addFeature(strip);
  project->connect(pId, strip->getId(), ftr::InputType{ftr::Strip::part});
  project->connect(bId, strip->getId(), ftr::InputType{ftr::Strip::blank});
  
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  dialog = new dlg::Strip(strip.get(), mainWindow);
}

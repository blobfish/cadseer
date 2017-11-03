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
  uuid partId = gu::createNilId();
  uuid blankId = gu::createNilId();
  uuid nestId = gu::createNilId();
  
  const slc::Containers &containers = eventHandler->getSelections();
  for (const auto &c : containers)
  {
    if (c.featureType == ftr::Type::Squash)
      blankId = c.featureId;
    else if (c.featureType == ftr::Type::Nest)
      nestId = c.featureId;
    else
      partId = c.featureId;
  }
  
  if (blankId.is_nil() || nestId.is_nil())
  {
    auto ids = project->getAllFeatureIds();
    for (const auto &id : ids)
    {
      ftr::Base *bf = project->findFeature(id);
      if ((bf->getType() == ftr::Type::Squash) && (blankId.is_nil()))
        blankId = id;
      if ((bf->getType() == ftr::Type::Nest) && (nestId.is_nil()))
        nestId = id;
    }
  }
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  std::shared_ptr<ftr::Strip> strip(new ftr::Strip());
  project->addFeature(strip);
  
  observer->outBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  
  dialog = new dlg::Strip(strip.get(), mainWindow);
  dialog->setPartId(partId);
  dialog->setBlankId(blankId);
  dialog->setNestId(nestId);
}

StripEdit::StripEdit(ftr::Base *feature) : Base()
{
  strip = dynamic_cast<ftr::Strip*>(feature);
  assert(strip);
}

StripEdit::~StripEdit()
{
  if (dialog)
    dialog->deleteLater();
}

std::string StripEdit::getStatusMessage()
{
  return "Editing Strip Feature";
}

void StripEdit::activate()
{
  if (!dialog)
  {
    observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
    dialog = new dlg::Strip(strip, mainWindow);
  }
  
  isActive = true;
  dialog->show();
  dialog->raise();
  dialog->activateWindow();
}

void StripEdit::deactivate()
{
  dialog->hide();
  isActive = false;
}

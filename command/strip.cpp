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
  //no preselection.
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  std::shared_ptr<ftr::Strip> strip(new ftr::Strip());
  project->addFeature(strip);
  
  //this should trick the dagview into updating so it isn't screwed up
  //while dialog is running. only dagview responds to this message
  //as of git hash a530460.
  observer->outBlocked(msg::Response | msg::Post | msg::UpdateModel);
  
  dialog = new dlg::Strip(strip.get(), mainWindow);
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
    dialog = new dlg::Strip(strip, mainWindow);
  
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

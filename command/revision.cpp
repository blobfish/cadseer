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

#include <application/mainwindow.h>
#include <dialogs/revision.h>
#include <command/revision.h>

using namespace cmd;

Revision::Revision() : Base()
{
  shouldUpdate = false;
}

Revision::~Revision()
{
  if (dialog)
    dialog->deleteLater();
}

std::string Revision::getStatusMessage()
{
  return QObject::tr("Manage Revisions").toStdString();
}

void Revision::activate()
{
  isActive = true;
  
  if (!dialog)
    dialog = new dlg::Revision(mainWindow);
  dialog->show();
  dialog->raise();
  dialog->activateWindow();
}

void Revision::deactivate()
{
  if (dialog)
    dialog->hide();
  isActive = false;
}


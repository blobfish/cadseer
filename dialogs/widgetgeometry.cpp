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

#include <cassert>
#include <iostream>

#include <QSettings>
#include <QWidget>
#include <QEvent>

#include <application/application.h>
#include <dialogs/widgetgeometry.h>

using namespace dlg;

WidgetGeometry::WidgetGeometry(QObject *parent, const QString &uniqueNameIn) :
  QObject(parent), uniqueName(uniqueNameIn)
{
}

bool WidgetGeometry::eventFilter(QObject *objectIn, QEvent *eventIn)
{
  QWidget *widget = qobject_cast<QWidget *>(objectIn);
  assert(widget);
  if (eventIn->type() == QEvent::Show)
    restoreSettings(widget);
  else if (eventIn->type() == QEvent::Hide)
    saveSettings(widget);
  
  return QObject::eventFilter(objectIn, eventIn);
}

void WidgetGeometry::restoreSettings(QWidget *widget)
{
  QSettings &settings = static_cast<app::Application*>(qApp)->getUserSettings();
  settings.beginGroup(uniqueName);
  widget->restoreGeometry(settings.value("geometry").toByteArray());
  settings.endGroup();
}

void WidgetGeometry::saveSettings(QWidget *widget)
{
  QSettings &settings = static_cast<app::Application*>(qApp)->getUserSettings();
  settings.beginGroup(uniqueName);
  settings.setValue("geometry", widget->saveGeometry());
  settings.endGroup();
}

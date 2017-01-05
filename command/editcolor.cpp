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

#include <QColorDialog>

#include <application/application.h>
#include <application/mainwindow.h>
#include <message/message.h>
#include <message/observer.h>
#include <project/project.h>
#include <feature/base.h>
#include <selection/eventhandler.h>
#include <command/editcolor.h>

using namespace cmd;

EditColor::EditColor() : Base()
{
}

EditColor::~EditColor()
{

}

std::string EditColor::getStatusMessage()
{
  return QObject::tr("Select object to change color").toStdString();
}

void EditColor::activate()
{
  const slc::Containers &containers = eventHandler->getSelections();
  if (!containers.empty())
  {
    osg::Vec4 inColor = project->findFeature(containers.front().featureId)->getColor();
    QColor inQColor;
    inQColor.setRedF(inColor.x());
    inQColor.setGreenF(inColor.y());
    inQColor.setBlueF(inColor.z());
    QColor freshColor = QColorDialog::getColor(inQColor, application->getMainWindow());
    if (freshColor.isValid())
    {
      osg::Vec4 osgColor(freshColor.redF(), freshColor.greenF(), freshColor.blueF(), 1.0);
      for (const auto &container : containers)
      {
        if (container.selectionType != slc::Type::Object)
          continue;
        project->setColor(container.featureId, osgColor);
      }
    }
  }
  sendDone();
}

void EditColor::deactivate()
{

}

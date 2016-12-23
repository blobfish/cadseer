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

#include <project/project.h>
#include <application/application.h>
#include <application/mainwindow.h>
#include <selection/eventhandler.h>
#include <selection/manager.h>
#include <message/message.h>
#include <message/observer.h>
#include <dialogs/checkgeometry.h>
#include <command/checkgeometry.h>

using namespace cmd;

CheckGeometry::CheckGeometry() : Base()
{
  setupDispatcher();
  
  unsigned int selectionState = slc::ObjectsEnabled | slc::ObjectsSelectable;
  selectionManager->startCommand(selectionState);
}

CheckGeometry::~CheckGeometry()
{
  if (dialog)
    dialog->deleteLater();
  selectionManager->endCommand();
}

std::string CheckGeometry::getStatusMessage()
{
  return QObject::tr("Select geometry to check").toStdString();
}

void CheckGeometry::activate()
{
  isActive = true;
  
  if (!hasRan)
    go();

  if (dialog)
    dialog->show();
}

void CheckGeometry::deactivate()
{
  isActive = false;
  
  if (dialog)
    dialog->hide();
}

void CheckGeometry::go()
{
  const slc::Containers &containers = eventHandler->getSelections();
  if
  (
    (!containers.empty()) &&
    (containers.front().selectionType == slc::Type::Object)
  )
  {
    ftr::Base *feature = project->findFeature(containers.front().featureId);
    assert(feature);
    if (feature->hasSeerShape())
    {
      assert(!dialog);
      dialog = new dlg::CheckGeometry(*feature, application->getMainWindow());
      QString freshTitle = dialog->windowTitle() + " --" + feature->getName() + "--";
      dialog->setWindowTitle(freshTitle);
      hasRan = true;
      dialog->go();
      return;
    }
  }
  
  //here we didn't have an acceptable pre seleciton.
  observer->messageOutSignal(msg::Message(msg::Request | msg::Selection | msg::Clear));
  observer->messageOutSignal(msg::buildStatusMessage("Select object to check"));
}

void CheckGeometry::setupDispatcher()
{
  msg::Mask mask;
  
  mask = msg::Response | msg::Post | msg::Selection | msg::Add;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind
    (&CheckGeometry::selectionAdditionDispatched, this, _1)));
}

void CheckGeometry::selectionAdditionDispatched(const msg::Message&)
{
  if (hasRan)
    return;
  go();
  if (dialog)
    dialog->show();
}

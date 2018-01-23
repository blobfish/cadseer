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

#include <application/mainwindow.h>
#include <project/project.h>
#include <message/observer.h>
#include <selection/eventhandler.h>
#include <viewer/widget.h>
#include <feature/intersect.h>
#include <dialogs/boolean.h>
#include <command/intersect.h>

using namespace cmd;

Intersect::Intersect() : Base()
{
}

Intersect::~Intersect()
{
  if (dialog)
    dialog->deleteLater();
}

std::string Intersect::getStatusMessage()
{
  return QObject::tr("Select feature or geometry for boolean intersect").toStdString();
}

void Intersect::activate()
{
  isActive = true;
  
  if (firstRun)
  {
    firstRun = false;
    go();
  }
  
  if (dialog)
  {
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
  }
  else sendDone();
}

void Intersect::deactivate()
{
  if (dialog)
    dialog->hide();
  isActive = false;
}

void Intersect::go()
{
  auto goDialog = [&]()
  {
    observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
    std::shared_ptr<ftr::Intersect> intersect(new ftr::Intersect());
    project->addFeature(intersect);
    observer->outBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
    dialog = new dlg::Boolean(intersect.get(), mainWindow);
    observer->outBlocked(msg::buildStatusMessage("invalid pre selection"));
  };
  
  const slc::Containers &containers = eventHandler->getSelections();
  if (containers.empty())
  {
    goDialog();
    return;
  }
  
  ftr::Base *tf = project->findFeature(containers.front().featureId);
  if (!tf->hasAnnex(ann::Type::SeerShape))
  {
    goDialog();
    return;
  }
  
  std::shared_ptr<ftr::Intersect> intersect(new ftr::Intersect());
  ftr::Pick targetPick;
  targetPick.id = containers.front().shapeId;
  ftr::Picks targetPicks;
  if (!targetPick.id.is_nil())
  {
    targetPick.shapeHistory = project->getShapeHistory().createDevolveHistory(targetPick.id);
    targetPicks.push_back(targetPick);
    intersect->setTargetPicks(targetPicks);
  }
  intersect->setColor(tf->getColor());
  
  project->addFeature(intersect);
  project->connect(tf->getId(), intersect->getId(), ftr::InputType{ftr::InputType::target});
  
  observer->outBlocked(msg::buildHideThreeD(tf->getId()));
  observer->outBlocked(msg::buildHideOverlay(tf->getId()));
  
  ftr::Picks toolPicks;
  for (std::size_t index = 1; index < containers.size(); ++index)
  {
    ftr::Pick toolPick;
    toolPick.id = containers.at(index).shapeId;
    if (!toolPick.id.is_nil())
    {
      toolPick.shapeHistory = project->getShapeHistory().createDevolveHistory(toolPick.id);
      toolPicks.push_back(toolPick);
    }
    project->connect(containers.at(index).featureId, intersect->getId(), ftr::InputType{ftr::InputType::tool});
    observer->outBlocked(msg::buildHideThreeD(containers.at(index).featureId));
    observer->outBlocked(msg::buildHideOverlay(containers.at(index).featureId));
  }
  intersect->setToolPicks(toolPicks);
  
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

IntersectEdit::IntersectEdit(ftr::Base *in) : Base()
{
  //command manager edit dispatcher dispatches on ftr::type, so we know the type of 'in'
  intersect = dynamic_cast<ftr::Intersect*>(in);
  assert(intersect);
}

IntersectEdit::~IntersectEdit()
{
  if (dialog)
    dialog->deleteLater();
}

std::string IntersectEdit::getStatusMessage()
{
  return QObject::tr("Editing intersection").toStdString();
}

void IntersectEdit::activate()
{
  isActive = true;
  if (!dialog)
  {
    observer->outBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    dialog = new dlg::Boolean(intersect, mainWindow);
    dialog->setEditDialog();
  }

  dialog->show();
  dialog->raise();
  dialog->activateWindow();
}

void IntersectEdit::deactivate()
{
  dialog->hide();
  isActive = false;
}

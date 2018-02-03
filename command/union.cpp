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

#include <project/project.h>
#include <application/mainwindow.h>
#include <message/observer.h>
#include <selection/eventhandler.h>
#include <viewer/widget.h>
#include <feature/union.h>
#include <dialogs/boolean.h>
#include <command/union.h>

using namespace cmd;

Union::Union() : Base() {}
Union::~Union()
{
  if (dialog)
    dialog->deleteLater();
}

std::string Union::getStatusMessage()
{
  return QObject::tr("Select feature or geometry for boolean union").toStdString();
}

void Union::activate()
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

void Union::deactivate()
{
  if (dialog)
    dialog->hide();
  isActive = false;
}

void Union::go()
{
  auto goDialog = [&]()
  {
    observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
    std::shared_ptr<ftr::Union> onion(new ftr::Union());
    project->addFeature(onion);
    observer->outBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
    dialog = new dlg::Boolean(onion.get(), mainWindow);
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
  
  std::shared_ptr<ftr::Union> unite(new ftr::Union());
  ftr::Pick targetPick;
  targetPick.id = containers.front().shapeId;
  ftr::Picks targetPicks;
  if (!targetPick.id.is_nil())
  {
    targetPick.shapeHistory = project->getShapeHistory().createDevolveHistory(targetPick.id);
    targetPicks.push_back(targetPick);
    unite->setTargetPicks(targetPicks);
  }
  unite->setColor(tf->getColor());
  
  project->addFeature(unite);
  project->connectInsert(tf->getId(), unite->getId(), ftr::InputType{ftr::InputType::target});
  
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
    project->connect(containers.at(index).featureId, unite->getId(), ftr::InputType{ftr::InputType::tool});
    observer->outBlocked(msg::buildHideThreeD(containers.at(index).featureId));
    observer->outBlocked(msg::buildHideOverlay(containers.at(index).featureId));
    
  }
  unite->setToolPicks(toolPicks);
  
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
  observer->out(msg::Mask(msg::Request | msg::Project | msg::Update));
}




UnionEdit::UnionEdit(ftr::Base *in) : Base()
{
  //command manager edit dispatcher dispatches on ftr::type, so we know the type of 'in'
  onion = dynamic_cast<ftr::Union*>(in);
  assert(onion);
}

UnionEdit::~UnionEdit()
{
  if (dialog)
    dialog->deleteLater();
}

std::string UnionEdit::getStatusMessage()
{
  return QObject::tr("Editing union").toStdString();
}

void UnionEdit::activate()
{
  isActive = true;
  if (!dialog)
  {
    observer->outBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    dialog = new dlg::Boolean(onion, mainWindow);
    dialog->setEditDialog();
  }

  dialog->show();
  dialog->raise();
  dialog->activateWindow();
}

void UnionEdit::deactivate()
{
  dialog->hide();
  isActive = false;
}

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

#include <QInputDialog>

#include <tools/idtools.h>
#include <application/application.h>
#include <application/mainwindow.h>
#include <message/observer.h>
#include <selection/eventhandler.h>
#include <project/project.h>
#include <feature/base.h>
#include <command/featurerename.h>

using namespace cmd;

using boost::uuids::uuid;

FeatureRename::FeatureRename() : Base()
{
  setupDispatcher();
  observer->name = "cmd::FeatureRename";
}

FeatureRename::~FeatureRename() {}

std::string FeatureRename::getStatusMessage()
{
  return QObject::tr("Select object to rename").toStdString();
}

void FeatureRename::activate()
{
  isActive = true;
  go();
}

void FeatureRename::deactivate()
{
  isActive = false;
}

void FeatureRename::go()
{
  const slc::Containers &containers = eventHandler->getSelections();
  
  //only works on the first selected object.
  uuid featureId = gu::createNilId();
  for (const auto &container : containers)
  {
    if (container.selectionType != slc::Type::Object)
      continue;
    featureId = container.featureId;
    break;
  }
  
  if (!featureId.is_nil())
  {
    ftr::Base *feature = project->findFeature(featureId);
    QString oldName = feature->getName();
    
    bool result;
    QString freshName = QInputDialog::getText
    (
      application->getMainWindow(),
     QString("Rename Feature"),
     QString("Rename Feature"),
     QLineEdit::Normal,
     oldName,
     &result
    );
    
    if (result && !freshName.isEmpty())
    {
      feature->setName(freshName);
      
      //setting the name doesn't make the feature dirty and thus doesn't
      //serialize it. Here we force a serialization so rename is in sync
      //with git, but doesn't trigger an unneeded update.
      feature->serialWrite(QDir(QString::fromStdString(project->getSaveDirectory())));
      
      std::ostringstream gitStream;
      gitStream << "Rename feature id: " << gu::idToString(feature->getId())
      << "    From: " << oldName.toStdString()
      << "    To: " << freshName.toStdString();
      observer->messageOutSignal(msg::buildGitMessage(gitStream.str()));
    }
    
    sendDone();
  }
  else
    observer->messageOutSignal(msg::buildSelectionMask(slc::ObjectsEnabled | slc::ObjectsSelectable));
}

void FeatureRename::setupDispatcher()
{
  msg::Mask mask;
  
  mask = msg::Response | msg::Post | msg::Selection | msg::Add;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind
    (&FeatureRename::selectionAdditionDispatched, this, _1)));
}

void FeatureRename::selectionAdditionDispatched(const msg::Message&)
{
  if (!isActive)
    return;
  
  go();
}

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

FeatureRename::FeatureRename() : Base(), id(gu::createNilId()), name()
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

class FtrMessageVisitor : public boost::static_visitor<ftr::Message>
{
public:
  ftr::Message operator()(const prj::Message&) const {return ftr::Message();}
  ftr::Message operator()(const slc::Message&) const {return ftr::Message();}
  ftr::Message operator()(const app::Message&) const {return ftr::Message();}
  ftr::Message operator()(const vwr::Message&) const {return ftr::Message();}
  ftr::Message operator()(const ftr::Message &mIn) const {return mIn;}
};
void FeatureRename::setFromMessage(const msg::Message &mIn)
{
  ftr::Message fm = boost::apply_visitor(FtrMessageVisitor(), mIn.payload);
  id = fm.featureId;
  name = fm.string;
}

void FeatureRename::go() //re-enters from dispatch.
{
  if (id.is_nil()) //try to get feature id from selection.
  {
    const slc::Containers &containers = eventHandler->getSelections();
    for (const auto &container : containers)
    {
      if (container.selectionType != slc::Type::Object)
        continue;
      id = container.featureId;
      break; //only works on the first selected object.
    }
  }
  
  if (!id.is_nil())
  {
    if(name.isEmpty())
    {
      ftr::Base *feature = project->findFeature(id);
      QString oldName = feature->getName();
      
      name = QInputDialog::getText
      (
        application->getMainWindow(),
        QString("Rename Feature"),
        QString("Rename Feature"),
        QLineEdit::Normal,
        oldName
      );
    }
    
    if (!name.isEmpty())
      goRename();
    
    sendDone();
  }
  else
    observer->out(msg::buildSelectionMask(slc::ObjectsEnabled | slc::ObjectsSelectable));
}

void FeatureRename::goRename()
{
  ftr::Base *feature = project->findFeature(id);
  assert(feature);
  QString oldName = feature->getName();
  feature->setName(name);
      
  //setting the name doesn't make the feature dirty and thus doesn't
  //serialize it. Here we force a serialization so rename is in sync
  //with git, but doesn't trigger an unneeded update.
  feature->serialWrite(QDir(QString::fromStdString(project->getSaveDirectory())));
  
  std::ostringstream gitStream;
  gitStream << "Rename feature id: " << gu::idToString(feature->getId())
  << "    From: " << oldName.toStdString()
  << "    To: " << name.toStdString();
  observer->out(msg::buildGitMessage(gitStream.str()));
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

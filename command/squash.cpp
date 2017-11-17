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

#include <TopoDS.hxx>

#include <application/application.h>
#include <application/mainwindow.h>
#include <selection/eventhandler.h>
#include <message/observer.h>
#include <project/project.h>
#include <annex/seershape.h>
#include <feature/squash.h>
#include <tools/occtools.h>
#include <tools/idtools.h>
#include <command/squash.h>

using namespace cmd;
using boost::uuids::uuid;

Squash::Squash() : Base() {}

Squash::~Squash(){}

std::string Squash::getStatusMessage()
{
  return QObject::tr("Select feature to squash").toStdString();
}

void Squash::activate()
{
  isActive = true;
  
  go();
  
  sendDone();
}

void Squash::deactivate()
{
  isActive = false;
}

void Squash::go()
{
  //only works with preselection for now.
  ftr::Base *f = nullptr;
  ftr::Picks fps;
  const slc::Containers &containers = eventHandler->getSelections();
  for (const auto &container : containers)
  {
    if (container.selectionType != slc::Type::Face)
      continue;
    ftr::Base *tf = project->findFeature(container.featureId);
    assert(tf);
    if (!tf->hasAnnex(ann::Type::SeerShape))
      continue;
    if (!f)
    {
      f = tf;
    }
    else
    {
      if (f->getId() != tf->getId())
        continue;
    }
    
    const ann::SeerShape &ss = f->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
    TopoDS_Face face = TopoDS::Face(ss.getOCCTShape(container.shapeId));  
    ftr::Pick pick;
    pick.id = container.shapeId;
    pick.setParameter(face, container.pointLocation);
    pick.shapeHistory = project->getShapeHistory().createDevolveHistory(pick.id);
    fps.push_back(pick);
  }
  if(!f)
  {
    std::cout << "Squash: no feature found" << std::endl;
    observer->out(msg::buildStatusMessage("Squash: no feature found"));
    return;
  }
  if(fps.empty())
  {
    std::cout << "Squash: no faces" << std::endl;
    observer->out(msg::buildStatusMessage("Squash: no faces"));
    return;
  }
  std::shared_ptr<ftr::Squash> squash(new ftr::Squash());
  project->addFeature(squash);
  project->connect(f->getId(), squash->getId(), ftr::InputType{ftr::InputType::target});
  squash->setPicks(fps);
  squash->setColor(f->getColor());
  
  //here we are going to execute the update manually and then set skipUpdate to true.
  //this way we get the feature, but we stop the slow updates for now.
  app::WaitCursor wc;
  observer->outBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  observer->out(msg::Message(msg::Request | msg::Update | msg::Model));
  squash->setGranularity(0.0); //this will 'freeze' 
  
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

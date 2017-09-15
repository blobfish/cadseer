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

#include <TopoDS_Face.hxx>
#include <TopoDS.hxx>

#include <project/project.h>
#include <message/observer.h>
#include <selection/eventhandler.h>
#include <feature/seershape.h>
#include <feature/extract.h>
#include <command/extract.h>

using namespace cmd;
using boost::uuids::uuid;

Extract::Extract() : Base() {}

Extract::~Extract() {}

std::string Extract::getStatusMessage()
{
  return QObject::tr("Select geometry to extract").toStdString();
}

void Extract::activate()
{
  isActive = true;
  
  /* first time the command is activated we will check for a valid preselection.
   * if there is one then we will just build a simple blend feature and call
   * the parameter dialog. Otherwise we will call the blend dialog.
   */
  if (firstRun)
  {
    firstRun = false;
    go();
  }
  
  sendDone();
}

void Extract::deactivate()
{
  isActive = false;
}

void Extract::go()
{
  const slc::Containers &containers = eventHandler->getSelections();
  for (const auto &container : containers)
  {
    ftr::Base *baseFeature = project->findFeature(container.featureId);
    assert(baseFeature);
    if (!baseFeature->hasSeerShape())
      continue;
    const ftr::SeerShape &targetSeerShape = baseFeature->getSeerShape();
    
    if (container.selectionType == slc::Type::Face)
    {
      TopoDS_Face face = TopoDS::Face(targetSeerShape.getOCCTShape(container.shapeId));  
      ftr::Pick pick;
      pick.id = container.shapeId;
      pick.setParameter(face, container.pointLocation);
      pick.shapeHistory = project->getShapeHistory().createDevolveHistory(pick.id);
      
      std::shared_ptr<ftr::Extract> extract(new ftr::Extract());
      ftr::Extract::AccruePick ap;
      ap.picks = ftr::Picks({pick});
      ap.accrueType = ftr::AccrueType::Tangent;
      ap.parameter = ftr::Extract::buildAngleParameter(10.0);
      extract->sync(ftr::Extract::AccruePicks({ap}));
      
      project->addFeature(extract);
      project->connect(baseFeature->getId(), extract->getId(), ftr::InputType{ftr::InputType::target});
      
      baseFeature->hide3D();
      baseFeature->hideOverlay();
      extract->setColor(baseFeature->getColor());
    }
    else
    {
      ftr::Pick pick;
      pick.id = container.shapeId;
      pick.shapeHistory = project->getShapeHistory().createDevolveHistory(pick.id);
      std::shared_ptr<ftr::Extract> extract(new ftr::Extract());
      ftr::Picks picks({pick});
      extract->sync(picks);
      
      project->addFeature(extract);
      project->connect(baseFeature->getId(), extract->getId(), ftr::InputType{ftr::InputType::target});
      
      baseFeature->hide3D();
      baseFeature->hideOverlay();
      extract->setColor(baseFeature->getColor());
    }
  }
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

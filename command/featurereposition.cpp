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

#include <globalutilities.h>
#include <project/project.h>
#include <application/mainwindow.h>
#include <selection/eventhandler.h>
#include <feature/csysbase.h>
#include <viewer/widget.h>
#include <command/featurereposition.h>

using namespace cmd;

FeatureReposition::FeatureReposition() : Base() {}

FeatureReposition::~FeatureReposition(){}

std::string FeatureReposition::getStatusMessage()
{
  return QObject::tr("Select feature to reposition").toStdString();
}

void FeatureReposition::activate()
{
  isActive = true;
  
  go();
  
  sendDone();
}

void FeatureReposition::deactivate()
{
  isActive = false;
}

void FeatureReposition::go()
{
  //only works with preselection for now.
  const osg::Matrixd& cs = mainWindow->getViewer()->getCurrentSystem();
  const slc::Containers &containers = eventHandler->getSelections();
  for (const auto &container : containers)
  {
    if (container.selectionType != slc::Type::Object)
      continue;
    ftr::Base *baseFeature = project->findFeature(container.featureId);
    assert(baseFeature);
    ftr::CSysBase *csysFeature = dynamic_cast<ftr::CSysBase*>(baseFeature);
    if (!csysFeature)
      continue;
    
    osg::Matrixd dm = csysFeature->getDragger().getMatrix(); //dragger matrix
    dm = osg::Matrixd::inverse(dm);
    osg::Matrixd ns = gu::toOsg(csysFeature->getSystem()) * dm * cs;
    
    csysFeature->setSystem(ns);
    csysFeature->updateDragger(cs);
  }
}

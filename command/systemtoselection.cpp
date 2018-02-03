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

#include <boost/optional.hpp>

#include <message/observer.h>
#include <selection/eventhandler.h>
#include <viewer/widget.h>
#include <command/systemtoselection.h>

using namespace cmd;

boost::optional<osg::Matrixd> from3Points(const slc::Containers &csIn)
{
  if (csIn.size() != 3)
    return boost::optional<osg::Matrixd>();
  for (const auto &c : csIn)
  {
    if (!slc::isPointType(c.selectionType))
      return boost::optional<osg::Matrixd>();
  }
  osg::Vec3d origin = csIn.at(0).pointLocation;
  osg::Vec3d xPoint = csIn.at(1).pointLocation;
  osg::Vec3d yPoint = csIn.at(2).pointLocation;
  
  osg::Vec3d xVector = xPoint - origin;
  osg::Vec3d yVector = yPoint - origin;
  if (xVector.isNaN() || yVector.isNaN())
    return boost::optional<osg::Matrixd>();
  xVector.normalize();
  yVector.normalize();
  if ((1 - std::fabs(xVector * yVector)) < std::numeric_limits<float>::epsilon())
    return boost::optional<osg::Matrixd>();
  osg::Vec3d zVector = xVector ^ yVector;
  if (zVector.isNaN())
    return boost::optional<osg::Matrixd>();
  zVector.normalize();
  yVector = zVector ^ xVector;
  yVector.normalize();

  osg::Matrixd fm;
  fm(0,0) = xVector.x(); fm(0,1) = xVector.y(); fm(0,2) = xVector.z();
  fm(1,0) = yVector.x(); fm(1,1) = yVector.y(); fm(1,2) = yVector.z();
  fm(2,0) = zVector.x(); fm(2,1) = zVector.y(); fm(2,2) = zVector.z();
  fm.setTrans(origin);
  
  return boost::optional<osg::Matrixd>(fm);
}

SystemToSelection::SystemToSelection() : Base() {}
SystemToSelection::~SystemToSelection() {}

std::string SystemToSelection::getStatusMessage()
{
  return QObject::tr("Select geometry for derived system").toStdString();
}

void SystemToSelection::activate()
{
  isActive = true;
  go();
  sendDone();
}

void SystemToSelection::deactivate()
{
  isActive = false;
}

void SystemToSelection::go()
{
  const slc::Containers &containers = eventHandler->getSelections();
  if (containers.empty())
  {
    observer->outBlocked(msg::buildStatusMessage("No selection for system derivation"));
    return;
  }
  
  boost::optional<osg::Matrixd> ocsys;
  ocsys = from3Points(containers);
  if (ocsys)
  {
    observer->outBlocked(msg::buildStatusMessage("Current system set to points"));
    viewer->setCurrentSystem(*ocsys);
    return;
  }
  
  observer->outBlocked(msg::buildStatusMessage("Selection not supported for system derivation"));
}

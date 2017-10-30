/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2015  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef OVERLAYCAMERA_H
#define OVERLAYCAMERA_H

#include <memory>

#include <osg/Camera>
#include <osg/Switch>

namespace osgViewer{class GraphicsWindow;}

namespace msg{class Message; class Observer;}
namespace vwr
{
class OverlayCamera : public osg::Camera
{
public:
    OverlayCamera(osgViewer::GraphicsWindow *);

private:
  std::unique_ptr<msg::Observer> observer;
  void setupDispatcher();
  void featureAddedDispatched(const msg::Message &);
  void featureRemovedDispatched(const msg::Message &);
  void closeProjectDispatched(const msg::Message&);
  void addOverlayGeometryDispatched(const msg::Message&);
  void clearOverlayGeometryDispatched(const msg::Message&); //!< only fleeting, not features.
  void overlayToggleDispatched(const msg::Message &);
  void showOverlayDispatched(const msg::Message &);
  void hideOverlayDispatched(const msg::Message &);
  void projectOpenedDispatched(const msg::Message &);
  
  void serialRead();
  void serialWrite();
  
  osg::ref_ptr<osg::Switch> fleetingGeometry;
};
}

#endif // OVERLAYCAMERA_H

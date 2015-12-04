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

#include <osg/Camera>
#include <osg/Switch>
#include <osg/observer_ptr>

#include <message/message.h>

namespace osgViewer{class GraphicsWindow;}

class OverlayCamera : public osg::Camera
{
public:
    OverlayCamera(osgViewer::GraphicsWindow *);
    
    void messageInSlot(const msg::Message &);
    typedef boost::signals2::signal<void (const msg::Message &)> MessageOutSignal;
    boost::signals2::connection connectMessageOut(const MessageOutSignal::slot_type &subscriber)
    {
      return messageOutSignal.connect(subscriber);
    }
private:
  MessageOutSignal messageOutSignal; //new message system.
  msg::MessageDispatcher dispatcher;
  void setupDispatcher();
  void featureAddedDispatched(const msg::Message &);
  void featureRemovedDispatched(const msg::Message &);
};

#endif // OVERLAYCAMERA_H

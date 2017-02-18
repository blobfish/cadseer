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

#include <cassert>

#include <viewer/gleventwidget.h>
#include <viewer/spaceballqevent.h>
#include <viewer/spaceballosgevent.h>

using namespace vwr;

GLEventWidget::GLEventWidget(const QGLFormat& format, QWidget* parent, const QGLWidget* shareWidget,
                             Qt::WindowFlags f, bool forwardKeyEvents) :
    inherited(format, parent, shareWidget, f, forwardKeyEvents)
{
}

bool GLEventWidget::event(QEvent* event)
{
    if (event->type() == spb::MotionEvent::Type)
    {
        static_cast<spb::MotionEvent*>(event)->setHandled(true);
        osg::ref_ptr<osgGA::GUIEventAdapter> osgEvent = _gw->getEventQueue()->createEvent();
        osgEvent->setEventType(osgGA::GUIEventAdapter::USER);
        osgEvent->setUserData(convertEvent(event).get());
        _gw->getEventQueue()->addEvent(osgEvent);
        return true;
    }
    else if (event->type() == spb::ButtonEvent::Type)
    {
      static_cast<spb::ButtonEvent*>(event)->setHandled(true);
      osg::ref_ptr<osgGA::GUIEventAdapter> osgEvent = _gw->getEventQueue()->createEvent();
      osgEvent->setEventType(osgGA::GUIEventAdapter::USER);
      osgEvent->setUserData(convertEvent(event).get());
      _gw->getEventQueue()->addEvent(osgEvent);
      return true;
    }
    return inherited::event(event);
}

osg::ref_ptr<spb::SpaceballOSGEvent> GLEventWidget::convertEvent(QEvent* qEvent)
{
    osg::ref_ptr<spb::SpaceballOSGEvent> osgEvent = new spb::SpaceballOSGEvent();
    if (qEvent->type() == spb::MotionEvent::Type)
    {
        spb::MotionEvent *spaceQEvent = dynamic_cast<spb::MotionEvent *>(qEvent);
        osgEvent->theType = spb::SpaceballOSGEvent::Motion;
        osgEvent->translationX = spaceQEvent->translationX();
        osgEvent->translationY = spaceQEvent->translationY();
        osgEvent->translationZ = spaceQEvent->translationZ();
        osgEvent->rotationX = spaceQEvent->rotationX();
        osgEvent->rotationY = spaceQEvent->rotationY();
        osgEvent->rotationZ = spaceQEvent->rotationZ();
    }
    else if (qEvent->type() == spb::ButtonEvent::Type)
    {
      spb::ButtonEvent *buttonQEvent = dynamic_cast<spb::ButtonEvent*>(qEvent);
      assert(buttonQEvent);
      osgEvent->theType = spb::SpaceballOSGEvent::Button;
      osgEvent->buttonNumber = buttonQEvent->buttonNumber();
      if (buttonQEvent->buttonStatus() == spb::BUTTON_PRESSED)
        osgEvent->theButtonState = spb::SpaceballOSGEvent::Pressed;
      else
        osgEvent->theButtonState = spb::SpaceballOSGEvent::Released;
    }
    
    return osgEvent;
}

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
    if (event->type() == vwr::MotionEvent::Type)
    {
        static_cast<vwr::MotionEvent*>(event)->setHandled(true);
        osg::ref_ptr<osgGA::GUIEventAdapter> osgEvent = _gw->getEventQueue()->createEvent();
        osgEvent->setEventType(osgGA::GUIEventAdapter::USER);
        osgEvent->setUserData(convertEvent(event).get());
        _gw->getEventQueue()->addEvent(osgEvent);
        return true;
    }
    else if (event->type() == vwr::ButtonEvent::Type)
    {
      static_cast<vwr::ButtonEvent*>(event)->setHandled(true);
      osg::ref_ptr<osgGA::GUIEventAdapter> osgEvent = _gw->getEventQueue()->createEvent();
      osgEvent->setEventType(osgGA::GUIEventAdapter::USER);
      osgEvent->setUserData(convertEvent(event).get());
      _gw->getEventQueue()->addEvent(osgEvent);
      return true;
    }
    return inherited::event(event);
}

osg::ref_ptr<vwr::SpaceballOSGEvent> GLEventWidget::convertEvent(QEvent* qEvent)
{
    osg::ref_ptr<vwr::SpaceballOSGEvent> osgEvent = new vwr::SpaceballOSGEvent();
    if (qEvent->type() == vwr::MotionEvent::Type)
    {
        vwr::MotionEvent *spaceQEvent = dynamic_cast<vwr::MotionEvent *>(qEvent);
        osgEvent->theType = vwr::SpaceballOSGEvent::Motion;
        osgEvent->translationX = spaceQEvent->translationX();
        osgEvent->translationY = spaceQEvent->translationY();
        osgEvent->translationZ = spaceQEvent->translationZ();
        osgEvent->rotationX = spaceQEvent->rotationX();
        osgEvent->rotationY = spaceQEvent->rotationY();
        osgEvent->rotationZ = spaceQEvent->rotationZ();
    }
    else if (qEvent->type() == vwr::ButtonEvent::Type)
    {
      vwr::ButtonEvent *buttonQEvent = dynamic_cast<vwr::ButtonEvent*>(qEvent);
      assert(buttonQEvent);
      osgEvent->theType = vwr::SpaceballOSGEvent::Button;
      osgEvent->buttonNumber = buttonQEvent->buttonNumber();
      if (buttonQEvent->buttonStatus() == vwr::BUTTON_PRESSED)
        osgEvent->theButtonState = vwr::SpaceballOSGEvent::Pressed;
      else
        osgEvent->theButtonState = vwr::SpaceballOSGEvent::Released;
    }
    
    return osgEvent;
}

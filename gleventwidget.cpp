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

#include <iostream>

#include "gleventwidget.h"
#include "spaceballqevent.h"
#include "spaceballosgevent.h"

GLEventWidget::GLEventWidget(const QGLFormat& format, QWidget* parent, const QGLWidget* shareWidget,
                             Qt::WindowFlags f, bool forwardKeyEvents) :
    inherited(format, parent, shareWidget, f, forwardKeyEvents)
{
}

bool GLEventWidget::event(QEvent* event)
{
    if (event->type() == spb::MotionEvent::Type)
    {
//        std::cout << "inside event widget" << std::endl;
        osg::ref_ptr<osgGA::GUIEventAdapter> osgEvent = _gw->getEventQueue()->createEvent();
        osgEvent->setEventType(osgGA::GUIEventAdapter::USER);
        osgEvent->setUserData(convertEvent(event).get());
        _gw->getEventQueue()->addEvent(osgEvent);
        return true;
    }
    return inherited::event(event);
}

osg::ref_ptr<SpaceballOSGEvent> GLEventWidget::convertEvent(QEvent* qEvent)
{
    osg::ref_ptr<SpaceballOSGEvent> osgEvent = new SpaceballOSGEvent();
    if (qEvent->type() == spb::MotionEvent::Type)
    {
        spb::MotionEvent *spaceQEvent = dynamic_cast<spb::MotionEvent *>(qEvent);
        osgEvent->theType = SpaceballOSGEvent::Motion;
        osgEvent->translationX = spaceQEvent->translationX();
        osgEvent->translationY = spaceQEvent->translationY();
        osgEvent->translationZ = spaceQEvent->translationZ();
        osgEvent->rotationX = spaceQEvent->rotationX();
        osgEvent->rotationY = spaceQEvent->rotationY();
        osgEvent->rotationZ = spaceQEvent->rotationZ();
    }
    return osgEvent;
}

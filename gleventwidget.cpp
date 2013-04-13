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
    if (event->type() == Spaceball::MotionEvent::Type)
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
    if (qEvent->type() == Spaceball::MotionEvent::Type)
    {
        Spaceball::MotionEvent *spaceQEvent = dynamic_cast<Spaceball::MotionEvent *>(qEvent);
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

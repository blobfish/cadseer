#include <iostream>
#include <QX11Info>

#include "application.h"
#include "mainwindow.h"
#include "spaceballqevent.h"

#include <spnav.h>

Application::Application(int &argc, char **argv) :
    QApplication(argc, argv)
{
    mainWindow = 0;
    spaceballPresent = false;
}

Application::~Application()
{

}

void Application::quittingSlot()
{
    if (!spnav_close())// the not seems goofy.
    {
//        std::cout << "spaceball disconnected" << std::endl;
    }
    else
    {
//        std::cout << "couldn't disconnect spaceball" << std::endl;
    }
}

bool Application::x11EventFilter(XEvent *event)
{
    spnav_event navEvent;
    if (!spnav_x11_event(event, &navEvent))
        return false;
//    std::cout << "got spacenav event" << std::endl;

    if (navEvent.type == SPNAV_EVENT_MOTION)
    {
        Spaceball::MotionEvent *qEvent = new Spaceball::MotionEvent();
        qEvent->setTranslations(navEvent.motion.x, navEvent.motion.y, navEvent.motion.z);
        qEvent->setRotations(navEvent.motion.rx, navEvent.motion.ry, navEvent.motion.rz);

        QWidget *currentWidget = qApp->focusWidget();
        if (currentWidget)
            this->postEvent(currentWidget, qEvent);
        return true;
    }

    if (navEvent.type == SPNAV_EVENT_BUTTON)
    {
        return true;
    }
    return false;
}

void Application::initializeSpaceball(MainWindow *mainWindowIn)
{
    mainWindow = mainWindowIn;
    if (!mainWindow)
        return;

    Spaceball::registerEvents();

    if (spnav_x11_open(QX11Info::display(), mainWindow->winId()) == -1)
    {
//        std::cout << "No spaceball found" << std::endl;
    }
    else
    {
//        std::cout << "Spaceball found" << std::endl;
        spaceballPresent = true;
    }
}

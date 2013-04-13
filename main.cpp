#include "application.h"
#include "viewerwidget.h"
#include "mainwindow.h"


int main( int argc, char** argv )
{
//    osg::ArgumentParser arguments(&argc, argv);

//    osgViewer::ViewerBase::ThreadingModel threadingModel = osgViewer::ViewerBase::CullDrawThreadPerContext;
//    while (arguments.read("--SingleThreaded")) threadingModel = osgViewer::ViewerBase::SingleThreaded;
//    while (arguments.read("--CullDrawThreadPerContext")) threadingModel = osgViewer::ViewerBase::CullDrawThreadPerContext;
//    while (arguments.read("--DrawThreadPerContext")) threadingModel = osgViewer::ViewerBase::DrawThreadPerContext;
//    while (arguments.read("--CullThreadPerCameraDrawThreadPerContext")) threadingModel = osgViewer::ViewerBase::CullThreadPerCameraDrawThreadPerContext;

    Application app(argc, argv);
    QObject::connect(&app, SIGNAL(aboutToQuit()), &app, SLOT(quittingSlot()));

    MainWindow mainWindow;
    app.initializeSpaceball(&mainWindow);

    mainWindow.showMaximized();
    return app.exec();
}

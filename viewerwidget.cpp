#include <iostream>
#include <iomanip>
#include <QtCore/QTimer>
#include <QHBoxLayout>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>
#include <osgGA/StandardManipulator>
#include <osgGA/OrbitManipulator>
#include <osgDB/ReadFile>
#include <osgQt/GraphicsWindowQt>
#include <osg/PolygonMode>

#include "viewerwidget.h"
#include "gleventwidget.h"
#include "spaceballmanipulator.h"

ViewerWidget::ViewerWidget(osgViewer::ViewerBase::ThreadingModel threadingModel) : QWidget()
{
    setThreadingModel(threadingModel);
    connect(&_timer, SIGNAL(timeout()), this, SLOT(update()));

    root = new osg::Group;
    osg::ref_ptr<osg::PolygonMode> pm = new osg::PolygonMode;
    pm->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::FILL);
    root->getOrCreateStateSet()->setAttribute(pm.get());

    osg::Camera* camera = createCamera();

    osgViewer::View* view = new osgViewer::View;
    view->setCamera(camera);
    addView(view);


    view->setSceneData(root);
    view->addEventHandler(new osgViewer::StatsHandler);
//    view->setCameraManipulator(new osgGA::TrackballManipulator);
    view->setCameraManipulator(new osgGA::SpaceballManipulator(camera));

//    SpaceballOSGEventHandler *eventHandler = new SpaceballOSGEventHandler(view);
//    view->addEventHandler(eventHandler);

    osgQt::GraphicsWindowQt* gw = dynamic_cast<osgQt::GraphicsWindowQt*>(camera->getGraphicsContext());
    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(gw->getGLWidget());
    setLayout(layout);

    _timer.start( 10 );
}

void dumpLookAt(const osg::Vec3d &eye, const osg::Vec3d &center, const osg::Vec3d &up)
{
    std::cout << std::endl << "stats: " << std::endl <<
                 "   eye: " << eye.x() << "   " << eye.y() << "   " << eye.z() << std::endl <<
                 "   center: " << center.x() << "   " << center.y() << "   " << center.z() << std::endl <<
                 "   up: " << up.x() << "   " << up.y() << "   " << up.z() << std::endl;

}

void ViewerWidget::addNode(osg::Node *node)
{
    root->addChild(node);
    osgViewer::View* view = this->getView(0);
    osgGA::CameraManipulator *camManip = view->getCameraManipulator();
    camManip->home(1.0);


//    osg::Vec3d eye, center, up;
//    camManip->getHomePosition(eye, center, up);
//    dumpLookAt(eye, center, up);

//    osg::Matrixd theMatrix;
//    theMatrix.makeLookAt(eye, center, up);

//    osg::Vec3d eyeOut, centerOut, upOut;
//    theMatrix.getLookAt(eyeOut, centerOut, upOut, (center-eye).length());
//    dumpLookAt(eyeOut, centerOut, upOut);

}

osg::Camera* ViewerWidget::createCamera()
{
    /*
     comparison osg::GraphicsContext::Traits to QGLFormat
     traits
            alpha = 0
            stencil = 0
            sampleBuffers = 0
            samples = 0

    qglformat
            alpha = -1
            stencil = -1
            sampleBuffers = 0
            samples = -1
          */
//    osgQt::GLWidget *glWidget = new osgQt::GLWidget(QGLFormat::defaultFormat(), this);
    GLEventWidget *glWidget = new GLEventWidget(QGLFormat::defaultFormat(), this);
    osgQt::GraphicsWindowQt *windowQt = new osgQt::GraphicsWindowQt(glWidget);

    osg::ref_ptr<osg::Camera> camera = new osg::Camera;
    camera->setGraphicsContext(windowQt);

    camera->setClearColor(osg::Vec4(0.2, 0.2, 0.6, 1.0));
    camera->setViewport(new osg::Viewport(0, 0, glWidget->width(), glWidget->height()));
//    camera->setProjectionMatrixAsPerspective(30.0f, static_cast<double>(glWidget->width()) /
//                                             static_cast<double>(glWidget->height()), 1.0f, 10000.0f);
    camera->setProjectionMatrixAsOrtho
            (static_cast<double>(glWidget->width()/-2.0d), static_cast<double>(glWidget->width()/2.0d),
             static_cast<double>(glWidget->height()/-2.0d), static_cast<double>(glWidget->height()/2.0d),
             1.0f, 10000.0f);
    return camera.release();
}

void ViewerWidget::paintEvent( QPaintEvent* event )
{
    frame();
}

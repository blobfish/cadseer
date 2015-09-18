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
#include <iomanip>

#include <QtCore/QTimer>
#include <QHBoxLayout>
#include <QApplication>
#include <QDir>
#include <QString>
#include <QFileDialog>

#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>
#include <osgGA/StandardManipulator>
#include <osgGA/OrbitManipulator>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgQt/GraphicsWindowQt>
#include <osg/PolygonMode>
#include <osg/ShadeModel>
#include <osg/Depth>
#include <osgUtil/Optimizer>
#include <osg/BlendFunc>
#include <osg/ValueObject>

#include "viewerwidget.h"
#include "gleventwidget.h"
#include "nodemaskdefs.h"
#include "selectiondefs.h"
#include "./testing/plotter.h"
#include "./gesture/gesturehandler.h"
#include "./command/commandmanager.h"
#include "globalutilities.h"
#include "coordinatesystem.h"

ViewerWidget::ViewerWidget(osgViewer::ViewerBase::ThreadingModel threadingModel) : QWidget()
{
    setThreadingModel(threadingModel);
    connect(&_timer, SIGNAL(timeout()), this, SLOT(update()));

    root = new osg::Group;
    root->setName("viewer root");
    
    osg::ref_ptr<osg::PolygonMode> pm = new osg::PolygonMode;
    pm->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::FILL);
//     pm->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
    root->getOrCreateStateSet()->setAttribute(pm.get());
    
    osg::ShadeModel *shadeModel = new osg::ShadeModel(osg::ShadeModel::SMOOTH);
    root->getOrCreateStateSet()->setAttribute(shadeModel);
    
    root->getOrCreateStateSet()->setMode(GL_MULTISAMPLE_ARB, osg::StateAttribute::ON);
    root->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::ON);
    
    Plotter::getReference().setBase(root);
    
    root->addChild(CoordinateSystem::buildCoordinateSystemNode());

    osgViewer::View* view = new osgViewer::View;
    createMainCamera(view->getCamera());

    //background was covering scene. don't know if it was the upgrade to osg 3.2
    //or the switch to the open source ati driver. I am guessing the driver.
    osg::Camera *backGroundCamera = createBackgroundCamera();
    backGroundCamera->setViewport(new osg::Viewport(0, 0, glWidgetWidth, glWidgetHeight));
    backGroundCamera->setProjectionResizePolicy(osg::Camera::ProjectionResizePolicy::FIXED);
    view->addSlave(backGroundCamera, false);

    osg::Camera *gestureCamera = createGestureCamera();
    gestureCamera->setViewport(new osg::Viewport(0, 0, glWidgetWidth, glWidgetHeight));
    gestureCamera->setProjectionResizePolicy(osg::Camera::ProjectionResizePolicy::FIXED);
    view->addSlave(gestureCamera, false);


    view->setSceneData(root);
    view->addEventHandler(new osgViewer::StatsHandler);
    selectionHandler = new SelectionEventHandler();
    view->addEventHandler(selectionHandler);
    //why does the following line create an additional thread. why?
    //not sure why it does, but it happens down inside librsvg and doesn't
    //come into play for application. I have proven out that our qactions
    //between gesture camera a mainwindlow slots are synchronized.
    view->addEventHandler(new GestureHandler(gestureCamera));
//    view->setCameraManipulator(new osgGA::TrackballManipulator);
    spaceballManipulator = new osgGA::SpaceballManipulator(view->getCamera());
    view->setCameraManipulator(spaceballManipulator);

    addView(view);

    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(windowQt->getGLWidget());
    setLayout(layout);

    setupCommands();

    _timer.start( 10 );
}

void dumpLookAt(const osg::Vec3d &eye, const osg::Vec3d &center, const osg::Vec3d &up)
{
    std::cout << std::endl << "stats: " << std::endl <<
                 "   eye: " << eye.x() << "   " << eye.y() << "   " << eye.z() << std::endl <<
                 "   center: " << center.x() << "   " << center.y() << "   " << center.z() << std::endl <<
                 "   up: " << up.x() << "   " << up.y() << "   " << up.z() << std::endl;

}

void ViewerWidget::update()
{
    osgViewer::View* view = this->getView(0);
    osgGA::CameraManipulator *camManip = view->getCameraManipulator();
    camManip->home(1.0);

    osgUtil::Optimizer opt;
    opt.optimize(root);


//    osg::Vec3d eye, center, up;
//    camManip->getHomePosition(eye, center, up);
//    dumpLookAt(eye, center, up);

//    osg::Matrixd theMatrix;
//    theMatrix.makeLookAt(eye, center, up);

//    osg::Vec3d eyeOut, centerOut, upOut;
//    theMatrix.getLookAt(eyeOut, centerOut, upOut, (center-eye).length());
//    dumpLookAt(eyeOut, centerOut, upOut);

}

void ViewerWidget::createMainCamera(osg::Camera *camera)
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
    QGLFormat format = QGLFormat::defaultFormat();
    format.setSamples(4); //big slowdown.

    GLEventWidget *glWidget = new GLEventWidget(format, this);
    windowQt = new osgQt::GraphicsWindowQt(glWidget);

    camera->setGraphicsContext(windowQt);

    QPixmap cursorImage(":/resources/images/cursor.png");
    QCursor cursor(cursorImage.scaled(32, 32));//hot point defaults to center.
    glWidget->setCursor(cursor);

    camera->setClearColor(osg::Vec4(0.2, 0.2, 0.6, 1.0));
    camera->setViewport(new osg::Viewport(0, 0, glWidget->width(), glWidget->height()));
//    camera->setProjectionMatrixAsPerspective(30.0f, static_cast<double>(glWidget->width()) /
//                                             static_cast<double>(glWidget->height()), 1.0f, 10000.0f);
    camera->setProjectionMatrixAsOrtho
            (0.0d, static_cast<double>(glWidget->width()),
             0.0d, static_cast<double>(glWidget->height()),
             1.0f, 10000.0f);

    //this allows us to see points.
    camera->setCullingMode(camera->getCullingMode() &
    ~osg::CullSettings::SMALL_FEATURE_CULLING);

    //use this for fade specs
    glWidgetWidth = glWidget->width();
    glWidgetHeight = glWidget->height();
}

void ViewerWidget::paintEvent( QPaintEvent* event )
{
    frame();
}

osg::Camera* ViewerWidget::createBackgroundCamera()
{
    //get background image and convert.
    QImage qImageBase(":/resources/images/background.png");
    //I am hoping that osg will free this memory.
    QImage *qImage = new QImage(QGLWidget::convertToGLFormat(qImageBase));
    unsigned char *imageData = qImage->bits();
    osg::ref_ptr<osg::Image> osgImage = new osg::Image();
    osgImage->setImage(qImage->width(), qImage->height(), 1, GL_RGBA, GL_RGBA,
                       GL_UNSIGNED_BYTE, imageData, osg::Image::USE_NEW_DELETE, 1);

    osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;

    texture->setImage(osgImage.get());
    osg::ref_ptr<osg::Drawable> quad = osg::createTexturedQuadGeometry
            (osg::Vec3(), osg::Vec3(1.0f, 0.0f, 0.0f), osg::Vec3(0.0f, 1.0f, 0.0f));
    quad->getOrCreateStateSet()->setTextureAttributeAndModes(0, texture.get());
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(quad.get());

    osg::Camera *bgCamera = new osg::Camera();
    bgCamera->setGraphicsContext(windowQt);
    bgCamera->setCullingActive(false);
    bgCamera->setClearMask(0);
    bgCamera->setAllowEventFocus(false);
    bgCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    bgCamera->setRenderOrder(osg::Camera::POST_RENDER);
    bgCamera->setProjectionMatrix(osg::Matrix::ortho2D(0.0, 1.0, 0.0, 1.0));
    bgCamera->addChild(geode.get());
    bgCamera->setNodeMask(NodeMaskDef::backGroundCamera);

    osg::StateSet* ss = bgCamera->getOrCreateStateSet();
    ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    ss->setAttributeAndModes(new osg::Depth(osg::Depth::LEQUAL, 1.0, 1.0));

    return bgCamera;
}

osg::Camera* ViewerWidget::createGestureCamera()
{
//     double localWidgetWidth = static_cast<double>(glWidgetWidth);
//     double localWidgetHeight = static_cast<double>(glWidgetHeight);

//     double projectionHeight = 1000.0; //height is constant
    double quadSize = 10000;


    osg::ref_ptr<osg::Geometry> quad = osg::createTexturedQuadGeometry
//            (osg::Vec3(), osg::Vec3(localWidgetWidth, 0.0, 0.0), osg::Vec3(0.0, localWidgetHeight, 0.0));
    (osg::Vec3(), osg::Vec3(quadSize, 0.0, 0.0), osg::Vec3(0.0, quadSize, 0.0));
//    quad->getOrCreateStateSet()->setTextureAttributeAndModes(0, texture.get());
    osg::Vec4Array *colorArray = new osg::Vec4Array();
    colorArray->push_back(osg::Vec4(0.0, 0.0, 0.0, 0.8));
    quad->setColorArray(colorArray);
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(quad.get());
    geode->setNodeMask(NodeMaskDef::gestureCamera);

    osg::Camera *fadeCamera = new osg::Camera();
    fadeCamera->setCullingActive(false);
    fadeCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
    fadeCamera->setAllowEventFocus(false);
    fadeCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    fadeCamera->setRenderOrder(osg::Camera::POST_RENDER);
//    fadeCamera->setProjectionMatrix(osg::Matrix::ortho2D(0.0, localWidgetWidth, 0.0, localWidgetHeight));
    fadeCamera->setProjectionMatrix(osg::Matrix::ortho2D(0.0, 1000.0, 0.0, 1000.0));
//    fadeCamera->setProjectionMatrix(osg::Matrix::ortho2D(0.0, 100.0, 0.0, 100.0));
    fadeCamera->setViewMatrix(osg::Matrix::identity());
    fadeCamera->setGraphicsContext(windowQt);
    fadeCamera->setNodeMask(NodeMaskDef::gestureCamera);

    osg::StateSet* ss = fadeCamera->getOrCreateStateSet();
    ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    ss->setAttributeAndModes(new osg::Depth(osg::Depth::LEQUAL, 0.0, 0.0));
    osg::ref_ptr<osg::BlendFunc> blend = new osg::BlendFunc();
    blend->setFunction(GL_SRC_COLOR, GL_ONE_MINUS_DST_ALPHA);
    ss->setAttributeAndModes(blend);
    ss->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

    osg::Switch *aSwitch = new osg::Switch();
    aSwitch->addChild(geode.get());
    aSwitch->setAllChildrenOff();
    fadeCamera->addChild(aSwitch);

    return fadeCamera;
}

void ViewerWidget::setSelectionMask(const int &maskIn)
{
    //turn points on or off.

    VisibleVisitor visitor((SelectionMask::verticesSelectable & maskIn) == SelectionMask::verticesSelectable);
    root->accept(visitor);

    selectionHandler->setSelectionMask(maskIn);
}

void ViewerWidget::setupCommands()
{
    QAction *topAction = new QAction(qApp);
    connect(topAction, SIGNAL(triggered()), this, SLOT(viewTopSlot()));
    Command topCommand(CommandConstants::StandardViewTop, "View Top", topAction);
    CommandManager::getManager().addCommand(topCommand);

    QAction *frontAction = new QAction(qApp);
    connect(frontAction, SIGNAL(triggered()), this, SLOT(viewFrontSlot()));
    Command frontCommand(CommandConstants::StandardViewFront, "View Front", frontAction);
    CommandManager::getManager().addCommand(frontCommand);

    QAction *rightAction = new QAction(qApp);
    connect(rightAction, SIGNAL(triggered()), this, SLOT(viewRightSlot()));
    Command rightCommand(CommandConstants::StandardViewRight, "View Right", rightAction);
    CommandManager::getManager().addCommand(rightCommand);

    QAction *fitAction = new QAction(qApp);
    connect(fitAction, SIGNAL(triggered()), this, SLOT(viewFitSlot()));
    Command fitCommand(CommandConstants::ViewFit, "View Fit", fitAction);
    CommandManager::getManager().addCommand(fitCommand);
}

void ViewerWidget::viewTopSlot()
{
    spaceballManipulator->setView(osg::Vec3d(0.0, 0.0, -1.0), osg::Vec3d(0.0, 1.0, 0.0));
    spaceballManipulator->viewFit();
}

void ViewerWidget::viewFrontSlot()
{
    spaceballManipulator->setView(osg::Vec3d(0.0, 1.0, 0.0), osg::Vec3d(0.0, 0.0, 1.0));
    spaceballManipulator->viewFit();
}

void ViewerWidget::viewRightSlot()
{
    spaceballManipulator->setView(osg::Vec3d(-1.0, 0.0, 0.0), osg::Vec3d(0.0, 0.0, 1.0));
    spaceballManipulator->viewFit();
}

void ViewerWidget::viewFitSlot()
{
    spaceballManipulator->viewFit();
}

void ViewerWidget::writeOSGSlot()
{
  QString start = QDir::homePath() + "/OpenSceneGraph.osg";
  QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), start, tr("Scene (*.osg)"));
  if (fileName.isEmpty())
    return;
  osgDB::writeNodeFile(*root, fileName.toStdString());
}

VisibleVisitor::VisibleVisitor(bool visIn) : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), visibility(visIn)
{
}

void VisibleVisitor::apply(osg::Switch &aSwitch)
{
    traverse(aSwitch);

    if (aSwitch.getNodeMask() & NodeMaskDef::vertex)
    {
        if (visibility)
            aSwitch.setAllChildrenOn();
        else
            aSwitch.setAllChildrenOff();
    }
}

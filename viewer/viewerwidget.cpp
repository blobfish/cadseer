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

#include <QHBoxLayout>
#include <QApplication>
#include <QDir>
#include <QString>
#include <QFileDialog>
#include <QTimer>
#include <QTextStream>

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

#include <viewer/spaceballmanipulator.h>
#include <viewer/viewerwidget.h>
#include <viewer/gleventwidget.h>
#include <modelviz/nodemaskdefs.h>
#include <modelviz/hiddenlineeffect.h>
#include <selection/definitions.h>
#include <testing/plotter.h>
#include <gesture/gesturehandler.h>
#include <globalutilities.h>
#include <tools/infotools.h>
#include <library/csysdragger.h>
#include <selection/eventhandler.h>
#include <selection/overlayhandler.h>
#include <message/dispatch.h>
#include <message/observer.h>
#include <viewer/textcamera.h>
#include <viewer/overlaycamera.h>
#include <feature/base.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>

using namespace vwr;

class HiddenLineVisitor : public osg::NodeVisitor
{
public:
  HiddenLineVisitor(bool visIn) :
    osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), visibility(visIn){}
  virtual void apply(osg::Group &aGroup) override
  {
    mdv::HiddenLineEffect *effect = dynamic_cast<mdv::HiddenLineEffect*>(&aGroup);
    if (effect)
      effect->setHiddenLine(visibility);
    traverse(aGroup);
  }
protected:
  bool visibility;
};

ViewerWidget::ViewerWidget(osgViewer::ViewerBase::ThreadingModel threadingModel) : QWidget(), osgViewer::CompositeViewer()
{
    observer = std::move(std::unique_ptr<msg::Observer>(new msg::Observer()));
    observer->name = "vwr::ViewerWidget";
    setupDispatcher();
    
    setThreadingModel(threadingModel);
    setKeyEventSetsDone(0); //stops the viewer from freezing when the escape key is pressed.
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update())); //inherited update slot.

    root = new osg::Group;
    root->setName("root");
    
    osg::ref_ptr<osg::PolygonMode> pm = new osg::PolygonMode;
    root->getOrCreateStateSet()->setAttribute(pm.get());
    viewFillDispatched(msg::Message());
    
    osg::ShadeModel *shadeModel = new osg::ShadeModel(osg::ShadeModel::SMOOTH);
    root->getOrCreateStateSet()->setAttribute(shadeModel);
    
    root->getOrCreateStateSet()->setMode(GL_MULTISAMPLE_ARB, osg::StateAttribute::ON);
    root->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::ON);
    
//     Plotter::getReference().setBase(root);
    
    osgViewer::View* view = new osgViewer::View;
    mainCamera = view->getCamera();
    createMainCamera(mainCamera);

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
    
    TextCamera *infoCamera = new TextCamera(windowQt);
    infoCamera->setViewport(0, 0, glWidgetWidth, glWidgetHeight);
    infoCamera->setProjectionResizePolicy(osg::Camera::ProjectionResizePolicy::FIXED);
    view->addSlave(infoCamera, false);
    
    OverlayCamera *oCamera = new OverlayCamera(windowQt);
    view->addSlave(oCamera, false);
    
    systemSwitch = new osg::Switch();
    oCamera->addChild(systemSwitch);
    
    currentSystem = new lbr::CSysDragger();
    currentSystem->setScreenScale(100.0f);
    currentSystem->setRotationIncrement(15.0);
    currentSystem->setTranslationIncrement(0.25);
    currentSystem->setHandleEvents(false);
    currentSystem->setupDefaultGeometry();
    currentSystem->setUnlink();
//     currentSystem->hide(lbr::CSysDragger::SwitchIndexes::XRotate);
//     currentSystem->hide(lbr::CSysDragger::SwitchIndexes::YRotate);
//     currentSystem->hide(lbr::CSysDragger::SwitchIndexes::ZRotate);
    currentSystem->hide(lbr::CSysDragger::SwitchIndexes::LinkIcon);
    currentSystemCallBack = new lbr::CSysCallBack(currentSystem.get());
    currentSystem->addDraggerCallback(currentSystemCallBack.get());
    systemSwitch->addChild(currentSystem);
    systemSwitch->setValue(0, prf::manager().rootPtr->visual().display().showCurrentSystem());

    view->setSceneData(root);
    view->addEventHandler(new StatsHandler());
    overlayHandler = new slc::OverlayHandler(oCamera);
    view->addEventHandler(overlayHandler.get());
    selectionHandler = new slc::EventHandler(root);
    view->addEventHandler(selectionHandler.get());
    //why does the following line create an additional thread. why?
    //not sure why it does, but it happens down inside librsvg and doesn't
    //come into play for application. I have proven out that our qactions
    //between gesture camera a mainwindlow slots are synchronized.
    view->addEventHandler(new GestureHandler(gestureCamera));
    view->addEventHandler(new ResizeEventHandler(infoCamera));
    spaceballManipulator = new vwr::SpaceballManipulator(view->getCamera());
    view->setCameraManipulator(spaceballManipulator);
    screenCaptureHandler = new osgViewer::ScreenCaptureHandler();
    screenCaptureHandler->setKeyEventTakeScreenShot(-1); //no keyboard execution
    screenCaptureHandler->setKeyEventToggleContinuousCapture(-1); //no keyboard execution
//     view->addEventHandler(screenCaptureHandler); //don't think this needs to be added the way I am using it.
    
    addView(view);

    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(windowQt->getGLWidget());
    setLayout(layout);

    timer->start(10);
}

ViewerWidget::~ViewerWidget() //for forward declarations.
{

}

QWidget* ViewerWidget::getGraphicsWidget()
{
  return windowQt->getGLWidget();
}

slc::EventHandler* ViewerWidget::getSelectionEventHandler()
{
  return selectionHandler.get();
}
const slc::Containers& ViewerWidget::getSelections() const
{
  return selectionHandler->getSelections();
}
void ViewerWidget::clearSelections() const
{
  selectionHandler->clearSelections();
}
const osg::Matrixd& ViewerWidget::getCurrentSystem() const
{
  return currentSystem->getMatrix();
}
void ViewerWidget::setCurrentSystem(const osg::Matrixd &mIn)
{
  currentSystem->setMatrix(mIn);
}

const osg::Matrixd& ViewerWidget::getViewSystem() const
{
    return mainCamera->getViewMatrix();
}

void ViewerWidget::myUpdate()
{
  //I am not sure how useful calling optimizer on my generated data is?
  //TODO build large file and run with and without the optimizer to test.
  
  //commented out for osg upgrade to 3.5.6. screwed up structure
  //    to the point of selection not working.
  //remove redundant nodes was screwing up hidden line effect.
//   osgUtil::Optimizer opt;
//   opt.optimize
//   (
//     root,
//     osgUtil::Optimizer::DEFAULT_OPTIMIZATIONS
//     & ~osgUtil::Optimizer::REMOVE_REDUNDANT_NODES
//   );
  
  //set the hidden line state.
  HiddenLineVisitor v(prf::Manager().rootPtr->visual().display().showHiddenLines());
  root->accept(v);
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

    vwr::GLEventWidget *glWidget = new vwr::GLEventWidget(format, this);
    windowQt = new osgQt::GraphicsWindowQt(glWidget);

    camera->setGraphicsContext(windowQt);
    camera->setName("main");
    camera->setRenderOrder(osg::Camera::NESTED_RENDER, 1);
    camera->setClearMask(GL_DEPTH_BUFFER_BIT);

    QPixmap cursorImage(":/resources/images/cursor.svg");
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

void ViewerWidget::paintEvent(QPaintEvent*)
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
    bgCamera->setName("backgrd");
    bgCamera->setGraphicsContext(windowQt);
    bgCamera->setCullingActive(false);
    bgCamera->setAllowEventFocus(false);
    bgCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    bgCamera->setRenderOrder(osg::Camera::NESTED_RENDER, 0);
    bgCamera->setProjectionMatrix(osg::Matrix::ortho2D(0.0, 1.0, 0.0, 1.0));
    bgCamera->addChild(geode.get());
    bgCamera->setNodeMask(mdv::backGroundCamera);

    osg::StateSet* ss = bgCamera->getOrCreateStateSet();
    ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    ss->setAttributeAndModes(new osg::Depth(osg::Depth::LEQUAL, 1.0, 1.0));

    return bgCamera;
}

osg::Camera* ViewerWidget::createGestureCamera()
{
    double quadSize = 10000;

    osg::ref_ptr<osg::Geometry> quad = osg::createTexturedQuadGeometry
      (osg::Vec3(), osg::Vec3(quadSize, 0.0, 0.0), osg::Vec3(0.0, quadSize, 0.0));
    osg::Vec4Array *colorArray = new osg::Vec4Array();
    colorArray->push_back(osg::Vec4(0.0, 0.0, 0.0, 0.6));
    quad->setColorArray(colorArray);
    quad->setColorBinding(osg::Geometry::BIND_OVERALL);
    quad->setNodeMask(mdv::gestureCamera);
    quad->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
    quad->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

    osg::Camera *fadeCamera = new osg::Camera();
    fadeCamera->setName("gesture");
    fadeCamera->setCullingActive(false);
    fadeCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
    fadeCamera->setAllowEventFocus(false);
    fadeCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    fadeCamera->setRenderOrder(osg::Camera::NESTED_RENDER, 4);
    fadeCamera->setProjectionMatrix(osg::Matrix::ortho2D(0.0, 1000.0, 0.0, 1000.0));
    fadeCamera->setViewMatrix(osg::Matrix::identity());
    fadeCamera->setGraphicsContext(windowQt);
    fadeCamera->setNodeMask(mdv::gestureCamera);
    fadeCamera->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    osg::Switch *aSwitch = new osg::Switch();
    aSwitch->addChild(quad.get());
    aSwitch->setAllChildrenOff();
    fadeCamera->addChild(aSwitch);

    return fadeCamera;
}

void ViewerWidget::viewTopDispatched(const msg::Message&)
{
    spaceballManipulator->setView(osg::Vec3d(0.0, 0.0, -1.0), osg::Vec3d(0.0, 1.0, 0.0));
    spaceballManipulator->viewFit();
}

void ViewerWidget::viewFrontDispatched(const msg::Message&)
{
    spaceballManipulator->setView(osg::Vec3d(0.0, 1.0, 0.0), osg::Vec3d(0.0, 0.0, 1.0));
    spaceballManipulator->viewFit();
}

void ViewerWidget::viewRightDispatched(const msg::Message&)
{
    spaceballManipulator->setView(osg::Vec3d(-1.0, 0.0, 0.0), osg::Vec3d(0.0, 0.0, 1.0));
    spaceballManipulator->viewFit();
}

void ViewerWidget::viewIsoDispatched(const msg::Message &)
{
  osg::Vec3d upVector(-1.0, 1.0, 1.0); upVector.normalize();
  osg::Vec3d lookVector(-1.0, 1.0, -1.0); lookVector.normalize();
  spaceballManipulator->setView(lookVector, upVector);
  spaceballManipulator->viewFit();
}

void ViewerWidget::viewFitDispatched(const msg::Message&)
{
  spaceballManipulator->home(1.0);
  spaceballManipulator->viewFit();
}

void ViewerWidget::viewFillDispatched(const msg::Message&)
{
  osg::PolygonMode *pMode = dynamic_cast<osg::PolygonMode*>
    (root->getOrCreateStateSet()->getAttribute(osg::StateAttribute::POLYGONMODE));
  assert(pMode);
  pMode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::FILL);
}

void ViewerWidget::viewLineDispatched(const msg::Message&)
{
  osg::PolygonMode *pMode = dynamic_cast<osg::PolygonMode*>
    (root->getOrCreateStateSet()->getAttribute(osg::StateAttribute::POLYGONMODE));
  assert(pMode);
  pMode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
}

void ViewerWidget::exportOSGDispatched(const msg::Message&)
{
  //ive doesn't appear to be working?
  
  QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), QDir::homePath(),
    tr("Scene (*.osgt *.osgx *.osgb *.osg *.ive)"));
  if (fileName.isEmpty())
    return;
  
  if
  (
    (!fileName.endsWith(QObject::tr(".osgt"))) &&
    (!fileName.endsWith(QObject::tr(".osgx"))) &&
    (!fileName.endsWith(QObject::tr(".osgb"))) &&
    (!fileName.endsWith(QObject::tr(".osg"))) &&
    (!fileName.endsWith(QObject::tr(".ive")))
  )
    fileName += QObject::tr(".osgt");
    
  clearSelections();
  
  osgDB::writeNodeFile(*root, fileName.toStdString());
}

void ViewerWidget::setupDispatcher()
{
  msg::Mask mask;
  
  mask = msg::Response | msg::Post | msg::Add | msg::Feature;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&ViewerWidget::featureAddedDispatched, this, _1)));
  
  mask = msg::Response | msg::Pre | msg::Remove | msg::Feature;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&ViewerWidget::featureRemovedDispatched, this, _1)));
  
  mask = msg::Response | msg::Post | msg::UpdateVisual;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&ViewerWidget::visualUpdatedDispatched, this, _1)));
  
  mask = msg::Request | msg::ViewTop;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&ViewerWidget::viewTopDispatched, this, _1)));
  
  mask = msg::Request | msg::ViewFront;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&ViewerWidget::viewFrontDispatched, this, _1)));
  
  mask = msg::Request | msg::ViewRight;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&ViewerWidget::viewRightDispatched, this, _1)));
  
  mask = msg::Request | msg::ViewIso;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&ViewerWidget::viewIsoDispatched, this, _1)));
  
  mask = msg::Request | msg::ViewFit;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&ViewerWidget::viewFitDispatched, this, _1)));
  
  mask = msg::Request | msg::ViewFill;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&ViewerWidget::viewFillDispatched, this, _1)));
  
  mask = msg::Request | msg::ViewLine;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&ViewerWidget::viewLineDispatched, this, _1)));
  
  mask = msg::Request | msg::ExportOSG;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&ViewerWidget::exportOSGDispatched, this, _1)));
  
  mask = msg::Response | msg::Pre | msg::CloseProject;;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&ViewerWidget::closeProjectDispatched, this, _1)));
  
  mask = msg::Request | msg::SystemReset;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&ViewerWidget::systemResetDispatched, this, _1)));
  
  mask = msg::Request | msg::SystemToggle;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&ViewerWidget::systemToggleDispatched, this, _1)));
  
  mask = msg::Request | msg::ViewToggleHiddenLine;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&ViewerWidget::viewToggleHiddenLinesDispatched, this, _1)));
}

void ViewerWidget::featureAddedDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  prj::Message message = boost::get<prj::Message>(messageIn.payload);
  root->addChild(message.feature->getMainSwitch());
}

void ViewerWidget::featureRemovedDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  prj::Message message = boost::get<prj::Message>(messageIn.payload);
  root->removeChild(message.feature->getMainSwitch());
}

void ViewerWidget::visualUpdatedDispatched(const msg::Message &)
{
  this->myUpdate();
}

void ViewerWidget::closeProjectDispatched(const msg::Message&)
{
  //don't need to keep any children of the viewer.
  root->removeChildren(0, root->getNumChildren());
}

void ViewerWidget::systemResetDispatched(const msg::Message&)
{
  currentSystem->setMatrix(osg::Matrixd::identity());
}

void ViewerWidget::systemToggleDispatched(const msg::Message&)
{
  if (systemSwitch->getValue(0))
    systemSwitch->setAllChildrenOff();
  else
    systemSwitch->setAllChildrenOn();
  
  prf::Manager &manager = prf::manager();
  manager.rootPtr->visual().display().showCurrentSystem() = systemSwitch->getValue(0);
  manager.saveConfig();
}

void ViewerWidget::viewToggleHiddenLinesDispatched(const msg::Message&)
{
  prf::Manager &manager = prf::manager();
  bool oldValue = manager.rootPtr->visual().display().showHiddenLines();
  manager.rootPtr->visual().display().showHiddenLines() = !oldValue;
  manager.saveConfig();
  
  //set the hidden line state.
  HiddenLineVisitor v(!oldValue);
  root->accept(v);
}

void ViewerWidget::screenCapture(const std::string &fp, const std::string &e)
{
  osg::ref_ptr<osgViewer::ScreenCaptureHandler::WriteToFile> wtf = new osgViewer::ScreenCaptureHandler::WriteToFile
  (
    fp, e, osgViewer::ScreenCaptureHandler::WriteToFile::OVERWRITE
  );
  screenCaptureHandler->setFramesToCapture(1);
  screenCaptureHandler->setCaptureOperation(wtf.get());
  screenCaptureHandler->captureNextFrame(*this);
}

QTextStream& ViewerWidget::getInfo(QTextStream &stream) const
{
  stream << endl << QObject::tr("Current System: ");
  gu::osgMatrixOut(stream, getCurrentSystem());
  
  return stream;
}

void StatsHandler::collectWhichCamerasToRenderStatsFor
(
  osgViewer::ViewerBase *viewer,
  osgViewer::ViewerBase::Cameras &cameras
)
{
  osgViewer::ViewerBase::Cameras tempCams;
  viewer->getCameras(tempCams);
  for (auto cam : tempCams)
  {
    if (cam->getName() == "main")
      cameras.push_back(cam);
    if (cam->getName() == "overlay")
      cameras.push_back(cam);
  }
}

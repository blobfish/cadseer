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

#include <boost/variant.hpp>
#include <boost/filesystem.hpp>

#include <QHBoxLayout>
#include <QApplication>
#include <QDir>
#include <QString>
#include <QFileDialog>
#include <QTimer>
#include <QTextStream>
#include <QScreen>
#include <QSvgRenderer>

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
#include <osg/DisplaySettings>

#include <viewer/spaceballmanipulator.h>
#include <viewer/widget.h>
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
#include <selection/visitors.h>
#include <message/dispatch.h>
#include <message/observer.h>
#include <viewer/textcamera.h>
#include <viewer/overlay.h>
#include <feature/base.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <project/serial/xsdcxxoutput/view.h>
#include <application/application.h>
#include <project/project.h>

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

Widget::Widget(osgViewer::ViewerBase::ThreadingModel threadingModel) : QWidget(), osgViewer::CompositeViewer()
{
    observer = std::move(std::unique_ptr<msg::Observer>(new msg::Observer()));
    observer->name = "vwr::Widget";
    setupDispatcher();
    
    osg::DisplaySettings::instance()->setTextShaderTechnique("SIGNED_DISTANCE_FUNCTION"); 
    setThreadingModel(threadingModel);
    setKeyEventSetsDone(0); //stops the viewer from freezing when the escape key is pressed.
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update())); //inherited update slot.

    root = new osg::Group;
    root->setName("root");
    
    osg::ref_ptr<osg::PolygonMode> pm = new osg::PolygonMode;
    root->getOrCreateStateSet()->setAttribute(pm.get());
    prf::RenderStyle::Value renderStyle = prf::Manager().rootPtr->visual().display().renderStyle().get();
    if (renderStyle == prf::RenderStyle::fill)
      viewFillDispatched(msg::Message());
    else if (renderStyle == prf::RenderStyle::triangulation)
      viewTriangulationDispatched(msg::Message());
    //nothing for wireframe yet.
    
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
    
    Overlay *oCamera = new Overlay(windowQt);
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

Widget::~Widget() //for forward declarations.
{

}

QWidget* Widget::getGraphicsWidget()
{
  return windowQt->getGLWidget();
}

slc::EventHandler* Widget::getSelectionEventHandler()
{
  return selectionHandler.get();
}
const slc::Containers& Widget::getSelections() const
{
  return selectionHandler->getSelections();
}
void Widget::clearSelections() const
{
  selectionHandler->clearSelections();
}
const osg::Matrixd& Widget::getCurrentSystem() const
{
  return currentSystem->getMatrix();
}
void Widget::setCurrentSystem(const osg::Matrixd &mIn)
{
  currentSystem->setMatrix(mIn);
}

const osg::Matrixd& Widget::getViewSystem() const
{
    return mainCamera->getViewMatrix();
}

void Widget::myUpdate()
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

QPixmap Widget::loadCursor()
{
  //from
  //https://falsinsoft.blogspot.com/2016/04/qt-snippet-render-svg-to-qpixmap-for.html
  
 const qreal PixelRatio = qApp->primaryScreen()->devicePixelRatio();
 QSvgRenderer SvgRenderer(QString(":/resources/images/cursor.svg"));
 int size = static_cast<int>(32.0 * PixelRatio);
 QPixmap Image(size, size);
 QPainter Painter;

 Image.fill(Qt::transparent);

 Painter.begin(&Image);
 Painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
 SvgRenderer.render(&Painter);
 Painter.end();

 Image.setDevicePixelRatio(PixelRatio);

 return Image;
}

void Widget::createMainCamera(osg::Camera *camera)
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
    int samples = prf::manager().rootPtr->visual().display().samples().get();
    if (samples > 0)
    {
      format.setSampleBuffers(true);
      format.setSamples(samples); //big slowdown.
    }

    vwr::GLEventWidget *glWidget = new vwr::GLEventWidget(format, this);
    windowQt = new osgQt::GraphicsWindowQt(glWidget);

    camera->setGraphicsContext(windowQt);
    camera->setName("main");
    camera->setRenderOrder(osg::Camera::NESTED_RENDER, 1);
    camera->setClearMask(GL_DEPTH_BUFFER_BIT);

    QCursor cursor(loadCursor());//hot point defaults to center.
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

void Widget::paintEvent(QPaintEvent*)
{
    frame();
}

osg::Camera* Widget::createBackgroundCamera()
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

osg::Camera* Widget::createGestureCamera()
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
    fadeCamera->getOrCreateStateSet()->setMode(GL_MULTISAMPLE_ARB, osg::StateAttribute::ON);
    fadeCamera->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    osg::Switch *aSwitch = new osg::Switch();
    aSwitch->addChild(quad.get());
    aSwitch->setAllChildrenOff();
    fadeCamera->addChild(aSwitch);

    return fadeCamera;
}

void Widget::viewTopDispatched(const msg::Message&)
{
    spaceballManipulator->setView(osg::Vec3d(0.0, 0.0, -1.0), osg::Vec3d(0.0, 1.0, 0.0));
    spaceballManipulator->viewFit();
}

void Widget::viewTopCurrentDispatched(const msg::Message&)
{
    osg::Vec3d z = -gu::getZVector(currentSystem->getMatrix());
    osg::Vec3d y = gu::getYVector(currentSystem->getMatrix());
    spaceballManipulator->setView(z, y);
    spaceballManipulator->viewFit();
}

void Widget::viewFrontDispatched(const msg::Message&)
{
    spaceballManipulator->setView(osg::Vec3d(0.0, 1.0, 0.0), osg::Vec3d(0.0, 0.0, 1.0));
    spaceballManipulator->viewFit();
}

void Widget::viewFrontCurrentDispatched(const msg::Message&)
{
    osg::Vec3d z = gu::getZVector(currentSystem->getMatrix());
    osg::Vec3d y = gu::getYVector(currentSystem->getMatrix());
    spaceballManipulator->setView(y, z);
    spaceballManipulator->viewFit();
}

void Widget::viewRightDispatched(const msg::Message&)
{
    spaceballManipulator->setView(osg::Vec3d(-1.0, 0.0, 0.0), osg::Vec3d(0.0, 0.0, 1.0));
    spaceballManipulator->viewFit();
}

void Widget::viewRightCurrentDispatched(const msg::Message&)
{
    osg::Vec3d x = -gu::getXVector(currentSystem->getMatrix());
    osg::Vec3d z = gu::getZVector(currentSystem->getMatrix());
    spaceballManipulator->setView(x, z);
    spaceballManipulator->viewFit();
}

void Widget::viewIsoDispatched(const msg::Message &)
{
  osg::Vec3d upVector(-1.0, 1.0, 1.0); upVector.normalize();
  osg::Vec3d lookVector(-1.0, 1.0, -1.0); lookVector.normalize();
  spaceballManipulator->setView(lookVector, upVector);
  spaceballManipulator->viewFit();
}

void Widget::viewFitDispatched(const msg::Message&)
{
  spaceballManipulator->home(1.0);
  spaceballManipulator->viewFit();
}

void Widget::viewFillDispatched(const msg::Message&)
{
  osg::PolygonMode *pMode = dynamic_cast<osg::PolygonMode*>
    (root->getOrCreateStateSet()->getAttribute(osg::StateAttribute::POLYGONMODE));
  assert(pMode);
  pMode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::FILL);
  
  //update preferences.
  prf::manager().rootPtr->visual().display().renderStyle() = prf::RenderStyle::fill;
  prf::manager().saveConfig();
}

void Widget::viewTriangulationDispatched(const msg::Message&)
{
  osg::PolygonMode *pMode = dynamic_cast<osg::PolygonMode*>
    (root->getOrCreateStateSet()->getAttribute(osg::StateAttribute::POLYGONMODE));
  assert(pMode);
  pMode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
  
  //update preferences.
  prf::manager().rootPtr->visual().display().renderStyle() = prf::RenderStyle::triangulation;
  prf::manager().saveConfig();
}

void Widget::renderStyleToggleDispatched(const msg::Message &)
{
  prf::RenderStyle::Value cStyle = prf::Manager().rootPtr->visual().display().renderStyle().get();
  if (cStyle == prf::RenderStyle::fill)
    viewTriangulationDispatched(msg::Message());
  else if (cStyle == prf::RenderStyle::triangulation)
    viewFillDispatched(msg::Message());
  //wireframe some day.
}

void Widget::exportOSGDispatched(const msg::Message&)
{
  //ive doesn't appear to be working?
  
  QString fileName = QFileDialog::getSaveFileName
  (
    this,
    tr("Save File"),
    QString::fromStdString(prf::manager().rootPtr->project().lastDirectory().get()),
    tr("Scene (*.osgt *.osgx *.osgb *.osg *.ive)")
  );
  if (fileName.isEmpty())
    return;
  
  boost::filesystem::path p = fileName.toStdString();
  prf::manager().rootPtr->project().lastDirectory() = p.remove_filename().string();
  prf::manager().saveConfig();
  
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

void Widget::setupDispatcher()
{
  msg::Mask mask;
  
  mask = msg::Response | msg::Post | msg::Add | msg::Feature;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Widget::featureAddedDispatched, this, _1)));
  
  mask = msg::Response | msg::Pre | msg::Remove | msg::Feature;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Widget::featureRemovedDispatched, this, _1)));
  
  mask = msg::Response | msg::Post | msg::Project | msg::Update | msg::Visual;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Widget::visualUpdatedDispatched, this, _1)));
  
  mask = msg::Request | msg::View | msg::Top;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Widget::viewTopDispatched, this, _1)));
  
  mask = msg::Request | msg::View | msg::Top | msg::Current;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Widget::viewTopCurrentDispatched, this, _1)));
  
  mask = msg::Request | msg::View | msg::Front;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Widget::viewFrontDispatched, this, _1)));
  
  mask = msg::Request | msg::View | msg::Front | msg::Current;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Widget::viewFrontCurrentDispatched, this, _1)));
  
  mask = msg::Request | msg::View | msg::Right;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Widget::viewRightDispatched, this, _1)));
  
  mask = msg::Request | msg::View | msg::Right | msg::Current;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Widget::viewRightCurrentDispatched, this, _1)));
  
  mask = msg::Request | msg::View | msg::Iso;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Widget::viewIsoDispatched, this, _1)));
  
  mask = msg::Request | msg::View | msg::Fit;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Widget::viewFitDispatched, this, _1)));
  
  mask = msg::Request | msg::View | msg::Fill;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Widget::viewFillDispatched, this, _1)));
  
  mask = msg::Request | msg::View | msg::Triangulation;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Widget::viewTriangulationDispatched, this, _1)));
  
  mask = msg::Request | msg::View | msg::RenderStyle | msg::Toggle;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Widget::renderStyleToggleDispatched, this, _1)));
  
  mask = msg::Request | msg::Export | msg::OSG;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Widget::exportOSGDispatched, this, _1)));
  
  mask = msg::Response | msg::Pre | msg::Close | msg::Project;;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Widget::closeProjectDispatched, this, _1)));
  
  mask = msg::Request | msg::SystemReset;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Widget::systemResetDispatched, this, _1)));
  
  mask = msg::Request | msg::SystemToggle;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Widget::systemToggleDispatched, this, _1)));
  
  mask = msg::Request | msg::View | msg::Show | msg::HiddenLine;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Widget::showHiddenLinesDispatched, this, _1)));
  
  mask = msg::Request | msg::View | msg::Hide | msg::HiddenLine;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Widget::hideHiddenLinesDispatched, this, _1)));
  
  mask = msg::Request | msg::View | msg::Toggle | msg::HiddenLine;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Widget::viewToggleHiddenLinesDispatched, this, _1)));
  
  mask = msg::Request | msg::View | msg::Show | msg::ThreeD;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Widget::showThreeDDispatched, this, _1)));
  
  mask = msg::Request | msg::View | msg::Hide | msg::ThreeD;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Widget::hideThreeDDispatched, this, _1)));
  
  mask = msg::Request | msg::View | msg::Toggle | msg::ThreeD;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Widget::threeDToggleDispatched, this, _1)));
  
  mask = msg::Response | msg::Post | msg::Open | msg::Project;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Widget::projectOpenedDispatched, this, _1)));
  
  mask = msg::Response | msg::Post | msg::Project | msg::Update | msg::Model;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Widget::projectUpdatedDispatched, this, _1)));
}

void Widget::featureAddedDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  prj::Message message = boost::get<prj::Message>(messageIn.payload);
  root->addChild(message.feature->getMainSwitch());
}

void Widget::featureRemovedDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  prj::Message message = boost::get<prj::Message>(messageIn.payload);
  root->removeChild(message.feature->getMainSwitch());
}

void Widget::visualUpdatedDispatched(const msg::Message &)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  this->myUpdate();
}

void Widget::closeProjectDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  //don't need to keep any children of the viewer.
  root->removeChildren(0, root->getNumChildren());
}

void Widget::systemResetDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  currentSystem->setMatrix(osg::Matrixd::identity());
}

void Widget::systemToggleDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  if (systemSwitch->getValue(0))
    systemSwitch->setAllChildrenOff();
  else
    systemSwitch->setAllChildrenOn();
  
  prf::Manager &manager = prf::manager();
  manager.rootPtr->visual().display().showCurrentSystem() = systemSwitch->getValue(0);
  manager.saveConfig();
}

void Widget::showHiddenLinesDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  HiddenLineVisitor v(true);
  root->accept(v);
  
  //make sure prefs are in sync.
  prf::manager().rootPtr->visual().display().showHiddenLines() = true;
  prf::manager().saveConfig();
}

void Widget::hideHiddenLinesDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  HiddenLineVisitor v(false);
  root->accept(v);
  
  //make sure prefs are in sync.
  prf::manager().rootPtr->visual().display().showHiddenLines() = false;
  prf::manager().saveConfig();
}

void Widget::viewToggleHiddenLinesDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  prf::Manager &manager = prf::manager();
  bool oldValue = manager.rootPtr->visual().display().showHiddenLines();
  manager.rootPtr->visual().display().showHiddenLines() = !oldValue;
  manager.saveConfig();
  
  //set the hidden line state.
  HiddenLineVisitor v(!oldValue);
  root->accept(v);
}

void Widget::showThreeDDispatched(const msg::Message &msgIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  slc::MainSwitchVisitor v(boost::get<vwr::Message>(msgIn.payload).featureId);
  root->accept(v);
  assert(v.out);
  if (!v.out)
    return;
  
  if (v.out->getNewChildDefaultValue()) //already shown.
    return;
  
  v.out->setAllChildrenOn();
  
  msg::Message mOut(msg::Response | msg::View | msg::Show | msg::ThreeD);
  mOut.payload = msgIn.payload;
  observer->outBlocked(mOut);
  
  //see message in toggled for why we do this after the message.
  HiddenLineVisitor hlv(prf::manager().rootPtr->visual().display().showHiddenLines());
  v.out->accept(hlv);
}

void Widget::hideThreeDDispatched(const msg::Message &msgIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  slc::MainSwitchVisitor v(boost::get<vwr::Message>(msgIn.payload).featureId);
  root->accept(v);
  assert(v.out);
  if (!v.out)
    return;
  
  if (!v.out->getNewChildDefaultValue()) //already hidden.
    return;
  
  v.out->setAllChildrenOff();
  
  msg::Message mOut(msg::Response | msg::View | msg::Hide | msg::ThreeD);
  mOut.payload = msgIn.payload;
  observer->outBlocked(mOut);
}

void Widget::threeDToggleDispatched(const msg::Message &msgIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  slc::MainSwitchVisitor v(boost::get<vwr::Message>(msgIn.payload).featureId);
  root->accept(v);
  assert(v.out);
  if (!v.out)
    return;
  msg::Mask maskOut; //notice we are not responding with Msg::Toggle that got us here.
  if (v.out->getNewChildDefaultValue())
  {
    v.out->setAllChildrenOff();
    maskOut = msg::Response | msg::View | msg::Hide | msg::ThreeD;
  }
  else
  {
    v.out->setAllChildrenOn();
    maskOut = msg::Response | msg::View | msg::Show | msg::ThreeD;
  }
  
  msg::Message mOut(maskOut);
  mOut.payload = msgIn.payload;
  observer->outBlocked(mOut);
  
  //we don't generate visuals until needed. so hidden and inactive features
  //have none or bogus osg data. This is the reason for the response message above.
  //It triggers the project to update the feature's visual data. Now we can run
  //the hidden line visitor over the generated osg data.
  HiddenLineVisitor hlv(prf::manager().rootPtr->visual().display().showHiddenLines());
  v.out->accept(hlv);
}

void Widget::projectOpenedDispatched(const msg::Message &)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  serialRead();
}

void Widget::projectUpdatedDispatched(const msg::Message &)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  serialWrite();
}

void Widget::screenCapture(const std::string &fp, const std::string &e)
{
  osg::ref_ptr<osgViewer::ScreenCaptureHandler::WriteToFile> wtf = new osgViewer::ScreenCaptureHandler::WriteToFile
  (
    fp, e, osgViewer::ScreenCaptureHandler::WriteToFile::OVERWRITE
  );
  screenCaptureHandler->setFramesToCapture(1);
  screenCaptureHandler->setCaptureOperation(wtf.get());
  screenCaptureHandler->captureNextFrame(*this);
}

QTextStream& Widget::getInfo(QTextStream &stream) const
{
  stream << endl << QObject::tr("Current System: ");
  gu::osgMatrixOut(stream, getCurrentSystem());
  
  return stream;
}

//restore states from serialize
class SerialInViewVisitor : public osg::NodeVisitor
{
public:
  SerialInViewVisitor(const prj::srl::States &statesIn) :
  NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
  states(statesIn)
  {
    observer.name = "vwr::Widget::SerialIn";
  }
  virtual void apply(osg::Switch &switchIn) override
  {
    std::string userValue;
    if (switchIn.getUserValue<std::string>(gu::idAttributeTitle, userValue))
    {
      for (const auto &s : states.array())
      {
        if (userValue == s.id())
        {
          msg::Payload payload((vwr::Message(gu::stringToId(userValue))));
          if (s.visible())
          {
            switchIn.setAllChildrenOn();
            observer.outBlocked(msg::Message(msg::Mask(msg::Response | msg::View | msg::Show | msg::ThreeD), payload));
          }
          else
          {
            switchIn.setAllChildrenOff();
            observer.outBlocked(msg::Message(msg::Mask(msg::Response | msg::View | msg::Hide | msg::ThreeD), payload));
          }
          break;
        }
      }
    }
    
    //only interested in top level children, so don't need to call traverse here.
  }
protected:
  const prj::srl::States &states;
  msg::Observer observer;
};

void Widget::serialRead()
{
  boost::filesystem::path file = static_cast<app::Application*>(qApp)->getProject()->getSaveDirectory();
  file /= "view.xml";
  if (!boost::filesystem::exists(file))
    return;
  
  auto sView = prj::srl::view(file.string(), ::xml_schema::Flags::dont_validate);
  SerialInViewVisitor v(sView->states());
  root->accept(v);
  
  if (sView->csys().present())
  {
    const auto &mIn = sView->csys().get();
    osg::Matrixd m
    (
      mIn.i0j0(), mIn.i0j1(), mIn.i0j2(), mIn.i0j3(),
      mIn.i1j0(), mIn.i1j1(), mIn.i1j2(), mIn.i1j3(),
      mIn.i2j0(), mIn.i2j1(), mIn.i2j2(), mIn.i2j3(),
      mIn.i3j0(), mIn.i3j1(), mIn.i3j2(), mIn.i3j3()
    );
    spaceballManipulator->setByMatrix(m);
  }
  
  if (sView->ortho().present())
  {
    const auto &p = sView->ortho().get();
    mainCamera->setProjectionMatrixAsOrtho(p.left(), p.right(), p.bottom(), p.top(), p.near(), p.far());
  }
}

//get all states to serialize
class SerialOutVisitor : public osg::NodeVisitor
{
public:
  SerialOutVisitor(prj::srl::States &statesIn) : NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), states(statesIn){}
  virtual void apply(osg::Switch &switchIn) override
  {
    std::string userValue;
    if (switchIn.getUserValue<std::string>(gu::idAttributeTitle, userValue))
      states.array().push_back(prj::srl::State(userValue, switchIn.getNewChildDefaultValue()));
    
    //only interested in top level children, so don't need to call traverse here.
  }
protected:
  prj::srl::States &states;
};

void Widget::serialWrite()
{
  prj::srl::States states;
  SerialOutVisitor v(states);
  root->accept(v);
  prj::srl::View svOut(states);
  
  osg::Matrixd m = mainCamera->getViewMatrix();
  prj::srl::Matrixd mOut
  (
    m(0,0), m(0,1), m(0,2), m(0,3),
    m(1,0), m(1,1), m(1,2), m(1,3),
    m(2,0), m(2,1), m(2,2), m(2,3),
    m(3,0), m(3,1), m(3,2), m(3,3)
  );
  svOut.csys() = mOut;
  
  double left, right, bottom, top, near, far;
  if (mainCamera->getProjectionMatrixAsOrtho(left, right, bottom, top, near, far))
    svOut.ortho() = prj::srl::Ortho(left, right, bottom, top, near, far);
  
  boost::filesystem::path file = static_cast<app::Application*>(qApp)->getProject()->getSaveDirectory();
  file /= "view.xml";
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(file.string());
  prj::srl::view(stream, svOut, infoMap);
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

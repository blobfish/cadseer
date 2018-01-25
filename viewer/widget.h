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

#ifndef VWR_WIDGET_H
#define VWR_WIDGET_H

#include <memory>

#include <QWidget>

#include <osg/ref_ptr>
#include <osgViewer/CompositeViewer>
#include <osgViewer/ViewerEventHandlers> //for subclass of stats handler.

#include <selection/container.h>

class QTextStream;
namespace osgQt{class GraphicsWindowQt;}
namespace vwr{class SpaceballManipulator;}
namespace slc{class EventHandler; class OverlayHandler;}
namespace lbr{class CSysDragger; class CSysCallBack;}
namespace msg{class Message; class Observer;}

namespace vwr
{
class Widget : public QWidget, public osgViewer::CompositeViewer
{
    Q_OBJECT
public:
    Widget(osgViewer::ViewerBase::ThreadingModel threadingModel=osgViewer::CompositeViewer::SingleThreaded);
    virtual ~Widget() override;
    
    virtual void paintEvent(QPaintEvent* event);
    void myUpdate();
    osg::Group* getRoot(){return root;}
    slc::EventHandler* getSelectionEventHandler();
    const slc::Containers& getSelections() const;
    void clearSelections() const;
    const osg::Matrixd& getCurrentSystem() const;
    void setCurrentSystem(const osg::Matrixd &mIn);
    const osg::Matrixd& getViewSystem() const;
    QTextStream& getInfo(QTextStream &stream) const;
    
    //! first is file path without dot and extension. second is extension without the dot
    void screenCapture(const std::string &, const std::string &);
    
    QWidget* getGraphicsWidget(); //!< needed to forward spaceball events.

protected:
    QPixmap loadCursor();
    void createMainCamera(osg::Camera *camera);
    osg::Camera* createBackgroundCamera();
    osg::Camera* createGestureCamera();
    osg::Camera* mainCamera;
    osg::ref_ptr<osg::Group> root;
    osg::ref_ptr<osg::Switch> infoSwitch;
    osg::ref_ptr<slc::EventHandler> selectionHandler;
    osg::ref_ptr<slc::OverlayHandler> overlayHandler;
    osg::ref_ptr<vwr::SpaceballManipulator> spaceballManipulator;
    osg::ref_ptr<osg::Switch> systemSwitch;
    osg::ref_ptr<lbr::CSysDragger> currentSystem;
    osg::ref_ptr<lbr::CSysCallBack> currentSystemCallBack;
    osg::ref_ptr<osgViewer::ScreenCaptureHandler> screenCaptureHandler;
    int glWidgetWidth;
    int glWidgetHeight;
    osgQt::GraphicsWindowQt *windowQt;
    std::unique_ptr<msg::Observer> observer;
    void setupDispatcher();
    void featureAddedDispatched(const msg::Message &);
    void featureRemovedDispatched(const msg::Message &);
    void visualUpdatedDispatched(const msg::Message &);
    void viewTopDispatched(const msg::Message &);
    void viewTopCurrentDispatched(const msg::Message &);
    void viewFrontDispatched(const msg::Message &);
    void viewFrontCurrentDispatched(const msg::Message &);
    void viewRightDispatched(const msg::Message &);
    void viewRightCurrentDispatched(const msg::Message &);
    void viewIsoDispatched(const msg::Message &);
    void viewFitDispatched(const msg::Message &);
    void viewFillDispatched(const msg::Message &);
    void viewTriangulationDispatched(const msg::Message &);
    void viewToggleHiddenLinesDispatched(const msg::Message&);
    void showHiddenLinesDispatched(const msg::Message&);
    void hideHiddenLinesDispatched(const msg::Message&);
    void exportOSGDispatched(const msg::Message &);
    void closeProjectDispatched(const msg::Message &);
    void systemResetDispatched(const msg::Message &);
    void systemToggleDispatched(const msg::Message &);
    void renderStyleToggleDispatched(const msg::Message &);
    void showThreeDDispatched(const msg::Message &);
    void hideThreeDDispatched(const msg::Message &);
    void threeDToggleDispatched(const msg::Message &);
    void projectOpenedDispatched(const msg::Message &);
    void projectUpdatedDispatched(const msg::Message &);
    
    void serialRead();
    void serialWrite();
};

class StatsHandler : public osgViewer::StatsHandler
{
public:
  virtual void collectWhichCamerasToRenderStatsFor
    (osgViewer::ViewerBase *, osgViewer::ViewerBase::Cameras &) override;
};

}

#endif // VWR_WIDGET_H

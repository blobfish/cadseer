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

#ifndef VIEWERWIDGET_H
#define VIEWERWIDGET_H

#include <QWidget>
#include <QTimer>
#include <osgViewer/CompositeViewer>
#ifndef Q_MOC_RUN
#include "selectioneventhandler.h"
#include "spaceballmanipulator.h"
#endif

namespace osgQt
{
class GraphicsWindowQt;
}

class ViewerWidget : public QWidget, public osgViewer::CompositeViewer
{
    Q_OBJECT
public:
    ViewerWidget(osgViewer::ViewerBase::ThreadingModel threadingModel=osgViewer::CompositeViewer::SingleThreaded);
    virtual void paintEvent(QPaintEvent* event);
    void update();
    osg::Group* getRoot(){return root;}
    const SelectionContainers& getSelections() const {return selectionHandler->getSelections();}
    void clearSelections() const {selectionHandler->clearSelections();}

public Q_SLOTS:
    void setSelectionMask(const int &maskIn);
    void viewTopSlot();
    void viewFrontSlot();
    void viewRightSlot();
    void viewFitSlot();
    void writeOSGSlot();

protected:
    void createMainCamera(osg::Camera *camera);
    osg::Camera* createBackgroundCamera();
    osg::Camera* createGestureCamera();
    void setupCommands();
    QTimer _timer;
    osg::ref_ptr<osg::Group> root;
    osg::ref_ptr<osg::Switch> infoSwitch;
    osg::ref_ptr<SelectionEventHandler> selectionHandler;
    osg::ref_ptr<osgGA::SpaceballManipulator> spaceballManipulator;
    int glWidgetWidth;
    int glWidgetHeight;
    osgQt::GraphicsWindowQt *windowQt;
};

class VisibleVisitor : public osg::NodeVisitor
{
public:
    VisibleVisitor(bool visIn);
    virtual void apply(osg::Switch &aSwitch);
protected:
    bool visibility;
};

#endif // VIEWERWIDGET_H

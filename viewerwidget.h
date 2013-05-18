#ifndef VIEWERWIDGET_H
#define VIEWERWIDGET_H

#include <QWidget>
#include <QTimer>
#include <osgViewer/CompositeViewer>
#include "selectioneventhandler.h"
#include "spaceballmanipulator.h"

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

public slots:
    void setSelectionMask(const int &maskIn);
    void hideSelected();
    void showAll();
    void viewTopSlot();
    void viewFrontSlot();
    void viewRightSlot();
    void viewFitSlot();

protected:
    osg::Camera* createMainCamera();
    osg::Camera* createBackgroundCamera();
    osg::Camera* createGestureCamera();
    void setupCommands();
    void addFade();
    QTimer _timer;
    osg::ref_ptr<osg::Group> root;
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

class VisitorHide : public osg::NodeVisitor
{
public:
    VisitorHide(bool visIn, int hashIn);
    virtual void apply(osg::Switch &aSwitch);
protected:
    bool visibility;
    int hash;
};

class VisitorShowAll : public osg::NodeVisitor
{
public:
    VisitorShowAll();
    virtual void apply(osg::Switch &aSwitch);
};


#endif // VIEWERWIDGET_H

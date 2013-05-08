#ifndef VIEWERWIDGET_H
#define VIEWERWIDGET_H

#include <QWidget>
#include <QTimer>
#include <osgViewer/CompositeViewer>
#include "selectioneventhandler.h"

class ViewerWidget : public QWidget, public osgViewer::CompositeViewer
{
    Q_OBJECT
public:
    ViewerWidget(osgViewer::ViewerBase::ThreadingModel threadingModel=osgViewer::CompositeViewer::SingleThreaded);
    osg::Camera* createCamera();
    virtual void paintEvent(QPaintEvent* event);
    void update();
    osg::Group* getRoot(){return root;}

public slots:
    void setSelectionMask(const int &maskIn);

protected:
    void addBackground();
    QTimer _timer;
    osg::ref_ptr<osg::Group> root;
    osg::ref_ptr<SelectionEventHandler> selectionHandler;
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

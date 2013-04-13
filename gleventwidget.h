#ifndef GLEVENTWIDGET_H
#define GLEVENTWIDGET_H

#include <osgQt/GraphicsWindowQt>

class SpaceballOSGEvent;

class GLEventWidget : public osgQt::GLWidget
{
    typedef osgQt::GLWidget inherited;
public:
    GLEventWidget(const QGLFormat& format, QWidget* parent = NULL, const QGLWidget* shareWidget = NULL,
                  Qt::WindowFlags f = 0, bool forwardKeyEvents = false);
protected:
    virtual bool event(QEvent* event);

private:
    osg::ref_ptr<SpaceballOSGEvent> convertEvent(QEvent* qEvent);
};

#endif // GLEVENTWIDGET_H

#ifndef GESTUREHANDLER_H
#define GESTUREHANDLER_H

#include <osgGA/GUIEventHandler>

class GestureHandler : public osgGA::GUIEventHandler
{
public:
    GestureHandler();
protected:
    virtual bool handle(const osgGA::GUIEventAdapter& eventAdapter,
                        osgGA::GUIActionAdapter& actionAdapter, osg::Object *object,
                        osg::NodeVisitor *nodeVistor);
    bool rightButtonDown;
    osg::ref_ptr<osg::Geode> geode;
    osg::ref_ptr<osg::Switch> gestureSwitch;
    osg::ref_ptr<osg::Camera> gestureCamera;
    osg::ref_ptr<osg::PositionAttitudeTransform> transform;
};

#endif // GESTUREHANDLER_H

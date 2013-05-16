#ifndef GESTUREHANDLER_H
#define GESTUREHANDLER_H

#include <osgGA/GUIEventHandler>

class GestureHandler : public osgGA::GUIEventHandler
{
public:
    GestureHandler(osg::Camera *cameraIn);
protected:
    virtual bool handle(const osgGA::GUIEventAdapter& eventAdapter,
                        osgGA::GUIActionAdapter& actionAdapter, osg::Object *object,
                        osg::NodeVisitor *nodeVistor);
    double calculateSprayRadius(int nodeCount);
    void startDrag(const osgGA::GUIEventAdapter& eventAdapter);
    void constructMenu();
    std::vector<osg::Vec3> buildNodeLocations(osg::Vec3 direction, int nodeCount);
    void spraySubNodes(osg::Vec3 cursorLocation);
    void contractSubNodes();
    bool rightButtonDown;
    bool dragStarted;
    osg::ref_ptr<osg::Switch> gestureSwitch;
    osg::ref_ptr<osg::Camera> gestureCamera;
    osg::ref_ptr<osg::MatrixTransform> startNode;
    osg::ref_ptr<osg::MatrixTransform> currentNode;
    osg::Matrixd aggregateMatrix;
    bool currentNodeLeft;

    double iconRadius;
    double includedAngle; //in degrees
};

class GestureAllSwitchesOffVisitor : public osg::NodeVisitor
{
public:
    GestureAllSwitchesOffVisitor();
    virtual void apply(osg::Switch &aSwitch);
protected:
    bool visibility;
    int hash;
};

#endif // GESTUREHANDLER_H

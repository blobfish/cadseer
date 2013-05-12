#include <iostream>

#include <QImage>
#include <QGLWidget>

#include <osgViewer/View>
#include <osg/Texture2D>
#include <osg/PositionAttitudeTransform>

#include "gesturehandler.h"
#include "nodemaskdefs.h"

GestureHandler::GestureHandler() : osgGA::GUIEventHandler(), rightButtonDown(false)
{
    QImage qImageBase(":/resources/images/viewBase.svg");
    //I am hoping that osg will free this memory.
    QImage *qImage = new QImage(QGLWidget::convertToGLFormat(qImageBase));
    unsigned char *imageData = qImage->bits();
    osg::ref_ptr<osg::Image> osgImage = new osg::Image();
    osgImage->setImage(qImage->width(), qImage->height(), 1, GL_RGBA, GL_RGBA,
                       GL_UNSIGNED_BYTE, imageData, osg::Image::USE_NEW_DELETE, 1);

    osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;


    texture->setImage(osgImage.get());
    float width = static_cast<float>(qImage->width());
    float height = static_cast<float>(qImage->height());
    osg::ref_ptr<osg::Drawable> quad = osg::createTexturedQuadGeometry
            (osg::Vec3(0.0, 0.0, 0.0), osg::Vec3(width, 0.0, 0.0f),
             osg::Vec3(0.0, height, 0.0f));
    quad->getOrCreateStateSet()->setTextureAttributeAndModes(0, texture.get());
    quad->getOrCreateStateSet()->setMode(GL_DEPTH_TEST,osg::StateAttribute::OFF);
    geode = new osg::Geode;
    geode->addDrawable(quad.get());

    osg::PositionAttitudeTransform *center = new osg::PositionAttitudeTransform();
    center->setPosition(osg::Vec3(-width/2.0, -height/2.0, 0.0));
    center->addChild(geode);

    transform = new osg::PositionAttitudeTransform();
    transform->addChild(center);
}


bool GestureHandler::handle(const osgGA::GUIEventAdapter& eventAdapter,
                            osgGA::GUIActionAdapter& actionAdapter, osg::Object *object,
                            osg::NodeVisitor *nodeVistor)
{
    if (!gestureSwitch.valid())
    {
        osg::View* view = actionAdapter.asView();
        if (!view)
            return false;
        gestureCamera = view->getSlave(1)._camera;
        if (!gestureCamera.valid())
            return false;
        gestureSwitch = dynamic_cast<osg::Switch *>(gestureCamera->getChild(0));
        if (!gestureSwitch.valid())
            return false;
        gestureSwitch->addChild(transform);
    }

    if (eventAdapter.getButton() == osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON)
    {
        if (eventAdapter.getEventType() == osgGA::GUIEventAdapter::PUSH)
        {
            gestureSwitch->setAllChildrenOn();
            rightButtonDown = true;

            osg::Matrixd projection = gestureCamera->getProjectionMatrix();
            osg::Matrixd window = gestureCamera->getViewport()->computeWindowMatrix();
            osg::Matrixd transformation = projection * window;
            transformation = osg::Matrixd::inverse(transformation);

            osg::Vec3 position(osg::Vec3d(eventAdapter.getX(), eventAdapter.getY(), 0.0));
            position = transformation * position;


            double aspect = static_cast<double>(eventAdapter.getWindowWidth()) /
                    static_cast<double>(eventAdapter.getWindowHeight());
            double sizeScale = 1000.0 / static_cast<double>(eventAdapter.getWindowWidth());

            transform->setPosition(position);
            transform->setScale(osg::Vec3d(sizeScale, aspect * sizeScale, 0.0));
        }

        if (eventAdapter.getEventType() == osgGA::GUIEventAdapter::RELEASE)
        {
            rightButtonDown = false;
            gestureSwitch->setAllChildrenOff();
        }
    }

    if (eventAdapter.getEventType() == osgGA::GUIEventAdapter::DRAG)
    {
//        if (rightButtonDown)
//            std::cout << "right button drag" << std::endl;
    }

    return false;
}

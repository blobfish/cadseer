#include <QImage>
#include <QGLWidget>

#include <osg/Switch>
#include <osg/MatrixTransform>
#include <osg/Texture2D>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/LineWidth>

#include "gesturenode.h"
#include "../nodemaskdefs.h"

osg::MatrixTransform *GestureNode::buildMenuNode(const char *resourceName)
{
    osg::ref_ptr<osg::MatrixTransform> mainNode = buildCommonNode(resourceName);
    mainNode->setNodeMask(NodeMask::gestureMenu);
    return mainNode.release();
}

osg::MatrixTransform* GestureNode::buildCommandNode(const char *resourceName)
{
    osg::ref_ptr<osg::MatrixTransform> mainNode = buildCommonNode(resourceName);
    mainNode->setNodeMask(NodeMask::gestureCommand);
    return mainNode.release();
}

osg::MatrixTransform* GestureNode::buildCommonNode(const char *resourceName)
{
    osg::ref_ptr<osg::MatrixTransform> mainNode = new osg::MatrixTransform();

    osg::ref_ptr<osg::Switch> lineSwitch = new osg::Switch();
    lineSwitch->addChild(buildLineGeode());
    mainNode->addChild(lineSwitch.get());

    osg::ref_ptr<osg::Switch> iconSwitch = new osg::Switch();
    iconSwitch->addChild(buildIconGeode(resourceName));
    mainNode->addChild(iconSwitch.get());

    return mainNode.release();
}

osg::Geode* GestureNode::buildIconGeode(const char *resourceName)
{
    QImage qImageBase(resourceName);
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
            (osg::Vec3(width/-2.0, height/-2.0, 0.0), osg::Vec3(width, 0.0, 0.0f),
             osg::Vec3(0.0, height, 0.0f));
    quad->getOrCreateStateSet()->setTextureAttributeAndModes(0, texture.get());
    quad->getOrCreateStateSet()->setMode(GL_DEPTH_TEST,osg::StateAttribute::OFF);
    osg::Geode *geode = new osg::Geode();
    geode->addDrawable(quad.get());

    return geode;
}

osg::Geode* GestureNode::buildLineGeode()
{
    osg::ref_ptr<osg::Geometry> geometryLine = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> points = new osg::Vec3Array();
    points->push_back(osg::Vec3(0.0, 0.0, 0.0));
    points->push_back(osg::Vec3(100.0, 0.0, 0.0));
    geometryLine->setVertexArray(points);

    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    colors->push_back(osg::Vec4(0.0, 0.0, 0.0, 1.0));
    geometryLine->setColorArray(colors.get());
    geometryLine->setColorBinding(osg::Geometry::BIND_OVERALL);

    geometryLine->getOrCreateStateSet()->setAttribute(new osg::LineWidth(2.0f));
    geometryLine->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    geometryLine->getOrCreateStateSet()->setMode(GL_DEPTH_TEST,osg::StateAttribute::OFF);

    geometryLine->addPrimitiveSet(new osg::DrawArrays(GL_LINES, 0, 2));

    osg::Geode *geode = new osg::Geode();
    geode->addDrawable(geometryLine.get());

    return geode;
}

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

#include <assert.h>

#include <QImage>
#include <QGLWidget>
#include <QDir>
#include <QDebug>

#include <osg/Switch>
#include <osg/MatrixTransform>
#include <osg/Texture2D>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/LineWidth>
#include <osgDB/ReadFile>
#include <osg/BlendFunc>

#include <gesture/gesturenode.h>
#include <modelviz/nodemaskdefs.h>

osg::MatrixTransform *gsn::buildMenuNode(const char *resourceName)
{
    osg::ref_ptr<osg::MatrixTransform> mainNode = buildCommonNode(resourceName);
    mainNode->setNodeMask(NodeMaskDef::gestureMenu);
    return mainNode.release();
}

osg::MatrixTransform* gsn::buildCommandNode(const char *resourceName)
{
    osg::ref_ptr<osg::MatrixTransform> mainNode = buildCommonNode(resourceName);
    mainNode->setNodeMask(NodeMaskDef::gestureCommand);
    return mainNode.release();
}

osg::MatrixTransform* gsn::buildCommonNode(const char *resourceName)
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

osg::Geode* gsn::buildIconGeode(const char *resourceName)
{
  //alright this sucks. getting svg icons onto geode is a pain.
  //I tried something I found on the web, but looks like ass.
  //the openscenegraph svg reader only takes a filename.
  //so we will write out the resource files to the temp directory
  //so we can pass a file name to the svg reader. I don't like it either.
  QString alteredFileName(resourceName);
  alteredFileName.remove(":");
  alteredFileName.replace("/", "_");
  QDir tempDirectory = QDir::temp();
  QString resourceFileName(tempDirectory.absolutePath() + "/" + alteredFileName);
  if (!tempDirectory.exists(alteredFileName))
  {
    QFile resourceFile(resourceName);
    if (!resourceFile.open(QIODevice::ReadOnly | QIODevice::Text))
      qDebug() << "couldn't resource file";
    QByteArray buffer = resourceFile.readAll();
    resourceFile.close();
     
    QFile newFile(resourceFileName);
    if (!newFile.open(QIODevice::WriteOnly | QIODevice::Text))
      qDebug() << "couldn't open new temp file";
    qDebug() << "newfilename is: " << resourceFileName;
    newFile.write(buffer);
    newFile.close();
  }
  
  osg::ref_ptr<osg::Image> image = osgDB::readImageFile(resourceFileName.toStdString());
  assert(image.valid());
  
  osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D();
  texture->setImage(image);
  texture->setDataVariance(osg::Object::DYNAMIC);
  texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
  texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
  texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
  texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
  
  osg::Vec2Array* textureCoordinates = new osg::Vec2Array();
  
  double radius = 32.0;
  double sides = 32.0;
  osg::Vec3d currentPoint(radius, 0.0, 0.0);
  std::vector<osg::Vec3d> points;
  points.push_back(osg::Vec3d(0.0, 0.0, 0.0));
  textureCoordinates->push_back(osg::Vec2(0.5, 0.5));
  osg::Quat rotation(2 * M_PI / sides , osg::Vec3d(0.0, 0.0, 1.0));
  for (int index = 0; index < sides; ++index)
  {
    points.push_back(currentPoint);
    osg::Vec3 tempPointVec(currentPoint);
    tempPointVec.normalize();
    osg::Vec2 tempTextVec(tempPointVec.x(), tempPointVec.y());
    textureCoordinates->push_back((tempTextVec * 0.5) + osg::Vec2(0.5, 0.5));
    currentPoint = rotation * currentPoint;
  }
  points.push_back(osg::Vec3d(radius, 0.0, 0.0));
  textureCoordinates->push_back(textureCoordinates->at(1));
  
  osg::ref_ptr<osg::BlendFunc> blendFunc = new osg::BlendFunc;
  blendFunc->setFunction(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  
  osg::Geometry *geometry = new osg::Geometry();
  geometry->getOrCreateStateSet()->setTextureAttributeAndModes(0, texture.get());
  geometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
  geometry->getOrCreateStateSet()->setAttributeAndModes(blendFunc);
  geometry->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
  
  osg::Vec3Array *vertices = new osg::Vec3Array();
  std::vector<osg::Vec3d>::const_iterator it;
  for (it = points.begin(); it != points.end(); ++it)
    vertices->push_back(*it);
  geometry->setVertexArray(vertices);
  
  geometry->setTexCoordArray(0, textureCoordinates);
  
  osg::Vec3Array *normals = new osg::Vec3Array();
  normals->push_back(osg::Vec3(0.0, 0.0, 1.0));
  geometry->setNormalArray(normals);
  geometry->setNormalBinding(osg::Geometry::BIND_OVERALL);
  
  geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, 0, vertices->size()));
  
  osg::Geode *geode = new osg::Geode();
  geode->addDrawable(geometry);
  return geode;
  
/* this was looking terrible
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
*/
}

osg::Geode* gsn::buildLineGeode()
{
    osg::ref_ptr<osg::Geometry> geometryLine = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> points = new osg::Vec3Array();
    points->push_back(osg::Vec3(0.0, 0.0, 0.0));
    points->push_back(osg::Vec3(100.0, 0.0, 0.0));
    geometryLine->setVertexArray(points);

    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    colors->push_back(osg::Vec4(0.0, 0.0, 0.0, 1.0));
    colors->push_back(osg::Vec4(0.0, 0.0, 0.0, 1.0));
    geometryLine->setColorArray(colors.get());
    geometryLine->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

    geometryLine->getOrCreateStateSet()->setAttribute(new osg::LineWidth(2.0f));
    geometryLine->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    geometryLine->getOrCreateStateSet()->setMode(GL_DEPTH_TEST,osg::StateAttribute::OFF);

    geometryLine->addPrimitiveSet(new osg::DrawArrays(GL_LINES, 0, 2));

    osg::Geode *geode = new osg::Geode();
    geode->addDrawable(geometryLine.get());

    return geode;
}

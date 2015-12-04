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

#include <iostream>

#include <QDir>
#include <QFile>

#include <osgDB/ReadFile>
#include <osgManipulator/Dragger>

#include "circlebuilder.h"
#include "cylinderbuilder.h"
#include "spherebuilder.h"
#include "conebuilder.h"
#include "torusbuilder.h"
#include "iconbuilder.h"
#include "geometrylibraryprivate.h"
#include "geometrylibrary.h"

using namespace lbr;

Manager& Manager::getManager()
{
  static Manager manager;
  return manager;
}

Manager::Manager() : mapWrapper(new MapWrapper)
{
  setup();
}

void Manager::link(const Tag& tagIn, osg::Geometry *geometryIn)
{
  MapRecord record;
  record.tag = tagIn;
  record.geometry = geometryIn;
  mapWrapper->mapContainer.insert(record);
  
  assert(record.geometry.valid());
}

bool Manager::isLinked(const Tag& tagIn)
{
  typedef MapContainer::index<MapRecord::ByTag>::type List;
  const List &list = mapWrapper->mapContainer.get<MapRecord::ByTag>();
  List::const_iterator it = list.find(tagIn);
  if(it == list.end())
    return false;
  return true;
}

void Manager::setup()
{
  this->link(lbr::csys::TranslationLineTag, lbr::csys::buildTranslationLine());
  this->link(lbr::csys::TranslationCylinderTag, lbr::csys::buildTranslationCylinder());
  this->link(lbr::csys::TranslationConeTag, lbr::csys::buildTranslationCone());
  this->link(lbr::csys::SphereTag, lbr::csys::buildSphere());
  this->link(lbr::csys::RotationLineTag, lbr::csys::buildRotationLine());
  this->link(lbr::csys::RotationTorusTag, lbr::csys::buildRotationTorus());
  this->link(lbr::csys::IconLinkTag, lbr::csys::buildIconLink());
  this->link(lbr::csys::IconUnlinkTag, lbr::csys::buildIconUnlink());
  
  //there seems to be someking of bug with the openscenegraph svg loader.
  //getting artifacts in background. Going to leave for now as we are going
  //to use qt to load icons anyway.
}

osg::Geometry* Manager::getGeometry(const Tag& tagIn)
{
  typedef MapContainer::index<MapRecord::ByTag>::type List;
  List &list = mapWrapper->mapContainer.get<MapRecord::ByTag>();
  List::iterator it = list.find(tagIn);
  assert(it != list.end());
  
  return it->geometry.get();
}

osg::Geometry* lbr::csys::buildTranslationLine()
{
  osg::Geometry *out = new osg::Geometry();
  osg::Vec3Array* vertices = new osg::Vec3Array(2);
  (*vertices)[0] = osg::Vec3(0.0f,0.0f,0.0f);
  (*vertices)[1] = osg::Vec3(0.0f,0.0f,1.0f);
  out->setVertexArray(vertices);
  out->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES,0,2));
  out->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
  
  return out;
}

osg::Geometry* lbr::csys::buildTranslationCylinder()
{
  CylinderBuilder cBuilder;
  cBuilder.setRadius(0.05);
  cBuilder.setHeight(1.0);
  cBuilder.setIsoLines(32);
  osg::Geometry *out = cBuilder;
  
  osgManipulator::setDrawableToAlwaysCull(*out);
  
  return out;
}

osg::Geometry* lbr::csys::buildTranslationCone()
{
  ConeBuilder builder;
  builder.setRadius(0.1);
  builder.setHeight(0.4f);
  builder.setIsoLines(32);
  osg::Geometry *coneGeometry = builder;
  
  //translate all points 1 unit in z.
  osg::Vec3d translation(0.0, 0.0, 1.0);
  osg::Vec3Array *conePoints = dynamic_cast<osg::Vec3Array *>(coneGeometry->getVertexArray());
  assert(conePoints);
  for (auto &point : *conePoints)
    point += translation;
  
  return coneGeometry;
}

osg::Geometry* lbr::csys::buildSphere()
{
  SphereBuilder sBuilder;
  sBuilder.setRadius(0.10);
  sBuilder.setIsoLines(32);
  
  return sBuilder;
}

osg::Geometry* lbr::csys::buildRotationLine()
{
  CircleBuilder cBuilder;
  cBuilder.setSegments(32);
  cBuilder.setRadius(0.75);
  cBuilder.setAngularSpanDegrees(90.0);
  std::vector<osg::Vec3d> circlePoints = cBuilder;
  
  osg::Vec3Array *points = new osg::Vec3Array();
  std::copy(circlePoints.begin(), circlePoints.end(), std::back_inserter(*points));
  
  osg::Geometry *out = new osg::Geometry();
  out->setVertexArray(points);
  out->setUseDisplayList(false);
  out->setUseVertexBufferObjects(true);
  
  osg::PrimitiveSet::Mode mode = (cBuilder.isCompleteCircle()) ? osg::PrimitiveSet::LINE_LOOP :
    osg::PrimitiveSet::LINE_STRIP;

  out->addPrimitiveSet(new osg::DrawArrays(mode, 0, points->size()));
  
  out->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
  
  return out;
}

osg::Geometry* lbr::csys::buildRotationTorus()
{
  TorusBuilder tBuilder;
  tBuilder.setMajorRadius(0.75);
  tBuilder.setMajorIsoLines(32);
  tBuilder.setMinorRadius(0.0375);
  tBuilder.setMinorIsoLines(16);
  tBuilder.setAngularSpanDegrees(90.0);
  
  osg::Geometry *torus = tBuilder;
  osgManipulator::setDrawableToAlwaysCull(*torus);
  
  return torus;
}

osg::Geometry* lbr::csys::buildIconLink()
{
  std::string resourceName = ":/resources/images/linkIcon.svg";
  resourceName = fileNameFromResource(resourceName);
  osg::Image *linkImage = osgDB::readImageFile(resourceName);
  assert(linkImage);
  IconBuilder iBuilder(linkImage);
  return iBuilder;
}

osg::Geometry* lbr::csys::buildIconUnlink()
{
  std::string resourceName = ":/resources/images/unlinkIcon.svg";
  resourceName = fileNameFromResource(resourceName);
  osg::Image *unlinkImage = osgDB::readImageFile(resourceName);
  assert(unlinkImage);
  IconBuilder iBuilder(unlinkImage);
  return iBuilder;
}

std::string lbr::csys::fileNameFromResource(const std::string &resourceName)
{
  QString alteredFileName = QString::fromStdString(resourceName);
  alteredFileName.remove(":");
  alteredFileName.replace("/", "_");
  QDir tempDirectory = QDir::temp();
  QString resourceFileName(tempDirectory.absolutePath() + "/" + alteredFileName);
  if (!tempDirectory.exists(alteredFileName))
  {
    QFile resourceFile(resourceName.c_str());
    if (!resourceFile.open(QIODevice::ReadOnly | QIODevice::Text))
      std::cout << "couldn't resource file" << std::endl;
    QByteArray buffer = resourceFile.readAll();
    resourceFile.close();
     
    QFile newFile(resourceFileName);
    if (!newFile.open(QIODevice::WriteOnly | QIODevice::Text))
      std::cout << "couldn't open new temp file" << std::endl;
    std::cout << "newfilename is: " << resourceFileName.toStdString() << std::endl;
    newFile.write(buffer);
    newFile.close();
  }
  return resourceFileName.toStdString();
}


/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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

#include <QDir>

#include <osg/Geometry>
#include <osg/LineWidth>
#include <osg/BlendFunc>

#include <nodemaskdefs.h>
#include <feature/datumplane.h>

using namespace ftr;

QIcon DatumPlane::icon;

DatumPlane::DatumPlane() : Base()
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionDatumPlane.svg");
  
  name = QObject::tr("Datum Plane");
  
  geometry = new osg::Geometry();
  geometry->setNodeMask(NodeMaskDef::datum);
  geometry->setName("datumPlane");
  geometry->getOrCreateStateSet()->setAttribute(new osg::LineWidth(4.0f));
  geometry->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
  
  osg::BlendFunc* bf = new osg::BlendFunc(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_SRC_ALPHA); 
  geometry->getOrCreateStateSet()->setAttributeAndModes(bf);
  
  osg::Vec3Array *vertices = new osg::Vec3Array();
  vertices->push_back(osg::Vec3d(1.0, -1.0, 0.0));
  vertices->push_back(osg::Vec3d(1.0, 1.0, 0.0));
  vertices->push_back(osg::Vec3d(-1.0, 1.0, 0.0));
  vertices->push_back(osg::Vec3d(-1.0, -1.0, 0.0));
  vertices->push_back(osg::Vec3d(1.0, -1.0, 0.0));
  vertices->push_back(osg::Vec3d(1.0, 1.0, 0.0));
  vertices->push_back(osg::Vec3d(-1.0, 1.0, 0.0));
  vertices->push_back(osg::Vec3d(-1.0, -1.0, 0.0));
  geometry->setVertexArray(vertices);
  geometry->setUseDisplayList(false);
  geometry->setUseVertexBufferObjects(true);
  geometry->setDataVariance(osg::Geometry::DYNAMIC);

  osg::Vec4Array *colors = new osg::Vec4Array();
  colors->push_back(osg::Vec4(.4f, .7f, .75f, .5f));
  colors->push_back(osg::Vec4(.4f, .7f, .75f, .5f));
  colors->push_back(osg::Vec4(.4f, .7f, .75f, .5f));
  colors->push_back(osg::Vec4(.4f, .7f, .75f, .5f));
  colors->push_back(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
  colors->push_back(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
  colors->push_back(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
  colors->push_back(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
  geometry->setColorArray(colors);
  geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

  osg::Vec3Array *normals = new osg::Vec3Array();
  normals->push_back(osg::Vec3d(0.0, 0.0, 1.0));
  normals->push_back(osg::Vec3d(0.0, 0.0, 1.0));
  normals->push_back(osg::Vec3d(0.0, 0.0, 1.0));
  normals->push_back(osg::Vec3d(0.0, 0.0, 1.0));
  normals->push_back(osg::Vec3d(0.0, 0.0, 1.0)); //unused but keep the same count.
  normals->push_back(osg::Vec3d(0.0, 0.0, 1.0));
  normals->push_back(osg::Vec3d(0.0, 0.0, 1.0));
  normals->push_back(osg::Vec3d(0.0, 0.0, 1.0));
  geometry->setNormalArray(normals);
  geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
  
  osg::DrawElementsUInt *triangles = new osg::DrawElementsUInt(GL_TRIANGLES, 6);
  (*triangles)[0] = 0;
  (*triangles)[1] = 1;
  (*triangles)[2] = 2;
  (*triangles)[3] = 2;
  (*triangles)[4] = 3;
  (*triangles)[5] = 0;
  geometry->addPrimitiveSet(triangles);
  
  osg::DrawArrays *lines = new osg::DrawArrays(GL_LINE_LOOP, 4, 4);
  geometry->addPrimitiveSet(lines);
  
  updateGeometry();
  overlaySwitch->addChild(geometry.get());
}

DatumPlane::~DatumPlane()
{

}

void DatumPlane::updateModel(const UpdateMap &mapIn)
{
  //temp for visual work.
  updateGeometry();
  setModelClean();
}

void DatumPlane::updateVisual()
{
  setVisualClean();
}

void DatumPlane::serialWrite(const QDir &directoryIn)
{
//   ftr::Base::serialWrite(directoryIn);
}

void DatumPlane::updateGeometry()
{
  //temp values for display testing.
  
  
  geometry->dirtyDisplayList();
}

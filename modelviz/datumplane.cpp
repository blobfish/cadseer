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

#include <cassert>

#include <osg/LineWidth>
#include <osg/BlendFunc>

#include <modelviz/nodemaskdefs.h>
#include <modelviz/datumplane.h>

using namespace mdv;

DatumPlane::DatumPlane() : Base()
{
  setUseDisplayList(false);
  setDataVariance(osg::Object::DYNAMIC);
  setUseVertexBufferObjects(true);
  
  setNodeMask(mdv::datum);
  setName("datumPlane");
  getOrCreateStateSet()->setAttribute(new osg::LineWidth(4.0f));
  getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
  
  osg::BlendFunc* bf = new osg::BlendFunc(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_SRC_ALPHA); 
  getOrCreateStateSet()->setAttributeAndModes(bf);
  
  osg::Vec3Array *vertices = new osg::Vec3Array();
  vertices->push_back(osg::Vec3d(1.0, -1.0, 0.0));
  vertices->push_back(osg::Vec3d(1.0, 1.0, 0.0));
  vertices->push_back(osg::Vec3d(-1.0, 1.0, 0.0));
  vertices->push_back(osg::Vec3d(-1.0, -1.0, 0.0));
  vertices->push_back(osg::Vec3d(1.0, -1.0, 0.0));
  vertices->push_back(osg::Vec3d(1.0, 1.0, 0.0));
  vertices->push_back(osg::Vec3d(-1.0, 1.0, 0.0));
  vertices->push_back(osg::Vec3d(-1.0, -1.0, 0.0));
  setVertexArray(vertices);

  color = osg::Vec4(.4f, .7f, .75f, .5f);
  
  osg::Vec4Array *colors = new osg::Vec4Array();
  colors->push_back(color);
  colors->push_back(color);
  colors->push_back(color);
  colors->push_back(color);
  colors->push_back(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
  colors->push_back(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
  colors->push_back(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
  colors->push_back(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
  setColorArray(colors);
  setColorBinding(osg::Geometry::BIND_PER_VERTEX);

  osg::Vec3Array *normals = new osg::Vec3Array();
  normals->push_back(osg::Vec3d(0.0, 0.0, 1.0));
  normals->push_back(osg::Vec3d(0.0, 0.0, 1.0));
  normals->push_back(osg::Vec3d(0.0, 0.0, 1.0));
  normals->push_back(osg::Vec3d(0.0, 0.0, 1.0));
  normals->push_back(osg::Vec3d(0.0, 0.0, 1.0)); //unused but keep the same count.
  normals->push_back(osg::Vec3d(0.0, 0.0, 1.0));
  normals->push_back(osg::Vec3d(0.0, 0.0, 1.0));
  normals->push_back(osg::Vec3d(0.0, 0.0, 1.0));
  setNormalArray(normals);
  setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
  
  osg::DrawElementsUInt *triangles = new osg::DrawElementsUInt(GL_TRIANGLES, 6);
  (*triangles)[0] = 0;
  (*triangles)[1] = 1;
  (*triangles)[2] = 2;
  (*triangles)[3] = 2;
  (*triangles)[4] = 3;
  (*triangles)[5] = 0;
  addPrimitiveSet(triangles);
  
  osg::DrawArrays *lines = new osg::DrawArrays(GL_LINE_LOOP, 4, 4);
  addPrimitiveSet(lines);
}

DatumPlane::DatumPlane(const DatumPlane &rhs, const osg::CopyOp& copyOperation) :
  Base(rhs, copyOperation)
{
  setUseDisplayList(false);
  setDataVariance(osg::Object::DYNAMIC);
  setUseVertexBufferObjects(true);
}

void DatumPlane::setFaceColor(const osg::Vec4 &colorIn)
{
  //create a temp color with transparency.
  osg::Vec4 tempColor = colorIn;
  tempColor.w() = 0.5f;
  
  osg::Vec4Array *colors = dynamic_cast<osg::Vec4Array*>(getColorArray());
  assert(colors);
  (*colors)[0] = tempColor;
  (*colors)[1] = tempColor;
  (*colors)[2] = tempColor;
  (*colors)[3] = tempColor;
  
  _colorArray->dirty();
}

void DatumPlane::setToColor()
{
  setFaceColor(color);
}

void DatumPlane::setToPreHighlight()
{
  setFaceColor(colorPreHighlight);
}

void DatumPlane::setToHighlight()
{
  setFaceColor(colorHighlight);
}

void DatumPlane::setParameters(double xmin, double xmax, double ymin, double ymax)
{
  osg::Vec3Array *vertices = dynamic_cast<osg::Vec3Array *>(getVertexArray());
  (*vertices)[0] = (osg::Vec3d(xmax, ymin, 0.0));
  (*vertices)[1] = (osg::Vec3d(xmax, ymax, 0.0));
  (*vertices)[2] = (osg::Vec3d(xmin, ymax, 0.0));
  (*vertices)[3] = (osg::Vec3d(xmin, ymin, 0.0));
  (*vertices)[4] = (osg::Vec3d(xmax, ymin, 0.0));
  (*vertices)[5] = (osg::Vec3d(xmax, ymax, 0.0));
  (*vertices)[6] = (osg::Vec3d(xmin, ymax, 0.0));
  (*vertices)[7] = (osg::Vec3d(xmin, ymin, 0.0));
  
  _vertexArray->dirty();
  dirtyBound();
}

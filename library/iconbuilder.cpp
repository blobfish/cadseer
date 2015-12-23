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

#include <osg/Geometry>
#include <osg/Texture2D>

#include "iconbuilder.h"

using namespace lbr;

IconBuilder::IconBuilder(osg::Image *imageIn) : image(imageIn)
{

}

IconBuilder::operator osg::Geometry* () const
{
  assert(image.valid());
  
  // set up the Geometry.
  osg::ref_ptr<osg::Geometry> out = new osg::Geometry();

  osg::Vec3Array* coordinates = new osg::Vec3Array(4);
  (*coordinates)[0] = lowerLeftCorner;
  (*coordinates)[1] = lowerLeftCorner + xVector;
  (*coordinates)[2] = lowerLeftCorner + xVector + yVector;
  (*coordinates)[3] = lowerLeftCorner + yVector;
  out->setVertexArray(coordinates);

  osg::Vec3Array* normals = new osg::Vec3Array(1);
  (*normals)[0] = xVector ^ yVector;
  (*normals)[0].normalize();
  out->setNormalArray(normals, osg::Array::BIND_OVERALL);

  osg::Vec2Array* textureCoordinates = new osg::Vec2Array(4);
  (*textureCoordinates)[0].set(0.0f, 0.0f);
  (*textureCoordinates)[1].set(1.0f, 0.0f);
  (*textureCoordinates)[2].set(1.0f, 1.0f);
  (*textureCoordinates)[3].set(0.0f, 1.0f);
  out->setTexCoordArray(0, textureCoordinates);

  out->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 4));

  osg::Texture2D* texture = new osg::Texture2D();
  texture->setImage(image.get());
  out->getOrCreateStateSet()->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
  
  out->getOrCreateStateSet()->setMode(GL_LIGHTING,osg::StateAttribute::OFF);
//   out->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
//   out->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

  return out.release();
}
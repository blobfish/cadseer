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

#include <cassert>

#include <osg/AutoTransform>

#include <library/geometrylibrary.h>
#include <library/lineardragger.h>

using namespace lbr;

LinearDragger::LinearDragger() : Translate1DDragger(osg::Vec3d(0.0,0.0,0.0), osg::Vec3d(0.0,0.0,1.0))
{
  setHandleEvents(false);
  
  autoScaleTransform = new osg::AutoTransform();
  autoScaleTransform->setAutoScaleToScreen(true);
  autoScaleTransform->setPosition(osg::Vec3d(0.0, 0.0, 0.0));
  addChild(autoScaleTransform);
  
  scaleTransform = new osg::MatrixTransform();
  autoScaleTransform->addChild(scaleTransform);
  setScreenScale(200.0);
  
  assert(lbr::Manager::getManager().isLinked(lbr::csys::TranslationConeTag));
  assert(lbr::Manager::getManager().isLinked(lbr::csys::TranslationLineTag));
  
  scaleTransform->addChild(lbr::Manager::getManager().getGeometry(lbr::csys::TranslationConeTag));
  scaleTransform->addChild(lbr::Manager::getManager().getGeometry(lbr::csys::TranslationLineTag));
  
  incrementConstraint = new osgManipulator::GridConstraint(*this, osg::Vec3d(0.0, 0.0, 0.0), osg::Vec3d(0.5, 0.5, 0.5));
  this->addConstraint(incrementConstraint.get());
  
  this->getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);
  this->setCullingActive(false);
}

void LinearDragger::setScreenScale(double scaleIn)
{
  if (screenScale == scaleIn)
    return;
  screenScale = scaleIn;
  scaleTransform->setMatrix(osg::Matrixd::scale(osg::Vec3d(screenScale, screenScale, screenScale)));
}

void LinearDragger::setIncrement(double incrementIn)
{
  incrementConstraint->setSpacing(osg::Vec3d(incrementIn, incrementIn, incrementIn));
}

double LinearDragger::getIncrement() const
{
  return incrementConstraint->getSpacing().z();
}


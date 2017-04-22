/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#include <osg/LineStipple>
#include <osg/Depth>

#include "hiddenlinetechnique.h"

using namespace mdv;

static osg::StateSet* buildSolidLineState()
{
  osg::StateSet *pass = new osg::StateSet();
  osg::Depth *edgeDepth = new osg::Depth();
  edgeDepth->setRange(0.001, 1.001);
  pass->setAttribute(edgeDepth);
  
  return pass;
}

HiddenLineTechnique::HiddenLineTechnique() : osgFX::Technique()
{
}

void HiddenLineTechnique::define_passes()
{
  {
    osg::StateSet *pass = buildSolidLineState();
    addPass(pass);
  }
  
  {
    osg::StateSet *pass = new osg::StateSet();
    osg::LineStipple *stipple = new osg::LineStipple(2, 0xf0f0);
    pass->setAttributeAndModes(stipple);
    osg::Depth *depth = new osg::Depth();
    depth->setFunction(osg::Depth::GREATER);
    depth->setRange(-0.01, 0.99);
    depth->setWriteMask(false);
    pass->setAttribute(depth);
    addPass(pass);
  }
}

NoHiddenLineTechnique::NoHiddenLineTechnique()
{
}

void NoHiddenLineTechnique::define_passes()
{
  osg::StateSet *pass = buildSolidLineState();
  addPass(pass);
}

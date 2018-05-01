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

#include <osgDB/ObjectWrapper>
#include <osgDB/Registry>
#include <osgFX/Registry>

#include "hiddenlinetechnique.h"
#include "hiddenlineeffect.h"

using namespace osgFX;
using namespace mdv;

Registry::Proxy proxy(new HiddenLineEffect());

REGISTER_OBJECT_WRAPPER( mdv_HiddenLinEffect_Wrapper,
                         new mdv::HiddenLineEffect,
                         mdv::HiddenLineEffect,
                         "osg::Object osg::Node osg::Group osgFX::Effect mdv::HiddenLineEffect" )
{
  ADD_BOOL_SERIALIZER(HiddenLine, false);
}

HiddenLineEffect::HiddenLineEffect() : Effect()
{
}

HiddenLineEffect::HiddenLineEffect(const HiddenLineEffect& copy, const osg::CopyOp& op) : Effect(copy, op)
{
  hiddenLine = copy.hiddenLine;
  selectTechnique(copy.getSelectedTechnique());
}

HiddenLineEffect::~HiddenLineEffect() {}

void HiddenLineEffect::setHiddenLine(bool freshValue)
{
  hiddenLine = freshValue;
  updateHiddenLine();
}

void HiddenLineEffect::updateHiddenLine()
{
  if (hiddenLine)
    selectTechnique(1);
  else
    selectTechnique(0);
}

bool HiddenLineEffect::define_techniques()
{
  addTechnique(new NoHiddenLineTechnique());
  addTechnique(new HiddenLineTechnique());
  return true;
}

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

#ifndef MDV_HIDDENLINEEFFECT_H
#define MDV_HIDDENLINEEFFECT_H

#include <osgFX/Effect>

namespace mdv
{
  class HiddenLineEffect : public osgFX::Effect
  {
  public:
    HiddenLineEffect();
    HiddenLineEffect(const HiddenLineEffect& copy, const osg::CopyOp& op = osg::CopyOp::SHALLOW_COPY);
    
    META_Effect
    (
      osgFX,
      HiddenLineEffect,
      "HiddenLineEffect",
      "Draw hidden lines",
      "Thomas Anderson"
    );
    
    void setHiddenLine(bool); //true equal hidden lines are shown.
    
  protected:
    virtual ~HiddenLineEffect() override;
    virtual bool define_techniques() override;
  };
}

#endif // MDV_HIDDENLINEEFFECT_H

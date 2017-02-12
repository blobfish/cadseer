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

#ifndef MDV_HIDDENLINETECHNIQUE_H
#define MDV_HIDDENLINETECHNIQUE_H

#include <osgFX/Technique>

namespace mdv
{
  class HiddenLineTechnique : public osgFX::Technique
  {
  public:
    HiddenLineTechnique();
  protected:
    virtual void define_passes() override;
  };
  
  class NoHiddenLineTechnique : public osgFX::Technique
  {
  public:
    NoHiddenLineTechnique();
  protected:
    virtual void define_passes() override;
  };
}

#endif // MDV_HIDDENLINETECHNIQUE_H

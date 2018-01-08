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

#ifndef CMD_MEASURELINEAR_H
#define CMD_MEASURELINEAR_H

#include <selection/definitions.h>
#include <command/base.h>

namespace cmd
{
  class MeasureLinear :  public Base
  {
  public:
    MeasureLinear();
    virtual ~MeasureLinear() override;
    
    virtual std::string getCommandName() override{return "Measure Linear";}
    virtual std::string getStatusMessage() override;
    virtual void activate() override;
    virtual void deactivate() override;
  private:
    void setupDispatcher();
    void selectionAdditionDispatched(const msg::Message&);
    void selectionMaskDispatched(const msg::Message&);
    void go();
    void build(const osg::Vec3d&, const osg::Vec3d&);
    slc::Mask selectionMask;
  };
}

#endif // CMD_MEASURELINEAR_H

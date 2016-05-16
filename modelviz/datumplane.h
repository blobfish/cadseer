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

#ifndef MDV_DATUMPLANE_H
#define MDV_DATUMPLANE_H

#include <modelviz/base.h>

namespace mdv
{
  class DatumPlane : public Base
  {
  public:
    DatumPlane();
    DatumPlane(const DatumPlane &rhs, const osg::CopyOp& copyOperation = osg::CopyOp::SHALLOW_COPY);

    virtual osg::Object* cloneType() const override {return new DatumPlane();}
    virtual osg::Object* clone(const osg::CopyOp& copyOperation) const override {return new DatumPlane(*this, copyOperation);}
    virtual bool isSameKindAs(const osg::Object* obj) const override {return dynamic_cast<const DatumPlane*>(obj)!=NULL;}
    virtual const char* libraryName() const override {return "osg";}
    virtual const char* className() const override {return "Geometry";}
    
    virtual void setToColor() override;
    virtual void setToPreHighlight() override;
    virtual void setToHighlight() override;
    
    void setParameters(double, double, double, double);
    
  protected:
    void setFaceColor(const osg::Vec4&);
    
  };
}

#endif // MDV_DATUMPLANE_H

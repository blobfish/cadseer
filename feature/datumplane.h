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

#ifndef FTR_DATUMPLANE_H
#define FTR_DATUMPLANE_H

#include <feature/base.h>

class QDir;
namespace osg{class MatrixTransform;}
namespace mdv{class DatumPlane;}

namespace ftr
{
  class DatumPlane : public Base
  {
  public:
    DatumPlane();
    ~DatumPlane();
    
    virtual void updateModel(const UpdateMap&) override;
    virtual void updateVisual() override;
    virtual Type getType() const override {return Type::DatumPlane;}
    virtual const std::string& getTypeString() const override {return toString(Type::DatumPlane);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
    virtual void serialWrite(const QDir&) override;
//     void serialRead(const prj::srl::FeatureDraft &);
    
  private:
    static QIcon icon;
    osg::ref_ptr<mdv::DatumPlane> display;
    osg::ref_ptr<osg::MatrixTransform> transform;
    
    void updateGeometry();
  };
}

#endif // FTR_DATUMPLANE_H

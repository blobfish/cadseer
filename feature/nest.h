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

#ifndef FTR_NEST_H
#define FTR_NEST_H

#include <memory>

#include <osg/ref_ptr>
#include <osg/Vec3d>

#include <feature/base.h>

namespace lbr{class PLabel;}

namespace ftr
{
  class Nest : public Base
  {
  public:
    constexpr static const char *blank = "Blank";
    
    Nest();
    virtual ~Nest() override;
    
    virtual void updateModel(const UpdatePayload&) override;
    virtual Type getType() const override {return Type::Nest;}
    virtual const std::string& getTypeString() const override {return toString(Type::Nest);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
    virtual void serialWrite(const QDir&) override;
//     void serialRead(const prj::srl::FeatureNest &);
    
    double getPitch() const;
    double getGap() const;
    
  protected:
    std::shared_ptr<Parameter> pitch; //!< not really a parameter. just using for convenience.
    std::shared_ptr<Parameter> gap;
    osg::ref_ptr<lbr::PLabel> gapLabel;
    osg::Vec3d feedDirection; //!< eventually a parameter.
    
    TopoDS_Shape calcPitch(TopoDS_Shape &bIn, double guess);
    double getDistance(const TopoDS_Shape &sIn1, const TopoDS_Shape &sIn2);
    
  private:
    static QIcon icon;
  };
}

#endif // FTR_NEST_H

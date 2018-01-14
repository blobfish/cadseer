/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2018  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef FTR_INSTANCELINEAR_H
#define FTR_INSTANCELINEAR_H

#include <osg/ref_ptr>

#include <tools/occtools.h>
#include <feature/pick.h>
#include <feature/base.h>

namespace osg{class Node;}

namespace ann{class Base; class SeerShape; class InstanceMapper; class CSysDragger;}
namespace lbr{class PLabel;}

namespace ftr
{
  /**
  * @brief Feature for linear patterns
  * 
  */
  class InstanceLinear : public Base
  {
  public:
    
    InstanceLinear();
    virtual ~InstanceLinear() override;
    
    virtual void updateModel(const UpdatePayload&) override;
    virtual Type getType() const override {return Type::InstanceLinear;}
    virtual const std::string& getTypeString() const override {return toString(Type::InstanceLinear);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
    
    virtual void serialWrite(const QDir&) override;
//     void serialRead(const prj::srl::FeatureRefine &);
    
    const Pick& getPick(){return pick;}
    void setPick(const Pick&);
    
  protected:
    std::unique_ptr<ann::SeerShape> sShape;
    std::unique_ptr<ann::InstanceMapper> iMapper;
    std::unique_ptr<ann::CSysDragger> csysDragger;
    
    prm::Parameter xOffset;
    prm::Parameter yOffset;
    prm::Parameter zOffset;
    
    prm::Parameter xCount;
    prm::Parameter yCount;
    prm::Parameter zCount;
    
    prm::Parameter csys;
    
    osg::ref_ptr<lbr::PLabel> xOffsetLabel;
    osg::ref_ptr<lbr::PLabel> yOffsetLabel;
    osg::ref_ptr<lbr::PLabel> zOffsetLabel;
    
    osg::ref_ptr<lbr::PLabel> xCountLabel;
    osg::ref_ptr<lbr::PLabel> yCountLabel;
    osg::ref_ptr<lbr::PLabel> zCountLabel;
    
    
    Pick pick;
    
  private:
    static QIcon icon;
  };
}

#endif // FTR_INSTANCELINEAR_H

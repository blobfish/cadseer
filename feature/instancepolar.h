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

#ifndef FTR_INSTANCEPOLAR_H
#define FTR_INSTANCEPOLAR_H

#include <osg/ref_ptr>

#include <feature/pick.h>
#include <feature/base.h>

namespace ann{class SeerShape; class InstanceMapper; class CSysDragger;}
namespace lbr{class PLabel;}
namespace prj{namespace srl{class FeatureInstancePolar;}}

namespace ftr
{
  /**
  * @brief feature for polar patterns.
  */
  class InstancePolar : public Base
  {
  public:
    constexpr static const char *rotationAxis = "rotationAxis";
    
    InstancePolar();
    virtual ~InstancePolar() override;
    
    virtual void updateModel(const UpdatePayload&) override;
    virtual Type getType() const override {return Type::InstancePolar;}
    virtual const std::string& getTypeString() const override {return toString(Type::InstancePolar);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
    
    virtual void serialWrite(const QDir&) override;
    void serialRead(const prj::srl::FeatureInstancePolar&);
    
    const Pick& getShapePick(){return shapePick;}
    void setShapePick(const Pick&);
    const Pick& getAxisPick(){return axisPick;}
    void setAxisPick(const Pick&);
    void setCSys(const osg::Matrixd&);
    
  protected:
    std::unique_ptr<prm::Parameter> csys;
    std::unique_ptr<prm::Parameter> count;
    std::unique_ptr<prm::Parameter> angle;
    std::unique_ptr<prm::Parameter> inclusiveAngle;
    std::unique_ptr<prm::Parameter> includeSource;
    
    std::unique_ptr<ann::SeerShape> sShape;
    std::unique_ptr<ann::InstanceMapper> iMapper;
    std::unique_ptr<ann::CSysDragger> csysDragger;
    
    osg::ref_ptr<lbr::PLabel> countLabel;
    osg::ref_ptr<lbr::PLabel> angleLabel;
    osg::ref_ptr<lbr::PLabel> inclusiveAngleLabel;
    osg::ref_ptr<lbr::PLabel> includeSourceLabel;
    
    Pick shapePick;
    Pick axisPick;
    
  private:
    static QIcon icon;
  };
}

#endif // FTR_INSTANCEPOLAR_H

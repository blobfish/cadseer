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

#ifndef FTR_INSTANCEMIRROR_H
#define FTR_INSTANCEMIRROR_H

#include <feature/pick.h>
#include <feature/base.h>

namespace ann{class SeerShape; class InstanceMapper; class CSysDragger;}

namespace ftr
{
  /**
  * @brief Feature for mirrored shapes.
  */
  class InstanceMirror : public Base
  {
  public:
    constexpr static const char *mirrorPlane = "MirrorPlane";
    
    InstanceMirror();
    virtual ~InstanceMirror() override;
    
    virtual void updateModel(const UpdatePayload&) override;
    virtual Type getType() const override {return Type::InstanceMirror;}
    virtual const std::string& getTypeString() const override {return toString(Type::InstanceMirror);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
    
    virtual void serialWrite(const QDir&) override;
//     void serialRead(const prj::srl::FeatureRefine &);
    
    const Pick& getShapePick(){return shapePick;}
    void setShapePick(const Pick&);
    const Pick& getPlanePick(){return planePick;}
    void setPlanePick(const Pick&);
    void setCSys(const osg::Matrixd&); //when no plane pick
    
  protected:
    std::unique_ptr<ann::SeerShape> sShape;
    std::unique_ptr<ann::InstanceMapper> iMapper;
    std::unique_ptr<ann::CSysDragger> csysDragger;
    prm::Parameter csys;
    
    Pick shapePick;
    Pick planePick;

  private:
    static QIcon icon;
  };
}

#endif // FTR_INSTANCEMIRROR_H

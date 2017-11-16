/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2015  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef FTR_INERT_H
#define FTR_INERT_H

#include <feature/base.h>

namespace prj{namespace srl{class FeatureInert;}}
namespace ann{class CSysDragger;}

namespace ftr
{
  /*! @brief static feature.
   * 
   * feature that has no real parameters or update.
   * for example, used for import geometry.
   */
  class Inert : public Base
  {
  public:
    Inert() = delete;
    Inert(const TopoDS_Shape &shapeIn);
    virtual ~Inert() override;
    virtual void updateModel(const UpdatePayload&) override;
    virtual Type getType() const override {return Type::Inert;}
    virtual const std::string& getTypeString() const override {return toString(Type::Inert);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
    virtual void serialWrite(const QDir&) override;
    void serialRead(const prj::srl::FeatureInert &sBox);
    
    void setCSys(const osg::Matrixd&);
    osg::Matrixd getCSys() const {return static_cast<osg::Matrixd>(csys);}
    
  protected:
    prm::Parameter csys;
    std::unique_ptr<ann::CSysDragger> csysDragger;
    std::unique_ptr<ann::SeerShape> sShape;
    
  private:
    static QIcon icon;
  };
}

#endif // FTR_INERT_H

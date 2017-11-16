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

#ifndef SPHERE_H
#define SPHERE_H

#include <osg/ref_ptr>

#include <feature/base.h>

class BRepPrimAPI_MakeSphere;
namespace lbr{class IPGroup;}
namespace prj{namespace srl{class FeatureSphere;}}
namespace ann{class CSysDragger; class SeerShape;}

namespace ftr
{
  class Sphere : public Base
  {
  public:
    Sphere();
    virtual ~Sphere() override;
    void setRadius(const double &radiusIn);
    void setCSys(const osg::Matrixd&);
    double getRadius() const {return static_cast<double>(radius);}
    osg::Matrixd getCSys() const {return static_cast<osg::Matrixd>(csys);}
    
    virtual void updateModel(const UpdatePayload&) override;
    virtual Type getType() const override {return Type::Sphere;}
    virtual const std::string& getTypeString() const override {return toString(Type::Sphere);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
    virtual void serialWrite(const QDir&) override; //!< write xml file. not const, might reset a modified flag.
    void serialRead(const prj::srl::FeatureSphere &sSphere); //!<initializes this from sBox. not virtual, type already known.
    
  protected:
    prm::Parameter radius;
    prm::Parameter csys;
  
    std::unique_ptr<ann::CSysDragger> csysDragger;
    std::unique_ptr<ann::SeerShape> sShape;
    
    osg::ref_ptr<lbr::IPGroup> radiusIP;
    
    void initializeMaps();
    void updateResult(BRepPrimAPI_MakeSphere&);
    void setupIPGroup();
    void updateIPGroup();
    
  private:
    static QIcon icon;
  };
}

#endif // SPHERE_H

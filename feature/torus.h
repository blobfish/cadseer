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

#ifndef FTR_TORUS_H
#define FTR_TORUS_H

#include <feature/base.h>

namespace prj{namespace srl{class FeatureTorus;}}
namespace ann{class CSysDragger; class SeerShape;}
namespace lbr{class IPGroup;}

namespace ftr
{
  namespace prm{class Parameter;}
  
  /**
  * @brief build a torus solid primitive
  */
  class Torus : public Base
  {
  public:
    Torus();
    virtual ~Torus() override;
    
    void setRadius1(const double &lengthIn);
    void setRadius2(const double &lengthIn);
    void setCSys(const osg::Matrixd&);
    
    double getRadius1() const;
    double getRadius2() const;
    osg::Matrixd getCSys() const;
    
    virtual void serialWrite(const QDir&) override;
    void serialRead(const prj::srl::FeatureTorus&);
    
    virtual void updateModel(const UpdatePayload&) override;
    virtual Type getType() const override {return Type::Torus;}
    virtual const std::string& getTypeString() const override {return toString(Type::Torus);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
    
  protected:
    std::unique_ptr<prm::Parameter> radius1;
    std::unique_ptr<prm::Parameter> radius2; //<! has to be smaller than radius 1.
    std::unique_ptr<prm::Parameter> csys;
    
    osg::ref_ptr<lbr::IPGroup> radius1IP;
    osg::ref_ptr<lbr::IPGroup> radius2IP;
  
    std::unique_ptr<ann::CSysDragger> csysDragger;
    std::unique_ptr<ann::SeerShape> sShape;
    
    std::vector<boost::uuids::uuid> offsetIds;
    
    void initializeMaps();
    void setupIPGroup();
    void updateIPGroup();
    void updateResult(); //expects seershape to be set.
    
  private:
    static QIcon icon;
  };
}

#endif // FTR_TORUS_H

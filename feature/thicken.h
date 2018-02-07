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

#ifndef FTR_THICKEN_H
#define FTR_THICKEN_H

#include <osg/ref_ptr>

#include <feature/base.h>

class BRepOffset_MakeOffset;

namespace lbr{class PLabel;}
namespace ann{class SeerShape;}
namespace prj{namespace srl{class FeatureThicken;}}

namespace ftr
{

  /**
  * @todo write docs
  */
  class Thicken : public Base
  {
  public:
    Thicken();
    virtual ~Thicken() override;
    
    virtual void updateModel(const UpdatePayload&) override;
    virtual Type getType() const override {return Type::Thicken;}
    virtual const std::string& getTypeString() const override {return toString(Type::Thicken);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
    
    virtual void serialWrite(const QDir&) override;
    void serialRead(const prj::srl::FeatureThicken&);
    
  protected:
    std::unique_ptr<prm::Parameter> distance;
    
    std::unique_ptr<ann::SeerShape> sShape;
    
    osg::ref_ptr<lbr::PLabel> distanceLabel;
    
    void thickenMatch(const BRepOffset_MakeOffset&);
    std::map<boost::uuids::uuid, boost::uuids::uuid> faceMap; //map face from target to offset face.
    std::map<boost::uuids::uuid, boost::uuids::uuid> edgeMap; //map edge from target to offset edge.
    std::map<boost::uuids::uuid, boost::uuids::uuid> boundaryMap; //map edge from target to perimeter face.
    std::map<boost::uuids::uuid, boost::uuids::uuid> oWireMap; //map new face to outer wire.
    
    boost::uuids::uuid solidId;
    boost::uuids::uuid shellId;
    
  private:
    static QIcon icon;
  };
}

#endif // FTR_THICKEN_H

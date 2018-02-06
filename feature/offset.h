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

#ifndef FTR_OFFSET_H
#define FTR_OFFSET_H

#include <osg/ref_ptr>

#include <feature/pick.h>
#include <feature/base.h>

class BRepOffset_MakeOffset;

namespace lbr{class PLabel;}
namespace ann{class SeerShape;}
namespace prj{namespace srl{class FeatureOffset;}}

namespace ftr
{
  /**
  * @todo write docs
  */
  class Offset : public Base
  {
  public:
    Offset();
    virtual ~Offset() override;
    
    virtual void updateModel(const UpdatePayload&) override;
    virtual Type getType() const override {return Type::Offset;}
    virtual const std::string& getTypeString() const override {return toString(Type::Offset);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Alter;}
    
    virtual void serialWrite(const QDir&) override;
    void serialRead(const prj::srl::FeatureOffset &);
    
    void setPicks(const Picks&);
    Picks getPicks(){return picks;}
    
  protected:
    std::unique_ptr<prm::Parameter> distance;
    
    std::unique_ptr<ann::SeerShape> sShape;
    
    osg::ref_ptr<lbr::PLabel> distanceLabel;
    
    Picks picks;
    
    void offsetMatch(const BRepOffset_MakeOffset&, const ann::SeerShape&);
  private:
    static QIcon icon;
  };
}

#endif // FTR_OFFSET_H

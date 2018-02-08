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

#ifndef FTR_SEW_H
#define FTR_SEW_H

#include <feature/base.h>

class BRepBuilderAPI_Sewing;

namespace ann{class SeerShape;}
namespace prj{namespace srl{class FeatureSew;}}

namespace ftr
{
  /**
  * @todo write docs
  */
  class Sew : public Base
  {
  public:
    Sew();
    virtual ~Sew() override;
    
    virtual void updateModel(const UpdatePayload&) override;
    virtual Type getType() const override {return Type::Sew;}
    virtual const std::string& getTypeString() const override {return toString(Type::Sew);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
    
    virtual void serialWrite(const QDir&) override;
    void serialRead(const prj::srl::FeatureSew&);
    
  protected:
    std::unique_ptr<ann::SeerShape> sShape;
    
    void assignSolidShell();
    void sewModifiedMatch(const BRepBuilderAPI_Sewing&, const ann::SeerShape&);
    boost::uuids::uuid solidId;
    boost::uuids::uuid shellId;
    
  private:
    static QIcon icon;
  };
}

#endif // FTR_SEW_H

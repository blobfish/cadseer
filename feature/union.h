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

#ifndef FTR_UNION_H
#define FTR_UNION_H

#include <feature/booleanbase.h>

class QDir;

namespace prj{namespace srl{class FeatureUnion;}}
namespace ann{class SeerShape;}
namespace ftr
{
  class Union : public BooleanBase
  {
  public:
    Union();
    virtual ~Union() override;
    
    virtual void updateModel(const UpdatePayload&) override;
    virtual Type getType() const override {return Type::Union;}
    virtual const std::string& getTypeString() const override {return toString(Type::Union);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Alter;}
    virtual void serialWrite(const QDir&) override; //!< write xml file. not const, might reset a modified flag.
    void serialRead(const prj::srl::FeatureUnion &sUnionIn); //!<initializes this from sBox. not virtual, type already known.
    
  protected:
    std::unique_ptr<ann::SeerShape> sShape;
    
  private:
    static QIcon icon;
  };
}

#endif // FTR_UNION_H

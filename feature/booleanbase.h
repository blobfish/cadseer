/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef BOOLEANBASE_H
#define BOOLEANBASE_H

#include <feature/intersectionmapping.h>
#include <feature/base.h>

namespace prj{namespace srl{class FeatureBooleanBase;}}

namespace ftr
{
  class BooleanBase : public Base
  {
  public:
    BooleanBase();
    virtual Type getType() const override {return Type::Boolean;}
    virtual const std::string& getTypeString() const override {return toString(Type::Boolean);}
    virtual const QIcon& getIcon() const override {static QIcon junk; return junk;}
    virtual Descriptor getDescriptor() const override {return Descriptor::None;}
//     virtual void updateModel(const UpdateMap&) override;

  protected:
    prj::srl::FeatureBooleanBase serialOut(); //!<convert this into serializable object.
    void serialIn(const prj::srl::FeatureBooleanBase &sBooleanBaseIn); //intialize this from serial object.
    IMapWrapper iMapWrapper;
  };
}

#endif // BOOLEANBASE_H

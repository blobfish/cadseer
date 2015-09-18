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

#ifndef CSYSBASE_H
#define CSYSBASE_H

#include <gp_Ax2.hxx>

#include "base.h"

namespace Feature
{
  /*! Base class for features dependent on a coordinate system*/
  class CSysBase : public Base
  {
  public:
    CSysBase();
    virtual Type getType() const override {return Type::CSys;}
    virtual const std::string& getTypeString() const override {return Feature::getTypeString(Type::CSys);}
    virtual const QIcon& getIcon() const override {static QIcon junk; return junk;}
    virtual Descriptor getDescriptor() const override {return Descriptor::None;}
    
    void setSystem(const gp_Ax2 &systemIn);
    const gp_Ax2& getSystem() const {return system;}
    
  protected:
    gp_Ax2 system;
  };
}

#endif // CSYSBASE_H

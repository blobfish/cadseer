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

#ifndef FTR_INTERSECT_H
#define FTR_INTERSECT_H

#include <feature/base.h>

namespace ftr
{
  class Intersect : public Base
  {
  public:
    Intersect();
    virtual void updateModel(const UpdateMap&) override;
    virtual Type getType() const override {return Type::Intersect;}
    virtual const std::string& getTypeString() const override {return toString(Type::Intersect);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Alter;}
    
  private:
    static QIcon icon;
  };
}

#endif // FTR_INTERSECT_H

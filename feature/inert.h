/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2015  tanderson <email>
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

#ifndef INERT_H
#define INERT_H

#include "base.h"

namespace Feature
{
  /*! @brief static feature.
   * 
   * feature that has no real parameters or update.
   * for example, used for import geometry.
   */
  class Inert : public Base
  {
  public:
    Inert(const TopoDS_Shape &shapeIn);
    virtual void update(const UpdateMap&) override {}
    virtual Type getType() const override {return Type::Inert;}
    virtual const std::string& getTypeString() const override {return Feature::getTypeString(Type::Inert);}
  private:
    Inert(){};
  };
}

#endif // INERT_H

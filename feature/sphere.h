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

#ifndef SPHERE_H
#define SPHERE_H

#include "base.h"

namespace Feature
{
  class Sphere : public Base
  {
  public:
    Sphere();
    void setRadius(const double &radiusIn);
    double getRadius() const {return radius;}
    virtual void update(const UpdateMap&) override;
    virtual Type getType() const override {return Type::Sphere;}
    virtual const std::string& getTypeString() const override {return Feature::getTypeString(Type::Sphere);}
    
  protected:
    double radius;
  };
}

#endif // SPHERE_H

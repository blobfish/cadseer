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

#ifndef CONE_H
#define CONE_H

#include "base.h"

namespace Feature
{
  class Cone : public Base
  {
  public:
    Cone();
    void setRadius1(const double &radius1In);
    void setRadius2(const double &radius2In);
    void setHeight(const double &heightIn);
    void setParameters(const double &radius1In, const double &radius2In, const double &heightIn);
    double getRadius1() const {return radius1;}
    double getRadius2() const {return radius2;}
    double getHeight() const {return height;}
    void getParameters (double &radius1Out, double &radius2Out, double &heightOut) const;
    virtual void update(const UpdateMap&) override;
    virtual Type getType() const override {return Type::Cone;}
    virtual const std::string& getTypeString() const override {return Feature::getTypeString(Type::Cone);}
    
  protected:
    double radius1;
    double radius2; //!< maybe zero.
    double height;
    
    
    
  };
}

#endif // CONE_H

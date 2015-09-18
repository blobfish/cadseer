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

#ifndef SPHERE_H
#define SPHERE_H

#include "csysbase.h"

class BRepPrimAPI_MakeSphere;

namespace Feature
{
  class Sphere : public CSysBase
  {
  public:
    Sphere();
    void setRadius(const double &radiusIn);
    double getRadius() const {return radius;}
    virtual void update(const UpdateMap&) override;
    virtual Type getType() const override {return Type::Sphere;}
    virtual const std::string& getTypeString() const override {return Feature::getTypeString(Type::Sphere);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
    
  protected:
    double radius;
    
    void initializeMaps();
    void updateResult(BRepPrimAPI_MakeSphere&);
    
  private:
    static QIcon icon;
  };
}

#endif // SPHERE_H

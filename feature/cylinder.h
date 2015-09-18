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

#ifndef CYLINDER_H
#define CYLINDER_H

#include "csysbase.h"

namespace Feature
{
  class CylinderBuilder;
  
  class Cylinder : public CSysBase
  {
  public:
    Cylinder();
    void setRadius(const double &radiusIn);
    void setHeight(const double &heightIn);
    void setParameters(const double &radiusIn, const double &heightIn);
    double getRadius() const {return radius;}
    double getHeight() const {return height;}
    void getParameters (double &radiusOut, double &heightOut) const;
    
    virtual void update(const UpdateMap&) override;
    virtual Type getType() const override {return Type::Cylinder;}
    virtual const std::string& getTypeString() const override {return Feature::getTypeString(Type::Cylinder);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
  
  protected:
    double radius;
    double height;
    
    void initializeMaps();
    void updateResult(const CylinderBuilder &);
    
  private:
    static QIcon icon;
  };
}

#endif // CYLINDER_H

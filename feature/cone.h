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

#ifndef CONE_H
#define CONE_H

#include "csysbase.h"

namespace Feature
{
  class ConeBuilder;
  
  class Cone : public CSysBase
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
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
    
  protected:
    double radius1;
    double radius2; //!< maybe zero.
    double height;
    
    void initializeMaps();
    void updateResult(const ConeBuilder &);
    
  private:
    static QIcon icon;
  };
}

#endif // CONE_H

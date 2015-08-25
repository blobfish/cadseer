/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013  tanderson <email>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef BOX_H
#define BOX_H

#include "base.h"

class BoxBuilder;

namespace Feature
{
class Box : public Base
{
public:
  Box();
  void setLength(const double &lengthIn);
  void setWidth(const double &widthIn);
  void setHeight(const double &heightIn);
  void setParameters(const double &lengthIn, const double &widthIn, const double &heightIn);
  double getLength() const {return length;}
  double getWidth() const {return width;}
  double getHeight() const {return height;}
  void getParameters (double &lengthOut, double &widthOut, double &heightOut) const;
  virtual void update(const UpdateMap&) override;
  virtual Type getType() const override {return Type::Box;}
  virtual const std::string& getTypeString() const override {return Feature::getTypeString(Type::Box);}
  
protected:
  double length;
  double width;
  double height;
  
  void createResult();
  void updateResult(const BoxBuilder&);
  void createFeature(const BoxBuilder&);
  void createEvolution(const BoxBuilder&);
};
}

#endif // BOX_H

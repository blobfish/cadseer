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

#ifndef PARAMETER_H
#define PARAMETER_H

#include <string>

#include <boost/signals2.hpp>

namespace prj{namespace srl{class Parameter;}}

namespace ftr
{
  namespace ParameterNames
  {
    static const std::string Radius = "Radius"; //!< cylinder, sphere
    static const std::string Height = "Height"; //!< cylinder, box, cone
    static const std::string Length = "Length"; //!< box
    static const std::string Width = "Width"; //!< box
    static const std::string Radius1 = "Radius1"; //!< cone
    static const std::string Radius2 = "Radius2"; //!< cone
    static const std::string Position = "Position"; //!< blend
    
  }
  
  class Parameter
  {
  public:
    Parameter();
    Parameter(const std::string &nameIn, double valueIn);
    double getValue() const {return value;}
    void setValue(double valueIn);
    std::string getName() const {return name;}
    void setName(const std::string &nameIn){name = nameIn;}
    bool isConstant() const {return constant;} //!< true = not linked to forumla.
    void setConstant(bool constantIn);
    operator double() const {return value;}
    Parameter& operator=(double valueIn);
    
    typedef boost::signals2::signal<void ()> ValueChangedSignal;
    boost::signals2::connection connectValue(const ValueChangedSignal::slot_type &subscriber) const
    {
      return valueChangedSignal.connect(subscriber);
    }
    
    typedef boost::signals2::signal<void ()> ConstantChangedSignal;
    boost::signals2::connection connectConstant(const ConstantChangedSignal::slot_type &subscriber) const
    {
      return constantChangedSignal.connect(subscriber);
    }
    
    prj::srl::Parameter serialOut() const;
    void serialIn(const prj::srl::Parameter &sParameterIn);
    
  private:
    bool constant = true;
    std::string name;
    double value;
    
    //mutable allows us to connect to the signal through a const object.
    mutable ValueChangedSignal valueChangedSignal;
    mutable ConstantChangedSignal constantChangedSignal;
  };
  inline bool operator==(const Parameter &lhs, const Parameter &rhs)
  {
    return
    (
      (lhs.getName() == rhs.getName()) &&
      (lhs.getValue() == rhs.getValue()) &&
      (lhs.isConstant() == rhs.isConstant())
    );
  }
  inline bool operator==(const Parameter &lhs, double valueIn)
  {
    return lhs.getValue() == valueIn;
  }
  inline bool operator==(double valueIn, const Parameter &rhs)
  {
    return rhs.getValue() == valueIn;
  }
  
  typedef std::map<std::string, Parameter*> ParameterMap;
}

#endif // PARAMETER_H

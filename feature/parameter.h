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

#include <QString>

#include <boost/signals2/signal.hpp>
#include <boost/uuid/uuid.hpp>

namespace prj{namespace srl{class Parameter;}}

namespace ftr
{
  namespace ParameterNames
  {
    static const QString Radius = "Radius"; //!< cylinder, sphere
    static const QString Height = "Height"; //!< cylinder, box, cone
    static const QString Length = "Length"; //!< box
    static const QString Width = "Width"; //!< box
    static const QString Radius1 = "Radius1"; //!< cone
    static const QString Radius2 = "Radius2"; //!< cone
    static const QString Position = "Position"; //!< blend
    static const QString Distance = "Distance"; //!< chamfer
    static const QString Angle = "Angle"; //!< draft
    static const QString Offset = "Offset"; //!< datum plane
  }
  
  struct ParameterBoundary
  {
    enum class End
    {
      None,
      Open, //!< doesn't contain value.
      Closed //!< does contain value.
    };
    ParameterBoundary(double, End);
    double value;
    End end = End::None;
  };
  
  struct ParameterInterval
  {
    ParameterInterval(const ParameterBoundary&, const ParameterBoundary&);
    ParameterBoundary lower;
    ParameterBoundary upper;
    bool test(double) const;
  };
  
  struct ParameterConstraint
  {
    std::vector<ParameterInterval> intervals;
    bool test(double) const;
    
    static ParameterConstraint buildAll();
    static ParameterConstraint buildNonZeroPositive();
    static ParameterConstraint buildZeroPositive();
    static ParameterConstraint buildNonZeroNegative();
    static ParameterConstraint buildZeroNegative();
    static ParameterConstraint buildNonZero();
    static ParameterConstraint buildUnit();
    static void unitTest();
  };
  
  class Parameter
  {
  public:
    Parameter();
    Parameter(const QString &nameIn, double valueIn);
    double getValue() const {return value;}
    void setValue(double valueIn);
    QString getName() const {return name;}
    void setName(const QString &nameIn){name = nameIn;}
    bool isConstant() const {return constant;} //!< true = not linked to forumla.
    void setConstant(bool constantIn);
    operator double() const {return value;}
    Parameter& operator=(double valueIn);
    const boost::uuids::uuid& getId() const {return id;}
    void setConstraint(const ParameterConstraint &);
    bool isValidValue(const double &valueIn);
    
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
    QString name;
    double value;
    boost::uuids::uuid id;
    ParameterConstraint constraint;
    
    //mutable allows us to connect to the signal through a const object.
    mutable ValueChangedSignal valueChangedSignal;
    mutable ConstantChangedSignal constantChangedSignal;
  };
  inline bool operator==(const Parameter &lhs, const Parameter &rhs)
  {
    return
    (
      (lhs.getId() == rhs.getId())
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
  
  typedef std::vector<Parameter*> ParameterVector;
}

#endif // PARAMETER_H

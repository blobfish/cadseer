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
#include <boost/variant/variant_fwd.hpp>
#include <boost/filesystem/path.hpp>

#include <osg/Vec3d>
#include <osg/Quat>
#include <osg/Matrixd>

namespace prj{namespace srl{class Parameter;}}

namespace ftr
{
  namespace prm
  {
    namespace Names
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
    
    /*! Descriptor for path parameters.*/
    enum class PathType
    {
      None, //!< shouldn't be used.
      Directory, //!< future?
      Read, //!< think file open.
      Write, //!< think file save as.
    };
    
    typedef boost::variant
    <
      double,
      int,
      bool,
      std::string,
      boost::filesystem::path,
      osg::Vec3d,
      osg::Quat,
      osg::Matrixd
    > Variant;
    
    struct Boundary
    {
      enum class End
      {
        None,
        Open, //!< doesn't contain value.
        Closed //!< does contain value.
      };
      Boundary(double, End);
      double value;
      End end = End::None;
    };
    
    struct Interval
    {
      Interval(const Boundary&, const Boundary&);
      Boundary lower;
      Boundary upper;
      bool test(double) const;
    };
    
    struct Constraint
    {
      std::vector<Interval> intervals;
      bool test(double) const;
      
      static Constraint buildAll();
      static Constraint buildNonZeroPositive();
      static Constraint buildZeroPositive();
      static Constraint buildNonZeroNegative();
      static Constraint buildZeroNegative();
      static Constraint buildNonZero();
      static Constraint buildUnit();
      static void unitTest();
    };
    
    /*! @brief Parameters are values linked to features
     * 
     * We are using a boost variant to model various types that parmeters
     * can take. This presents some challenges as variants can change types
     * implicitly. We want parameters of different types but we want that
     * type to stay constant after being constructed. So no default constructor
     * and parameters' type established at construction by value passed in. setValue
     * functions will assert types are equal and no exception. Release build undefined
     * behaviour. value retrieval using explicit conversion operator using static_cast.
     * a little ugly, but should be safer. value retrieval will also assert with no
     * exception.
     * 
     */
    class Parameter
    {
    public:
      Parameter() = delete;
      Parameter(const QString &nameIn, double valueIn);
      Parameter(const QString &nameIn, int valueIn);
      Parameter(const QString &nameIn, bool valueIn);
      Parameter(const QString &nameIn, const boost::filesystem::path &valueIn, PathType);
      Parameter(const QString &nameIn, const osg::Vec3d &valueIn);
      
      QString getName() const {return name;}
      void setName(const QString &nameIn){name = nameIn;}
      bool isConstant() const {return constant;} //!< true = not linked to forumla.
      void setConstant(bool constantIn);
      const boost::uuids::uuid& getId() const {return id;}
      const std::type_info& getValueType() const; //!< compare return by: "getValueType() == typeid(std::string)"
      std::string getValueTypeString() const;
      const Variant& getVariant() const {return value;}
      PathType getPathType() const {return pathType;}
      
      //@{
      //! original functions from when only doubles supported.
      bool setValue(double valueIn);
      bool setValueQuiet(double valueIn); //!< don't trigger changed signal. use sparingly!
      explicit operator double() const;
      bool isValidValue(const double &valueIn) const;
      void setConstraint(const Constraint &); //!< only for doubles ..maybe ints?
      //@}
      
      //@{
      //! int support functions.
      bool setValue(int);
      bool setValueQuiet(int);
      explicit operator int() const;
      bool isValidValue(const int &valueIn) const; //!< just casting to double and using double constraints.
      //maybe setConstraint
      //@}
      
      //@{
      //! bool support functions.
      bool setValue(bool); //<! true = value was changed.
      bool setValueQuiet(bool);
      explicit operator bool() const;
      //@}
      
      //@{
      //! std::string support functions.
      explicit operator std::string() const;
      //@}
      
      //@{
      //! boost::filesystem::path support functions.
      bool setValue(const boost::filesystem::path&); //<! true = value was changed.
      bool setValueQuiet(const boost::filesystem::path&);
      explicit operator boost::filesystem::path() const;
      //@}
      
      //@{
      //! osg::Vec3d support functions.
      bool setValue(const osg::Vec3d&); //<! true = value was changed.
      bool setValueQuiet(const osg::Vec3d&);
      explicit operator osg::Vec3d() const;
      //@}
      
      //@{
      //! osg::Quat support functions.
      explicit operator osg::Quat() const;
      //@}
      
      //@{
      //! osg::Matrixd support functions.
      explicit operator osg::Matrixd() const;
      //@}
      
      
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
      Variant value;
      boost::uuids::uuid id;
      Constraint constraint;
      PathType pathType;
      
      //mutable allows us to connect to the signal through a const object.
      mutable ValueChangedSignal valueChangedSignal;
      mutable ConstantChangedSignal constantChangedSignal;
    };
    
    typedef std::vector<Parameter*> Vector;
  }
}

#endif // PARAMETER_H

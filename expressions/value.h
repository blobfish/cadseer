/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#include <boost/variant.hpp>

#include <osg/Matrixd>

#ifndef EXPR_VALUE_H
#define EXPR_VALUE_H

namespace expr
{
  //!* an object capable of holding all expression outputs.
  typedef boost::variant
  <
    double,
    osg::Vec3d,    //!< vector
    osg::Quat,     //!< orientation
    osg::Matrixd   //!< coordinate system
  > Value;
  
  /*! @brief ValueTypes. 
   * 
   * represent what kind of output the nodes have.
   */
  enum class ValueType
  {
    Variant = 0,  //!< might be any.
    Scalar,       //!< double value
    Vector,       //!< osg::Vec3d
    Orientation,  //!< osg::Matrixd with only rotation set.
    CSys          //!< osg::matrixd
  };
}

#endif // EXPR_VALUE_H

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

#ifndef TYPES_H
#define TYPES_H

#include <assert.h>
#include <map>
#include <string>

namespace Feature
{
  //! feature types. make sure and update function below.
  enum class Type
  {
    Base, //!< feature base class.
    CSys, //!< feature base class.
    Box, //!< feature box class.
    Sphere, //!< feature sphere class.
    Cone, //!< feature sphere class.
    Cylinder, //!< feature sphere class.
    Blend, //!< feature blend class.
    Inert, //!< feature box class.
    Union //!< feature box class.
  };
  
  inline const static std::string& getTypeString(Type typeIn)
  {
    typedef std::map<Type, const std::string> LocalMap;
    const static LocalMap strings = 
    {
      {Type::Base, "Base"},
      {Type::CSys, "CSys"},
      {Type::Box, "Box"},
      {Type::Sphere, "Sphere"},
      {Type::Cone, "Cone"},
      {Type::Cylinder, "Cylinder"},
      {Type::Blend, "Blend"},
      {Type::Inert, "Inert"},
      {Type::Union, "Union"}
    };
    
    assert(strings.count(typeIn) > 0);
    return strings.at(typeIn);
  }
}




#endif //TYPES_H
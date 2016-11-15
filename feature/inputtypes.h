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

#ifndef INPUTTYPES_H
#define INPUTTYPES_H

#include <assert.h>
#include <vector>
#include <string>
#include <map>

namespace ftr
{
  enum class InputTypes
  {
    none = 0,
    target,
    tool
  };
    
  inline const static std::string& getInputTypeString(InputTypes typeIn)
  {
    const static std::vector<std::string> strings = 
    {
      "None",
      "Target",
      "Tool"
    };
    
    std::size_t casted = static_cast<std::size_t>(typeIn);
    assert(casted < strings.size());
    return strings.at(casted);
  }
  
  inline const static InputTypes getInputFromString(const std::string &stringIn)
  {
    const static std::map<std::string, InputTypes> map
    {
      {"None", InputTypes::none},
      {"Target", InputTypes::target},
      {"Tool", InputTypes::tool}
    };
    
    auto it = map.find(stringIn);
    assert(it != map.end());
    return it->second;
  }
  
  enum class Descriptor
  {
    None,
    Create,
    Alter
  };
  
  inline const static std::string& getDescriptorString(Descriptor descriptorIn)
  {
    const static std::vector<std::string> strings =
    {
      "None",
      "Create",
      "Alter"
    };
    
    std::size_t casted = static_cast<std::size_t>(descriptorIn);
    assert(casted < strings.size());
    return strings.at(casted);
  }
}

#endif //INPUTTYPES_H
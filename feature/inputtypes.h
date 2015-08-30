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

#ifndef INPUTTYPES_H
#define INPUTTYPES_H

#include <assert.h>
#include <vector>
#include <string>

namespace Feature
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
}

#endif //INPUTTYPES_H
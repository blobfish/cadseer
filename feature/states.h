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

#ifndef STATES_H
#define STATES_H

#include <bitset>
#include <vector>
#include <string>
#include <assert.h>

namespace Feature
{
  typedef std::bitset<sizeof(int) * 8> State;
  
  namespace StateOffset
  {
    static const std::size_t ModelDirty = 0;
    static const std::size_t VisualDirty = 1;
    static const std::size_t Hidden3D = 2;
    static const std::size_t Failure = 3;
    static const std::size_t Inactive = 4;
    
    inline std::string toString(std::size_t offsetIn)
    {
      static const std::vector<std::string> strings =
      {
        "Model Dirty",
        "Visual Dirty",
        "Visibility3D",
        "Failure",
        "Inactive"
      };
      
      assert(offsetIn < strings.size());
      return strings[offsetIn];
    }
  };
}

#endif // STATES_H

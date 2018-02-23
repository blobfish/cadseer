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

#ifndef FTR_INPUTTYPE_H
#define FTR_INPUTTYPE_H

#include <assert.h>
#include <vector>
#include <set>
#include <string>
#include <initializer_list>

namespace ftr
{
  //! information describing connection between features
  class InputType
  {
  public:
    //@{
    //! Convenient string constants that may apply to more than one feature.
    constexpr static const char *target = "Target";
    constexpr static const char *tool = "Tool";
    constexpr static const char *create = "Create";
    //@}
    
    //@{
    //! Constructors
    InputType(){}
    InputType(std::initializer_list<std::string> listIn) : tags(listIn){}
    InputType(const InputType &other) : tags(other.tags){}
    //@}
    
    //@{
    //! Convenience wrappers around std::set
    bool has(const std::string &tagIn) const
    {
      return tags.count(tagIn) != 0;
    }
    void insert(const std::string &tagIn)
    {
      tags.insert(tagIn);
    }
    InputType& operator +=(const InputType &other)
    {
      for (const auto &t : other.tags)
        tags.insert(t);
      return *this;
    }
    //@}
    
    std::set<std::string> tags;
  };
}

#endif //FTR_INPUTTYPE_H

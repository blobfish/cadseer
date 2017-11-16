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

#ifndef LBR_GEOMETRYLIBRARYTAG_H
#define LBR_GEOMETRYLIBRARYTAG_H

#include <vector>
#include <string>
#include <algorithm>
#include <initializer_list>

namespace lbr
{
  struct Tag
  {
    Tag(){}
    Tag(const std::initializer_list<std::string> &listIn) : entries(listIn){}
    std::vector<std::string> entries;
  };

  inline Tag& operator<< (Tag &tag, const std::string &entryIn)
  {
    tag.entries.push_back(entryIn);
    return tag;
  }
  
  inline bool operator== (const Tag &lhs, const Tag &rhs)
  {
    if (lhs.entries.size() != rhs.entries.size())
      return false;
    return std::equal(lhs.entries.begin(), lhs.entries.end(), rhs.entries.begin());
  }
}


#endif //LBR_GEOMETRYLIBRARYTAG_H

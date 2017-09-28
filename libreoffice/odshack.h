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

#ifndef LBO_ODSHACK_H
#define LBO_ODSHACK_H

#include <string>
#include <map>

/* this is a specific hack for exporting strip
 * feature to a spreadsheet. Using this for anything
 * else will fail.
 */

namespace lbo
{
  struct Delimiter
  {
    std::string text = "";
    std::size_t pos = std::string::npos;
    
    bool isValid();
    std::size_t toEnd();
    
    static Delimiter build(const std::string&, const std::string&, std::size_t startPos = 0);
  };

  struct Disect
  {
    Delimiter start;
    Delimiter end;
    std::string value;
    
    bool isValid();
    bool replace(std::string &, const std::string&);
    
    static Disect build(const std::string&, const std::string&, const std::string&, std::size_t startPos = 0);
  };
  
  typedef std::pair<std::size_t, std::size_t> MapKey; //!< row, column
  typedef std::map<MapKey, std::string> Map;
  
  void replace(std::string &s, const Map &mi); //!< replace row and column in 'cs' sheet.
}

std::ostream& operator <<(std::ostream&, const lbo::Delimiter &);
std::ostream& operator <<(std::ostream&, const lbo::Disect &);

#endif // LBO_ODSHACK_H

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

#include <iostream>

#include <libreoffice/odshack.h>

using namespace lbo;

bool Delimiter::isValid()
{
  if (text.empty())
    return false;
  if (pos == std::string::npos)
    return false;
  return true;
}

std::size_t Delimiter::toEnd()
{
  return pos + text.size();
}

Delimiter Delimiter::build(const std::string &source, const std::string &target, std::size_t startPos)
{
  Delimiter out;
  
  std::size_t pos = source.find(target, startPos);
  if (pos == std::string::npos)
    return out;
  
  out.text = target;
  out.pos = pos;
  
  return out;
}

Disect Disect::build(const std::string &source, const std::string &start, const std::string &end, std::size_t startPos)
{
  Disect out;
  
  out.start = Delimiter::build(source, start, startPos);
  if (!out.start.isValid())
    return out;
  
  out.end = Delimiter::build(source, end, out.start.toEnd());
  if (!out.end.isValid())
    return out;
  
  std::size_t valueStart = out.start.toEnd();
  std::size_t valueEnd = out.end.pos;
  if (valueStart >= valueEnd) //should have at least 1 character.
    return out;
  
  out.value = source.substr(valueStart, valueEnd - valueStart); 
  
  return out;
}

bool Disect::isValid()
{
  if (!start.isValid())
    return false;
  if (!end.isValid())
    return false;
  if (value.empty())
    return false;
  
  return true;
}

//this function does NOT upate positions! Don't use object after this.
bool Disect::replace(std::string &source, const std::string &newText)
{
  if (!isValid())
    return false;
  if (start.pos >= source.size())
    return false;
  if (end.pos >= source.size())
    return false;
  
  std::size_t valueStart = start.toEnd();
  std::size_t valueEnd = end.pos;
  if (valueStart >= valueEnd) //should have at least 1 character.
    return false;
  
  source.replace(valueStart, valueEnd - valueStart, newText);
  return true;
}

void lbo::replace(std::string &s, const lbo::Map &mi)
{
  std::size_t s1 = s.find("<office:spreadsheet>", 0);
  if (s1 == std::string::npos)
    return;
  
  lbo::Disect table = lbo::Disect::build(s, "table:name=\"cs\"", "</table:table>", s1);
  if (!table.isValid())
  {
    std::cout << "couldn't find cadseer table" << std::endl;
    return;
  }
  
  std::size_t ri = 0; //row index
  lbo::Disect crow = lbo::Disect::build(s, "<table:table-row ", "</table:table-row>", table.start.toEnd());
  while (crow.isValid() && (crow.end.pos < table.end.pos))
  {
    std::size_t ci = 0; //column index
    lbo::Disect ccell = lbo::Disect::build(s, "<table:table-cell ", "</table:table-cell>", crow.start.toEnd());
//     std::cout << "row: " << ri;
    while (ccell.isValid() && (ccell.end.pos < crow.end.pos))
    {
      auto it = mi.find(std::make_pair(ri, ci));
      if (it != mi.end())
      {
        lbo::Disect ctext = lbo::Disect::build(s, "<text:p>", "</text:p>", ccell.start.toEnd());
        if (ctext.isValid() && (ctext.end.pos < ccell.end.pos))
        {
          ctext.replace(s, std::get<1>(*it));
//           std::cout << "          " << ctext.start.text << " = " << std::get<1>(*it);
        }
        lbo::Disect cvalue = lbo::Disect::build(s, "office:value-type=\"float\" office:value=\"", "\"", ccell.start.toEnd());
        if (cvalue.isValid() && (cvalue.end.pos < ccell.end.pos))
        {
          cvalue.replace(s, std::get<1>(*it));
//           std::cout << "          " << cvalue.start.text << " = " << std::get<1>(*it);
        }
      }
      ccell = lbo::Disect::build(s, "table:table-cell ", "</table:table-cell>", ccell.start.toEnd());
      ci++;
    }
//     std::cout << std::endl;
    crow = lbo::Disect::build(s, "<table:table-row ", "</table:table-row>", crow.start.toEnd());
    ri++;
    
    //end of table changes as we add or remove data.
    table = lbo::Disect::build(s, "table:name=\"cs\"", "</table:table>", s1);
  }
}

std::ostream& operator <<(std::ostream &s, const lbo::Delimiter &dIn)
{
  s << "text is: " << dIn.text << "    pos is: " << dIn.pos;
  return s;
}

std::ostream& operator <<(std::ostream &s, const lbo::Disect &dIn)
{
  s << "start is: " << dIn.start << std::endl
  << "end is: " << dIn.end << std::endl
  << "value is: " << dIn.value << std::endl;
  
  return s;
}

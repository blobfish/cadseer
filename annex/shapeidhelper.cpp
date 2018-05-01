/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2018  Thomas S. Anderson blobfish.at.gmx.com
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

#include <cassert>

#include <boost/filesystem.hpp>

#include <tools/idtools.h>
#include <annex/shapeidhelper.h>

using namespace ann;
using boost::optional;
using boost::uuids::uuid;

void ShapeIdHelper::add(const uuid &idIn, const TopoDS_Shape &shapeIn)
{
  ids.push_back(idIn);
  shapes.push_back(shapeIn);
}

optional<uuid> ShapeIdHelper::find(const TopoDS_Shape &shapeIn) const
{
  assert(ids.size() == shapes.size());
  for (auto it = shapes.begin(); it != shapes.end(); ++it)
  {
    if (shapeIn.IsEqual(*it))
    {
      std::size_t index = std::distance(shapes.begin(), it);
      assert(index < shapes.size());
      return ids.at(index);
    }
  }
  //used by external lod generator, so can't use std::cout.
//   std::cout << "Warning: no id for shape in ShapeIdHelper::find" << std::endl;
  return boost::none;
}

optional<const TopoDS_Shape&> ShapeIdHelper::find(const uuid &idIn) const
{
  assert(ids.size() == shapes.size());
  auto it = std::find(ids.begin(), ids.end(), idIn);
  if (it == ids.end())
  {
    //used by external lod generator, so can't use std::cout.
//     std::cout << "Warning: no shape for id in ShapeIdHelper::find" << std::endl;
    return boost::none;
  }
  std::size_t index = std::distance(ids.begin(), it);
  return shapes.at(index);
}

std::ostream& ann::operator<<(std::ostream &stream, const ShapeIdHelper &helperIn)
{
  for (std::size_t index = 0; index < helperIn.ids.size(); ++index)
  {
    stream
    << "id is: " << gu::idToString(helperIn.ids.at(index))
    << "    hash code is: " << occt::getShapeHash(helperIn.shapes.at(index))
    << std::endl;
  }
  
  return stream;
}

void ShapeIdHelper::write(const boost::filesystem::path &pathIn)
{
  std::ofstream stream(pathIn.string());
  for (const auto &id : ids)
    stream << gu::idToString(id) << std::endl;
}

std::vector<boost::uuids::uuid> ShapeIdHelper::read(const boost::filesystem::path &pathIn)
{
  std::vector<boost::uuids::uuid> out;
  std::ifstream stream(pathIn.string());
  std::string buffer;
  while(std::getline(stream, buffer))
    out.push_back(gu::stringToId(buffer));
  return out;
}

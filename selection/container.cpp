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

#include <boost/uuid/uuid_io.hpp>

#include <selection/container.h>

using namespace slc;

std::ostream& slc::operator<<(std::ostream& os, const Container& container)
{
  os << 
    "type is: " << getNameOfType(container.selectionType) << 
    "      featureid is: " << boost::uuids::to_string(container.featureId) <<
    "      id is: " << boost::uuids::to_string(container.shapeId) << std::endl;
  return os;
}

std::ostream& slc::operator<<(std::ostream& os, const Containers& containers)
{
  for (const auto &current : containers)
    os << current;
  return os;
}

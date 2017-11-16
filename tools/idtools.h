/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef GU_IDTOOLS_H
#define GU_IDTOOLS_H

#include <string>

#include <boost/uuid/uuid.hpp>

namespace gu
{
  boost::uuids::uuid createRandomId();
  boost::uuids::uuid createNilId();
  std::string idToString(const boost::uuids::uuid &idIn);
  boost::uuids::uuid stringToId(const std::string &stringIn);
}

#endif // GU_IDTOOLS_H

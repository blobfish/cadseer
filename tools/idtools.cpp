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

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <tools/idtools.h>

//note:     typedef basic_random_generator<mt19937> random_generator;
static thread_local boost::uuids::random_generator rGen;
static thread_local boost::uuids::string_generator sGen;
static thread_local boost::uuids::nil_generator nGen;

using namespace boost::uuids;

uuid gu::createRandomId()
{
  return rGen();
}

boost::uuids::uuid gu::createNilId()
{
  return nGen();
}

std::string gu::idToString(const boost::uuids::uuid &idIn)
{
  return boost::uuids::to_string(idIn);
}

boost::uuids::uuid gu::stringToId(const std::string &stringIn)
{
  return sGen(stringIn);
}

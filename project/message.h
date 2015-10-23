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

#include <boost/uuid/uuid.hpp>

#ifndef MESSAGE_H
#define MESSAGE_H

namespace ProjectSpace
{
  struct Message
  {
    enum class Type
    {
      None = 0,
      Request,
      Response
    };
    
    enum class Action
    {
      None = 0,
      SetCurrentLeaf,
      RemoveFeature
    };
    
    Message();
    Message::Type type;
    Action action;
    boost::uuids::uuid featureId;
  };
}

#endif // MESSAGE_H

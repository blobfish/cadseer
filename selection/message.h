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

#ifndef SELECTIONMESSAGE_H
#define SELECTIONMESSAGE_H

#include <vector>

#include <boost/uuid/uuid.hpp>

#include <osg/Vec3d>

#include <feature/types.h>
#include <selection/definitions.h>

namespace slc
{
  struct Message
  {
    Message();
    slc::Type type;
    ftr::Type featureType;
    boost::uuids::uuid featureId;
    boost::uuids::uuid shapeId;
    osg::Vec3d pointLocation;
  };
  
  inline bool operator==(const Message& lhs, const Message& rhs)
  {
    return
    (
      (lhs.type == rhs.type) &&
      (lhs.featureId == rhs.featureId) &&
      (lhs.shapeId == rhs.shapeId)
      //we ignore point location.
      //shouldn't need to test featureType if id's are the same
    );
  }
  
  inline bool operator!=(const Message& lhs, const Message& rhs)
  {
    return !(lhs == rhs);
  }
  
  typedef std::vector<Message> Messages;
  bool has(const Messages &messagesIn, const Message &messageIn);
  void add(Messages &messagesIn, const Message &messageIn);
  void remove(Messages &messagesIn, const Message &messageIn);
}

#endif // SELECTIONMESSAGE_H

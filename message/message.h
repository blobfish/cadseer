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

#ifndef MSG_MESSAGE_H
#define MSG_MESSAGE_H

#include <bitset>
#include <functional>
#include <map>

#include <boost/variant.hpp>

#include <project/message.h>
#include <selection/message.h>

namespace msg
{
  //! Mask is for a key in a function dispatch message handler. We might need a validation routine.
  // no pre and post on requests only on response.
  typedef std::bitset<32> Mask; //pay attention to size we convert to ullong for comparison operator.
  static const std::size_t Request =		1 << 1;		//!< message classification
  static const std::size_t Response =		1 << 2;		//!< message classification
  static const std::size_t Pre =		1 << 3;		//!< message response timing. think "about to". Data still valid
  static const std::size_t Post =		1 << 4;		//!< message response timing. think "done". Data invalid.
  static const std::size_t Preselection =	1 << 5;		//!< selection classification
  static const std::size_t Selection =		1 << 6;		//!< selection classification
  static const std::size_t Addition =		1 << 7;		//!< selection action
  static const std::size_t Subtraction =	1 << 8;		//!< selection action
  static const std::size_t Clear =		1 << 9;		//!< selection action
  static const std::size_t SetCurrentLeaf =	1 << 10;	//!< project action
  static const std::size_t RemoveFeature =	1 << 11;	//!< project action
  static const std::size_t AddFeature	 =	1 << 12;	//!< project action
  static const std::size_t Update	 =	1 << 13;	//!< project action
  static const std::size_t AddConnection =	1 << 14;	//!< project action
  static const std::size_t RemoveConnection =	1 << 15;	//!< project action
  
  typedef boost::variant<prj::Message, slc::Message> Payload;
  
  struct Message
  {
    Mask mask = 0;
    Payload payload;
  };
  
  typedef std::function< void (const Message&) > MessageHandler;
  
  struct MaskCompare
  {
    bool operator() (const Mask& lhs, const Mask& rhs){return lhs.to_ullong() < rhs.to_ullong();}
  };
  typedef std::map<Mask, MessageHandler, MaskCompare> MessageDispatcher;
}



#endif // MSG_MESSAGE_H

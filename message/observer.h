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

#ifndef MSG_OBSERVER_H
#define MSG_OBSERVER_H

#include <boost/signals2/signal.hpp>

#include <message/message.h>

namespace boost{namespace signals2{class shared_connection_block;}}

namespace msg
{
  class Observer
  {
  public:
    Observer();
    ~Observer();
    std::string name = "no name"; //used for any error messages.
    typedef boost::signals2::signal<void (const msg::Message &)> MessageOutSignal;
    MessageOutSignal out; //outgoing messages
    MessageDispatcher dispatcher; //incoming message map.
    boost::signals2::connection connection;
    void outBlocked(const msg::Message &); //!< block, send message, unblock.
    boost::signals2::shared_connection_block createBlocker();
  private:
    void messageInSlot(const Message &);
    std::size_t stackDepth = 0;
  };
}

#endif // OBSERVER_H

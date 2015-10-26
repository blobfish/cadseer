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

#include <boost/signals2.hpp>

#include <message/message.h>

#ifndef MSG_DISPATCH_H
#define MSG_DISPATCH_H

namespace msg
{
  /*! @brief Class for message dispatching.
   * 
   * messages received are simply forwarded to all connected
   * recipients. @warning Dispatch does NOT check for equality
   * between sender and receiver. It is left to receivers to
   * determin if they are the source of the signal and if/how to 
   * respond. Potential recursion! @see Message
   */
  class Dispatch
  {
  public:
    typedef boost::signals2::signal<void (const Message &)> MessageOutSignal;
    boost::signals2::connection connectMessageOut(const MessageOutSignal::slot_type &subscriber)
    {
      return messageOutSignal.connect(subscriber);
    }
    
    void messageInSlot(const Message&);
    void dumpString(const std::string &); //!< choke point for message debug output.
  private:
    MessageOutSignal messageOutSignal;
  };
  
  //! singleton dispatch.
  Dispatch& dispatch();
}
#endif // MSG_DISPATCH_H

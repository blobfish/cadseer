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


#ifndef CMD_MANAGER_H
#define CMD_MANAGER_H

#include <stack>

#include <boost/signals2.hpp>

#include <message/message.h>
#include <command/base.h>

namespace cmd
{
  class Manager
  {
  public:
    Manager();
    
    typedef boost::signals2::signal<void (const msg::Message&)> MessageOutSignal;
    boost::signals2::connection connectMessageOut(const MessageOutSignal::slot_type &subscriber)
    {
      return messageOutSignal.connect(subscriber);
    }
    
    void messageInSlot(const msg::Message&);
    void addCommand(BasePtr);
  private:
    MessageOutSignal messageOutSignal;
    msg::MessageDispatcher dispatcher;
    void cancelCommandDispatched(const msg::Message &);
    void doneCommandDispatched(const msg::Message &);
    void setupDispatcher();
    void doneSlot();
    void activateTop();
    void sendStatusMessage(const std::string &messageIn);
    void sendCommandMessage(const std::string &messageIn);
    
    void clearSelection();
    
    std::stack<BasePtr> stack;
    
    void featureToSystemDispatched(const msg::Message &);
    void systemToFeatureDispatched(const msg::Message &);
    void featureToDraggerDispatched(const msg::Message &);
    void draggerToFeatureDispatched(const msg::Message &);
  };
  
  Manager& manager();
}

#endif // CMD_MANAGER_H

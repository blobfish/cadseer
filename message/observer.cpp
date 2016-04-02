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

#include <iostream>

#include <message/dispatch.h>
#include <message/observer.h>

using namespace msg;

Observer::Observer()
{
  messageOutSignal.connect(boost::bind(&msg::Dispatch::messageInSlot, &msg::dispatch(), _1));
  connection = msg::dispatch().connectMessageOut(boost::bind(&Observer::messageInSlot, this, _1));
}

Observer::~Observer()
{
  connection.disconnect();
}

void Observer::messageInSlot(const Message &messageIn)
{
  msg::MessageDispatcher::iterator it = dispatcher.find(messageIn.mask);
  if (it == dispatcher.end())
    return;
  
  if (stackDepth)
    std::cout << "WARNING: " << name << " stack depth: " << stackDepth << std::endl;
  stackDepth++;
  it->second(messageIn);
  stackDepth--;
}

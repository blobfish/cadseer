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

#include <iostream>

#include <message/dispatch.h>

using namespace msg;

void Dispatch::messageInSlot(const Message &message)
{
  messageOutSignal(message);
}

void Dispatch::startLogging()
{
  log.clear();
  isLogging = true;
}

void Dispatch::stopLogging()
{
  isLogging = false;
}

void Dispatch::dumpString(const std::string& stringIn)
{
  if (!isLogging)
    return;
  
  log += stringIn; //no formatting here.
}

void Dispatch::dumpConnectionCount()
{
  std::cout << messageOutSignal.num_slots() << std::endl;
}

Dispatch& msg::dispatch()
{
  static Dispatch localDispatcher;
  return localDispatcher;
}

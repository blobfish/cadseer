/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017  tanderson <email>
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
 */

#ifndef CMD_NEST_H
#define CMD_NEST_H

#include <command/base.h>

namespace cmd
{
  class Nest : public Base
  {
  public:
    Nest();
    ~Nest();
    
    virtual std::string getCommandName() override{return "Nest";}
    virtual std::string getStatusMessage() override;
    virtual void activate() override;
    virtual void deactivate() override;
    
  private:
    void go();
  };
}

#endif // CMD_NEST_H

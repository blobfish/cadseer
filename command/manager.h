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

#ifndef COMMANDMANAGER_H
#define COMMANDMANAGER_H

#include <command/command.h>

namespace cmd
{
class Manager
{
public:
    static Manager& getManager();
    void addCommand(const Command &commandIn);
    void trigger(const cmd::Constants constant);

private:
    Manager(){}
    Manager(const Manager &other);
    void operator=(const Manager &other);

    typedef std::map<cmd::Constants, Command> CommandMap;
    CommandMap commandMap;
};
}

#endif // COMMANDMANAGER_H

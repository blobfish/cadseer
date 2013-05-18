#include <assert.h>
#include <iostream>

#include "commandmanager.h"

CommandManager& CommandManager::getManager()
{
    static CommandManager manager;
    return manager;
}

void CommandManager::addCommand(const Command &commandIn)
{
    commandMap.insert(std::make_pair(commandIn.getId(), commandIn));
}

void CommandManager::trigger(const CommandConstants::Constants constant)
{
    CommandMap::const_iterator it = commandMap.find(constant);
    assert(it != commandMap.end());
    it->second.trigger();
}

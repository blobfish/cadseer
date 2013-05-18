#ifndef COMMANDMANAGER_H
#define COMMANDMANAGER_H

#include "command.h"

class CommandManager
{
public:
    static CommandManager& getManager();
    void addCommand(const Command &commandIn);
    void trigger(const CommandConstants::Constants constant);

private:
    CommandManager(){}
    CommandManager(const CommandManager &other);
    void operator=(const CommandManager &other);

    typedef std::map<CommandConstants::Constants, Command> CommandMap;
    CommandMap commandMap;
};

#endif // COMMANDMANAGER_H

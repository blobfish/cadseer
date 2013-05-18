#include "command.h"

Command::Command(CommandConstants::Constants idIn, QString nameIn, QAction *actionIn) : id(idIn), name(nameIn), action(actionIn)
{
}


CommandConstants::Constants Command::getId() const
{
    return id;
}

QString Command::getName() const
{
    return name;
}

QAction* Command::getAction() const
{
    return action;
}

void Command::trigger() const
{
    action->trigger();
}

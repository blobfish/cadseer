#ifndef COMMAND_H
#define COMMAND_H

#include <QString>
#include <QAction>

#include "commandconstants.h"

class Command
{
public:
    Command(CommandConstants::Constants idIn, QString nameIn, QAction *actionIn);
    CommandConstants::Constants getId() const;
    QString getName() const;
    QAction* getAction() const;
    void trigger() const;

protected:
    CommandConstants::Constants id;
    QString name;
    QAction *action;
};

#endif // COMMAND_H

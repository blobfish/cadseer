#ifndef SELECTIONSTATE_H
#define SELECTIONSTATE_H

class SelectionState
{
public:
    SelectionState();
    bool facesEnabled;
    bool facesSelectable;
    bool edgesEnabled;
    bool edgesSelectable;
    bool verticesEnabled;
    bool verticesSelectable;
};

#endif // SELECTIONSTATE_H

#ifndef SELECTIONMANAGER_H
#define SELECTIONMANAGER_H

#include <QObject>
#include <QStack>
#include "selectionstate.h"

class SelectionManager : public QObject
{
    Q_OBJECT
public:
    explicit SelectionManager(QObject *parent = 0);
    void sendState();
    void startCommand(const SelectionState &stateIn); //uses stack for restoration of selection state
    void endCommand(); //uses stack for restoration of selection state
    void setState(const SelectionState &stateIn); //DOESN'T use stack for restoration of selection state
    SelectionState getState();
    
signals:
    void setSelectionMask(const int &mask);
    void guiFacesEnabled(const bool &state);
    void guiEdgesEnabled(const bool &state);
    void guiVerticesEnabled(const bool &state);
    void guiFacesSelectable(const bool &state);
    void guiEdgesSelectable(const bool &state);
    void guiVerticesSelectable(const bool &state);
    
public slots:
    void toggledFaces(bool faceStateIn);
    void toggledEdges(bool edgeStateIn);
    void toggledVertices(bool vertexStateIn);

private:
    void popState();
    void pushState();
    void updateGui();
    bool facesSelectable;
    bool edgesSelectable;
    bool verticesSelectable;
    bool facesEnabled;
    bool edgesEnabled;
    bool verticesEnabled;
    QStack<SelectionState> stateStack;
};

#endif // SELECTIONMANAGER_H

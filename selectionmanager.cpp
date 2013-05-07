#include "nodemaskdefs.h"
#include "selectionmanager.h"

SelectionManager::SelectionManager(QObject *parent) :
    QObject(parent), facesSelectable(false), edgesSelectable(false), verticesSelectable(false), facesEnabled(true),
    edgesEnabled(true), verticesEnabled(true)
{
}

void SelectionManager::toggledFaces(bool faceStateIn)
{
    facesSelectable = faceStateIn;
    sendState();
}

void SelectionManager::toggledEdges(bool edgeStateIn)
{
    edgesSelectable = edgeStateIn;
    sendState();
}

void SelectionManager::toggledVertices(bool vertexStateIn)
{
    verticesSelectable = vertexStateIn;
    sendState();
}

void SelectionManager::sendState()
{
    int out = 0xffffffff;
    out &= ~NodeMask::noSelect;
    if (!facesSelectable || !facesEnabled)
        out &= ~NodeMask::face;
    if (!edgesSelectable || !edgesEnabled)
        out &= ~NodeMask::edge;
    if (!verticesSelectable || !verticesEnabled)
        out &= ~NodeMask::vertex;
    emit setSelectionMask(out);
}

void SelectionManager::popState()
{
    if (stateStack.size() < 1)
        return;
    SelectionState currentState = stateStack.pop();
    setState(currentState);
}

void SelectionManager::pushState()
{
    SelectionState currentState = getState();
    stateStack.push(currentState);
}

void SelectionManager::startCommand(const SelectionState &stateIn)
{
    pushState();
    setState(stateIn);
}

void SelectionManager::endCommand()
{
    popState();
}

void SelectionManager::setState(const SelectionState &stateIn)
{
    facesEnabled = stateIn.facesEnabled;
    edgesEnabled = stateIn.edgesEnabled;
    verticesEnabled = stateIn.verticesEnabled;
    facesSelectable = stateIn.facesSelectable;
    edgesSelectable = stateIn.edgesSelectable;
    verticesSelectable = stateIn.verticesSelectable;
    updateGui();
    sendState();
}

SelectionState SelectionManager::getState()
{
    SelectionState currentState;
    currentState.facesEnabled = facesEnabled;
    currentState.edgesEnabled = edgesEnabled;
    currentState.verticesEnabled = verticesEnabled;
    currentState.facesSelectable = facesSelectable;
    currentState.edgesSelectable = edgesSelectable;
    currentState.verticesSelectable = verticesSelectable;
    return currentState;
}

void SelectionManager::updateGui()
{
    emit guiFacesEnabled(facesEnabled);
    emit guiEdgesEnabled(edgesEnabled);
    emit guiVerticesEnabled(verticesEnabled);
    emit guiFacesSelectable(facesSelectable);
    emit guiEdgesSelectable(edgesSelectable);
    emit guiVerticesSelectable(verticesSelectable);
}

#include <QAction>

#include "nodemaskdefs.h"
#include "selectiondefs.h"
#include "selectionmanager.h"

SelectionManager::SelectionManager(QObject *parent) :
    QObject(parent), selectionMask(SelectionMask::none)
{
}

void SelectionManager::triggeredObjects(bool objectStateIn)
{
    if (objectStateIn)
        selectionMask |= SelectionMask::objectsSelectable;
    else
        selectionMask &= ~SelectionMask::objectsSelectable;
    sendState();
}

void SelectionManager::triggeredFeatures(bool featureStateIn)
{
    if (featureStateIn)
        selectionMask |= SelectionMask::featuresSelectable;
    else
        selectionMask &= ~SelectionMask::featuresSelectable;
    sendState();
}

void SelectionManager::triggeredSolids(bool solidStateIn)
{
    if (solidStateIn)
        selectionMask |= SelectionMask::solidsSelectable;
    else
        selectionMask &= ~SelectionMask::solidsSelectable;
    sendState();
}

void SelectionManager::triggeredShells(bool shellStateIn)
{
    if (shellStateIn)
        selectionMask |= SelectionMask::shellsSelectable;
    else
        selectionMask &= ~SelectionMask::shellsSelectable;
    sendState();
}

void SelectionManager::triggeredFaces(bool faceStateIn)
{
    if (faceStateIn)
        selectionMask |= SelectionMask::facesSelectable;
    else
        selectionMask &= ~SelectionMask::facesSelectable;
    sendState();
}

void SelectionManager::triggeredWires(bool wireStateIn)
{
    if (wireStateIn)
        selectionMask |= SelectionMask::wiresSelectable;
    else
        selectionMask &= ~SelectionMask::wiresSelectable;
    sendState();
}

void SelectionManager::triggeredEdges(bool edgeStateIn)
{
    if (edgeStateIn)
        selectionMask |= SelectionMask::edgesSelectable;
    else
        selectionMask &= ~SelectionMask::edgesSelectable;
    sendState();
}

void SelectionManager::triggeredVertices(bool vertexStateIn)
{
    if (vertexStateIn)
        selectionMask |= SelectionMask::verticesSelectable;
    else
        selectionMask &= ~SelectionMask::verticesSelectable;
    sendState();
}

void SelectionManager::sendState()
{
//    int out = 0xffffffff;
//    out &= ~NodeMask::noSelect;
//    if ((selectionMask & SelectionMask::facesSelectable) != SelectionMask::facesSelectable)
//        out &= ~NodeMask::face;
//    if ((selectionMask & SelectionMask::edgesSelectable) != SelectionMask::edgesSelectable)
//        out &= ~NodeMask::edge;
//    if ((selectionMask & SelectionMask::verticesSelectable) != SelectionMask::verticesSelectable)
//        out &= ~NodeMask::vertex;
//    emit setSelectionMask(out);

    emit setSelectionMask(selectionMask);
}

void SelectionManager::popState()
{
    if (stateStack.size() < 1)
        return;
    selectionMask = stateStack.pop();
    updateToolbar();
    sendState();
}

void SelectionManager::pushState()
{
    stateStack.push(selectionMask);
}

void SelectionManager::startCommand(const unsigned int &stateIn)
{
    pushState();
    selectionMask = stateIn;
    updateToolbar();
    sendState();
}

void SelectionManager::endCommand()
{
    popState();
}

void SelectionManager::setState(const unsigned int &stateIn)
{
    selectionMask = stateIn;
    updateToolbar();
    sendState();
}

void SelectionManager::updateToolbar()
{
    actionSelectObjects->setEnabled((SelectionMask::objectsEnabled & selectionMask) == SelectionMask::objectsEnabled);
    actionSelectObjects->setChecked((SelectionMask::objectsSelectable & selectionMask) == SelectionMask::objectsSelectable);
    actionSelectFeatures->setEnabled((SelectionMask::featuresEnabled & selectionMask) == SelectionMask::featuresEnabled);
    actionSelectFeatures->setChecked((SelectionMask::featuresSelectable & selectionMask) == SelectionMask::featuresSelectable);
    actionSelectSolids->setEnabled((SelectionMask::solidsEnabled & selectionMask) == SelectionMask::solidsEnabled);
    actionSelectSolids->setChecked((SelectionMask::solidsSelectable & selectionMask) == SelectionMask::solidsSelectable);
    actionSelectShells->setEnabled((SelectionMask::shellsEnabled & selectionMask) == SelectionMask::shellsEnabled);
    actionSelectShells->setChecked((SelectionMask::shellsSelectable & selectionMask) == SelectionMask::shellsSelectable);
    actionSelectFaces->setEnabled((SelectionMask::facesEnabled & selectionMask) == SelectionMask::facesEnabled);
    actionSelectFaces->setChecked((SelectionMask::facesSelectable & selectionMask) == SelectionMask::facesSelectable);
    actionSelectWires->setEnabled((SelectionMask::wiresEnabled & selectionMask) == SelectionMask::wiresEnabled);
    actionSelectWires->setChecked((SelectionMask::wiresSelectable & selectionMask) == SelectionMask::wiresSelectable);
    actionSelectEdges->setEnabled((SelectionMask::edgesEnabled & selectionMask) == SelectionMask::edgesEnabled);
    actionSelectEdges->setChecked((SelectionMask::edgesSelectable & selectionMask) == SelectionMask::edgesSelectable);
    actionSelectVertices->setEnabled((SelectionMask::verticesEnabled & selectionMask) == SelectionMask::verticesEnabled);
    actionSelectVertices->setChecked((SelectionMask::verticesSelectable & selectionMask) == SelectionMask::verticesSelectable);
}

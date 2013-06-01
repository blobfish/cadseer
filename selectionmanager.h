#ifndef SELECTIONMANAGER_H
#define SELECTIONMANAGER_H

#include <QObject>
#include <QStack>

#include "selectiondefs.h"

class QAction;

class SelectionManager : public QObject
{
    Q_OBJECT
public:
    explicit SelectionManager(QObject *parent = 0);
    void sendState();
    void startCommand(const unsigned int &stateIn); //uses stack for restoration of selection state
    void endCommand(); //uses stack for restoration of selection state
    void setState(const unsigned int &stateIn); //DOESN'T use stack for restoration of selection state

    //thought about a couple of different ways. I don't want a thousand connect statements all over the place.
    QAction *actionSelectObjects;
    QAction *actionSelectFeatures;
    QAction *actionSelectSolids;
    QAction *actionSelectShells;
    QAction *actionSelectFaces;
    QAction *actionSelectWires;
    QAction *actionSelectEdges;
    QAction *actionSelectVertices;
    
signals:
    void setSelectionMask(const int &mask);
    
public slots:
    void triggeredObjects(bool objectStateIn);
    void triggeredFeatures(bool featureStateIn);
    void triggeredSolids(bool solidStateIn);
    void triggeredShells(bool shellStateIn);
    void triggeredFaces(bool faceStateIn);
    void triggeredWires(bool wireStateIn);
    void triggeredEdges(bool edgeStateIn);
    void triggeredVertices(bool vertexStateIn);

private:
    void popState();
    void pushState();
    void updateToolbar();

    unsigned int selectionMask;
    QStack<unsigned int> stateStack;
};

#endif // SELECTIONMANAGER_H

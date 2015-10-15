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

#ifndef SELECTIONMANAGER_H
#define SELECTIONMANAGER_H

#include <QObject>
#include <QStack>

#include <selection/definitions.h>

class QAction;

namespace Selection
{
class Manager : public QObject
{
    Q_OBJECT
public:
    explicit Manager(QObject *parent = 0);
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
    QAction *actionSelectEndPoints;
    QAction *actionSelectMidPoints;
    QAction *actionSelectCenterPoints;
    QAction *actionSelectQuadrantPoints;
    QAction *actionSelectNearestPoints;
    QAction *actionSelectScreenPoints;
    
Q_SIGNALS:
    void setSelectionMask(const int &mask);
    
public Q_SLOTS:
    void triggeredObjects(bool objectStateIn);
    void triggeredFeatures(bool featureStateIn);
    void triggeredSolids(bool solidStateIn);
    void triggeredShells(bool shellStateIn);
    void triggeredFaces(bool faceStateIn);
    void triggeredWires(bool wireStateIn);
    void triggeredEdges(bool edgeStateIn);
    void triggeredVertices(bool vertexStateIn);
    void triggeredEndPoints(bool endPointStateIn);
    void triggeredMidPoints(bool midPointStateIn);
    void triggeredCenterPoints(bool centerPointStateIn);
    void triggeredQuadrantPoints(bool quadrantPointStateIn);
    void triggeredNearestPoints(bool nearestPointStateIn);
    void triggeredScreenPoints(bool screenPointStateIn);

private:
    void popState();
    void pushState();
    void updateToolbar();

    unsigned int selectionMask;
    QStack<unsigned int> stateStack;
};
}

#endif // SELECTIONMANAGER_H

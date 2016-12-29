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

#include <memory>

#include <QObject>

#include <selection/definitions.h>

class QAction;

namespace msg{class Message; class Observer;}

namespace slc
{
class Manager : public QObject
{
    Q_OBJECT
public:
    explicit Manager(QObject *parent = 0);
    ~Manager();
    Mask getState(){return selectionMask;}

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
    void setState(Mask);
    void updateToolbar();
    Mask selectionMask;
    void sendUpdatedMask();
    std::unique_ptr<msg::Observer> observer;
    void setupDispatcher();
    void requestSelectionMaskDispatched(const msg::Message &);
};
}

#endif // SELECTIONMANAGER_H

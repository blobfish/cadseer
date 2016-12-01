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

#include <QAction>

#include <nodemaskdefs.h>
#include <selection/definitions.h>
#include <selection/manager.h>

using namespace slc;

Manager::Manager(QObject *parent) :
    QObject(parent), selectionMask(slc::None)
{
}

void Manager::triggeredObjects(bool objectStateIn)
{
    if (objectStateIn)
        selectionMask |= slc::ObjectsSelectable;
    else
        selectionMask &= ~slc::ObjectsSelectable;
    sendState();
}

void Manager::triggeredFeatures(bool featureStateIn)
{
    if (featureStateIn)
        selectionMask |= slc::FeaturesSelectable;
    else
        selectionMask &= ~slc::FeaturesSelectable;
    sendState();
}

void Manager::triggeredSolids(bool solidStateIn)
{
    if (solidStateIn)
        selectionMask |= slc::SolidsSelectable;
    else
        selectionMask &= ~slc::SolidsSelectable;
    sendState();
}

void Manager::triggeredShells(bool shellStateIn)
{
    if (shellStateIn)
        selectionMask |= slc::ShellsSelectable;
    else
        selectionMask &= ~slc::ShellsSelectable;
    sendState();
}

void Manager::triggeredFaces(bool faceStateIn)
{
    if (faceStateIn)
        selectionMask |= slc::FacesSelectable;
    else
        selectionMask &= ~slc::FacesSelectable;
    sendState();
}

void Manager::triggeredWires(bool wireStateIn)
{
    if (wireStateIn)
        selectionMask |= slc::WiresSelectable;
    else
        selectionMask &= ~slc::WiresSelectable;
    sendState();
}

void Manager::triggeredEdges(bool edgeStateIn)
{
    if (edgeStateIn)
        selectionMask |= slc::EdgesSelectable;
    else
        selectionMask &= ~slc::EdgesSelectable;
    sendState();
}

void Manager::triggeredVertices(bool vertexStateIn)
{
    if (vertexStateIn)
    {
        selectionMask |= slc::PointsSelectable;
        selectionMask |= slc::EndPointsEnabled;
        selectionMask |= slc::MidPointsEnabled;
        selectionMask |= slc::CenterPointsEnabled;
        selectionMask |= slc::QuadrantPointsEnabled;
        selectionMask |= slc::NearestPointsEnabled;
    }
    else
    {
        selectionMask &= ~slc::PointsSelectable;
        selectionMask &= ~slc::EndPointsEnabled;
        selectionMask &= ~slc::MidPointsEnabled;
        selectionMask &= ~slc::CenterPointsEnabled;
        selectionMask &= ~slc::QuadrantPointsEnabled;
        selectionMask &= ~slc::NearestPointsEnabled;
    }
    updateToolbar();
    sendState();
}

void Manager::triggeredEndPoints(bool endPointStateIn)
{
  if (endPointStateIn)
    selectionMask |= slc::EndPointsSelectable;
  else
    selectionMask &= ~slc::EndPointsSelectable;
  sendState();
}

void Manager::triggeredMidPoints(bool midPointStateIn)
{
  if (midPointStateIn)
    selectionMask |= slc::MidPointsSelectable;
  else
    selectionMask &= ~slc::MidPointsSelectable;
  sendState();
}

void Manager::triggeredCenterPoints(bool centerPointStateIn)
{
  if (centerPointStateIn)
    selectionMask |= slc::CenterPointsSelectable;
  else
    selectionMask &= ~slc::CenterPointsSelectable;
  sendState();

}

void Manager::triggeredQuadrantPoints(bool quadrantPointStateIn)
{
  if (quadrantPointStateIn)
    selectionMask |= slc::QuadrantPointsSelectable;
  else
    selectionMask &= ~slc::QuadrantPointsSelectable;
  sendState();

}

void Manager::triggeredNearestPoints(bool nearestPointStateIn)
{
  if (nearestPointStateIn)
    selectionMask |= slc::NearestPointsSelectable;
  else
    selectionMask &= ~slc::NearestPointsSelectable;
  sendState();
}

void Manager::triggeredScreenPoints(bool screenPointStateIn)
{
  if (screenPointStateIn)
    selectionMask |= slc::ScreenPointsSelectable;
  else
    selectionMask &= ~slc::ScreenPointsSelectable;
  sendState();
}


void Manager::sendState()
{
//    int out = 0xffffffff;
//    out &= ~NodeMask::noSelect;
//    if ((selectionMask & Selection::facesSelectable) != Selection::facesSelectable)
//        out &= ~NodeMask::face;
//    if ((selectionMask & Selection::edgesSelectable) != Selection::edgesSelectable)
//        out &= ~NodeMask::edge;
//    if ((selectionMask & Selection::verticesSelectable) != Selection::verticesSelectable)
//        out &= ~NodeMask::vertex;
//    emit setSelection(out);

    Q_EMIT setSelectionMask(selectionMask);
}

void Manager::popState()
{
    if (stateStack.size() < 1)
        return;
    selectionMask = stateStack.pop();
    updateToolbar();
    sendState();
}

void Manager::pushState()
{
    stateStack.push(selectionMask);
}

void Manager::startCommand(const unsigned int &stateIn)
{
    pushState();
    selectionMask = stateIn;
    updateToolbar();
    sendState();
}

void Manager::endCommand()
{
    popState();
}

void Manager::setState(const unsigned int &stateIn)
{
    selectionMask = stateIn;
    updateToolbar();
    sendState();
}

void Manager::updateToolbar()
{
    actionSelectObjects->setEnabled(slc::ObjectsEnabled & selectionMask);
    actionSelectObjects->setChecked(slc::ObjectsSelectable & selectionMask);
    actionSelectFeatures->setEnabled(slc::FeaturesEnabled & selectionMask);
    actionSelectFeatures->setChecked(slc::FeaturesSelectable & selectionMask);
    actionSelectSolids->setEnabled(slc::SolidsEnabled & selectionMask);
    actionSelectSolids->setChecked(slc::SolidsSelectable & selectionMask);
    actionSelectShells->setEnabled(slc::ShellsEnabled & selectionMask);
    actionSelectShells->setChecked(slc::ShellsSelectable & selectionMask);
    actionSelectFaces->setEnabled(slc::FacesEnabled & selectionMask);
    actionSelectFaces->setChecked(slc::FacesSelectable & selectionMask);
    actionSelectWires->setEnabled(slc::WiresEnabled & selectionMask);
    actionSelectWires->setChecked(slc::WiresSelectable & selectionMask);
    actionSelectEdges->setEnabled(slc::EdgesEnabled & selectionMask);
    actionSelectEdges->setChecked(slc::EdgesSelectable & selectionMask);
    actionSelectVertices->setEnabled(slc::PointsEnabled & selectionMask);
    actionSelectVertices->setChecked(slc::PointsSelectable & selectionMask);
    actionSelectEndPoints->setEnabled(slc::EndPointsEnabled & selectionMask);
    actionSelectEndPoints->setChecked(slc::EndPointsSelectable & selectionMask);
    actionSelectMidPoints->setEnabled(slc::MidPointsEnabled & selectionMask);
    actionSelectMidPoints->setChecked(slc::MidPointsSelectable & selectionMask);
    actionSelectCenterPoints->setEnabled(slc::CenterPointsEnabled & selectionMask);
    actionSelectCenterPoints->setChecked(slc::CenterPointsSelectable & selectionMask);
    actionSelectQuadrantPoints->setEnabled(slc::QuadrantPointsEnabled & selectionMask);
    actionSelectQuadrantPoints->setChecked(slc::QuadrantPointsSelectable & selectionMask);
    actionSelectNearestPoints->setEnabled(slc::NearestPointsEnabled & selectionMask);
    actionSelectNearestPoints->setChecked(slc::NearestPointsSelectable & selectionMask);
    actionSelectScreenPoints->setEnabled(slc::ScreenPointsEnabled & selectionMask);
    actionSelectScreenPoints->setChecked(slc::ScreenPointsSelectable & selectionMask);
}

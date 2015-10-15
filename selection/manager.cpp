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

using namespace Selection;

Manager::Manager(QObject *parent) :
    QObject(parent), selectionMask(Selection::None)
{
}

void Manager::triggeredObjects(bool objectStateIn)
{
    if (objectStateIn)
        selectionMask |= Selection::ObjectsSelectable;
    else
        selectionMask &= ~Selection::ObjectsSelectable;
    sendState();
}

void Manager::triggeredFeatures(bool featureStateIn)
{
    if (featureStateIn)
        selectionMask |= Selection::FeaturesSelectable;
    else
        selectionMask &= ~Selection::FeaturesSelectable;
    sendState();
}

void Manager::triggeredSolids(bool solidStateIn)
{
    if (solidStateIn)
        selectionMask |= Selection::SolidsSelectable;
    else
        selectionMask &= ~Selection::SolidsSelectable;
    sendState();
}

void Manager::triggeredShells(bool shellStateIn)
{
    if (shellStateIn)
        selectionMask |= Selection::ShellsSelectable;
    else
        selectionMask &= ~Selection::ShellsSelectable;
    sendState();
}

void Manager::triggeredFaces(bool faceStateIn)
{
    if (faceStateIn)
        selectionMask |= Selection::FacesSelectable;
    else
        selectionMask &= ~Selection::FacesSelectable;
    sendState();
}

void Manager::triggeredWires(bool wireStateIn)
{
    if (wireStateIn)
        selectionMask |= Selection::WiresSelectable;
    else
        selectionMask &= ~Selection::WiresSelectable;
    sendState();
}

void Manager::triggeredEdges(bool edgeStateIn)
{
    if (edgeStateIn)
        selectionMask |= Selection::EdgesSelectable;
    else
        selectionMask &= ~Selection::EdgesSelectable;
    sendState();
}

void Manager::triggeredVertices(bool vertexStateIn)
{
    if (vertexStateIn)
    {
        selectionMask |= Selection::PointsSelectable;
	selectionMask |= Selection::EndPointsEnabled;
	selectionMask |= Selection::MidPointsEnabled;
	selectionMask |= Selection::CenterPointsEnabled;
	selectionMask |= Selection::QuadrantPointsEnabled;
	selectionMask |= Selection::NearestPointsEnabled;
    }
    else
    {
        selectionMask &= ~Selection::PointsSelectable;
	selectionMask &= ~Selection::EndPointsEnabled;
	selectionMask &= ~Selection::MidPointsEnabled;
	selectionMask &= ~Selection::CenterPointsEnabled;
	selectionMask &= ~Selection::QuadrantPointsEnabled;
	selectionMask &= ~Selection::NearestPointsEnabled;
    }
    updateToolbar();
    sendState();
}

void Manager::triggeredEndPoints(bool endPointStateIn)
{
  if (endPointStateIn)
    selectionMask |= Selection::EndPointsSelectable;
  else
    selectionMask &= ~Selection::EndPointsSelectable;
  sendState();
}

void Manager::triggeredMidPoints(bool midPointStateIn)
{
  if (midPointStateIn)
    selectionMask |= Selection::MidPointsSelectable;
  else
    selectionMask &= ~Selection::MidPointsSelectable;
  sendState();
}

void Manager::triggeredCenterPoints(bool centerPointStateIn)
{
  if (centerPointStateIn)
    selectionMask |= Selection::CenterPointsSelectable;
  else
    selectionMask &= ~Selection::CenterPointsSelectable;
  sendState();

}

void Manager::triggeredQuadrantPoints(bool quadrantPointStateIn)
{
  if (quadrantPointStateIn)
    selectionMask |= Selection::QuadrantPointsSelectable;
  else
    selectionMask &= ~Selection::QuadrantPointsSelectable;
  sendState();

}

void Manager::triggeredNearestPoints(bool nearestPointStateIn)
{
  if (nearestPointStateIn)
    selectionMask |= Selection::NearestPointsSelectable;
  else
    selectionMask &= ~Selection::NearestPointsSelectable;
  sendState();
}

void Manager::triggeredScreenPoints(bool screenPointStateIn)
{
  if (screenPointStateIn)
    selectionMask |= Selection::ScreenPointsSelectable;
  else
    selectionMask &= ~Selection::ScreenPointsSelectable;
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
    actionSelectObjects->setEnabled(Selection::ObjectsEnabled & selectionMask);
    actionSelectObjects->setChecked(Selection::ObjectsSelectable & selectionMask);
    actionSelectFeatures->setEnabled(Selection::FeaturesEnabled & selectionMask);
    actionSelectFeatures->setChecked(Selection::FeaturesSelectable & selectionMask);
    actionSelectSolids->setEnabled(Selection::SolidsEnabled & selectionMask);
    actionSelectSolids->setChecked(Selection::SolidsSelectable & selectionMask);
    actionSelectShells->setEnabled(Selection::ShellsEnabled & selectionMask);
    actionSelectShells->setChecked(Selection::ShellsSelectable & selectionMask);
    actionSelectFaces->setEnabled(Selection::FacesEnabled & selectionMask);
    actionSelectFaces->setChecked(Selection::FacesSelectable & selectionMask);
    actionSelectWires->setEnabled(Selection::WiresEnabled & selectionMask);
    actionSelectWires->setChecked(Selection::WiresSelectable & selectionMask);
    actionSelectEdges->setEnabled(Selection::EdgesEnabled & selectionMask);
    actionSelectEdges->setChecked(Selection::EdgesSelectable & selectionMask);
    actionSelectVertices->setEnabled(Selection::PointsEnabled & selectionMask);
    actionSelectVertices->setChecked(Selection::PointsSelectable & selectionMask);
    actionSelectEndPoints->setEnabled(Selection::EndPointsEnabled & selectionMask);
    actionSelectEndPoints->setChecked(Selection::EndPointsSelectable & selectionMask);
    actionSelectMidPoints->setEnabled(Selection::MidPointsEnabled & selectionMask);
    actionSelectMidPoints->setChecked(Selection::MidPointsSelectable & selectionMask);
    actionSelectCenterPoints->setEnabled(Selection::CenterPointsEnabled & selectionMask);
    actionSelectCenterPoints->setChecked(Selection::CenterPointsSelectable & selectionMask);
    actionSelectQuadrantPoints->setEnabled(Selection::QuadrantPointsEnabled & selectionMask);
    actionSelectQuadrantPoints->setChecked(Selection::QuadrantPointsSelectable & selectionMask);
    actionSelectNearestPoints->setEnabled(Selection::NearestPointsEnabled & selectionMask);
    actionSelectNearestPoints->setChecked(Selection::NearestPointsSelectable & selectionMask);
    actionSelectScreenPoints->setEnabled(Selection::ScreenPointsEnabled & selectionMask);
    actionSelectScreenPoints->setChecked(Selection::ScreenPointsSelectable & selectionMask);
}

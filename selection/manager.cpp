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

#include <sstream>

#include <QAction>

#include <osg/Node>

#include <modelviz/nodemaskdefs.h>
#include <selection/definitions.h>
#include <message/dispatch.h>
#include <message/observer.h>
#include <selection/message.h>
#include <selection/manager.h>

using namespace slc;

Manager::Manager(QObject *parent) :
  QObject(parent), selectionMask(slc::AllEnabled)
{
  observer = std::move(std::unique_ptr<msg::Observer>(new msg::Observer()));
  observer->name = "slc::Manager";
  setupDispatcher();
}

Manager::~Manager() {}

void Manager::triggeredObjects(bool objectStateIn)
{
    if (objectStateIn)
        selectionMask |= slc::ObjectsSelectable;
    else
        selectionMask &= ~slc::ObjectsSelectable;
    Q_EMIT setSelectionMask(selectionMask);
    sendUpdatedMask();
}

void Manager::triggeredFeatures(bool featureStateIn)
{
    if (featureStateIn)
        selectionMask |= slc::FeaturesSelectable;
    else
        selectionMask &= ~slc::FeaturesSelectable;
    Q_EMIT setSelectionMask(selectionMask);
    sendUpdatedMask();
}

void Manager::triggeredSolids(bool solidStateIn)
{
    if (solidStateIn)
        selectionMask |= slc::SolidsSelectable;
    else
        selectionMask &= ~slc::SolidsSelectable;
    Q_EMIT setSelectionMask(selectionMask);
    sendUpdatedMask();
}

void Manager::triggeredShells(bool shellStateIn)
{
    if (shellStateIn)
        selectionMask |= slc::ShellsSelectable;
    else
        selectionMask &= ~slc::ShellsSelectable;
    Q_EMIT setSelectionMask(selectionMask);
    sendUpdatedMask();
}

void Manager::triggeredFaces(bool faceStateIn)
{
    if (faceStateIn)
        selectionMask |= slc::FacesSelectable;
    else
        selectionMask &= ~slc::FacesSelectable;
    Q_EMIT setSelectionMask(selectionMask);
    sendUpdatedMask();
}

void Manager::triggeredWires(bool wireStateIn)
{
    if (wireStateIn)
        selectionMask |= slc::WiresSelectable;
    else
        selectionMask &= ~slc::WiresSelectable;
    Q_EMIT setSelectionMask(selectionMask);
    sendUpdatedMask();
}

void Manager::triggeredEdges(bool edgeStateIn)
{
    if (edgeStateIn)
        selectionMask |= slc::EdgesSelectable;
    else
        selectionMask &= ~slc::EdgesSelectable;
    Q_EMIT setSelectionMask(selectionMask);
    sendUpdatedMask();
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
    Q_EMIT setSelectionMask(selectionMask);
    sendUpdatedMask();
}

void Manager::triggeredEndPoints(bool endPointStateIn)
{
  if (endPointStateIn)
    selectionMask |= slc::EndPointsSelectable;
  else
    selectionMask &= ~slc::EndPointsSelectable;
  Q_EMIT setSelectionMask(selectionMask);
  sendUpdatedMask();
}

void Manager::triggeredMidPoints(bool midPointStateIn)
{
  if (midPointStateIn)
    selectionMask |= slc::MidPointsSelectable;
  else
    selectionMask &= ~slc::MidPointsSelectable;
  Q_EMIT setSelectionMask(selectionMask);
  sendUpdatedMask();
}

void Manager::triggeredCenterPoints(bool centerPointStateIn)
{
  if (centerPointStateIn)
    selectionMask |= slc::CenterPointsSelectable;
  else
    selectionMask &= ~slc::CenterPointsSelectable;
  Q_EMIT setSelectionMask(selectionMask);
  sendUpdatedMask();
}

void Manager::triggeredQuadrantPoints(bool quadrantPointStateIn)
{
  if (quadrantPointStateIn)
    selectionMask |= slc::QuadrantPointsSelectable;
  else
    selectionMask &= ~slc::QuadrantPointsSelectable;
  Q_EMIT setSelectionMask(selectionMask);
  sendUpdatedMask();
}

void Manager::triggeredNearestPoints(bool nearestPointStateIn)
{
  if (nearestPointStateIn)
    selectionMask |= slc::NearestPointsSelectable;
  else
    selectionMask &= ~slc::NearestPointsSelectable;
  Q_EMIT setSelectionMask(selectionMask);
  sendUpdatedMask();
}

void Manager::triggeredScreenPoints(bool screenPointStateIn)
{
  if (screenPointStateIn)
    selectionMask |= slc::ScreenPointsSelectable;
  else
    selectionMask &= ~slc::ScreenPointsSelectable;
  Q_EMIT setSelectionMask(selectionMask);
  sendUpdatedMask();
}

void Manager::setState(const unsigned int &stateIn)
{
    selectionMask = stateIn;
    updateToolbar();
    Q_EMIT setSelectionMask(selectionMask);
    sendUpdatedMask();
}
void Manager::sendUpdatedMask()
{
  slc::Message out;
  out.selectionMask = selectionMask;
  msg::Message mOut(msg::Response | msg::Selection | msg::SetMask);
  mOut.payload = out;
  observer->messageOutSignal(mOut);
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

void Manager::setupDispatcher()
{
  msg::Mask mask;
  
  mask = msg::Request | msg::Selection | msg::SetMask;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::requestSelectionMaskDispatched, this, _1)));
}

void Manager::requestSelectionMaskDispatched(const msg::Message &messageIn)
{
  
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  slc::Message message = boost::get<slc::Message>(messageIn.payload);
  setState(message.selectionMask);
}

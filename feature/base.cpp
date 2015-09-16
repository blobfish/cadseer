/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013  tanderson <email>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <boost/uuid/random_generator.hpp>

#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>

#include "../nodemaskdefs.h"
#include "../modelviz/graph.h"
#include "../globalutilities.h"
#include "base.h"

using namespace Feature;

std::size_t Base::nextConstructionIndex = 0;

Base::Base()
{
  id = boost::uuids::basic_random_generator<boost::mt19937>()();
  constructionIndex = nextConstructionIndex;
  nextConstructionIndex++;
  
  name = QObject::tr("Empty");
  
  mainSwitch = new osg::Switch();
  mainSwitch->setNodeMask(NodeMaskDef::object);
  mainSwitch->setUserValue(GU::idAttributeTitle, GU::idToString(id));
  
  state.set(StateOffset::ModelDirty, true);
  state.set(StateOffset::VisualDirty, true);
  state.set(StateOffset::Hidden3D, false);
  state.set(StateOffset::Failure, false);
  state.set(StateOffset::Inactive, false);
}

Base::~Base()
{
}

//the visual is dependent on the model.
//so if model is set dirty so is the visual.
void Base::setModelDirty()
{
  //ensure model and visual are in sync.
  if (isModelClean())
  {
    state.set(StateOffset::ModelDirty, true);
    stateChangedSignal(id, StateOffset::ModelDirty);
  }
  if (isVisualClean())
  {
    state.set(StateOffset::VisualDirty, true);
    stateChangedSignal(id, StateOffset::VisualDirty);
  }
}

void Base::setModelClean()
{
  if (isModelClean())
    return;
  state.set(StateOffset::ModelDirty, false);
  stateChangedSignal(id, StateOffset::ModelDirty);
}

void Base::setVisualClean()
{
  if (isVisualClean())
    return;
  state.set(StateOffset::VisualDirty, false);
  stateChangedSignal(id, StateOffset::VisualDirty);
}

TopoDS_Compound Base::compoundWrap(const TopoDS_Shape& shapeIn)
{
  TopoDS_Compound compound;
  BRep_Builder builder;
  builder.MakeCompound(compound);
  builder.Add(compound, shapeIn);
  return compound;
}

void Base::updateVisual()
{
  //clear all the children from the main switch.
  mainSwitch->removeChildren(0, mainSwitch->getNumChildren());
  
  if (shape.IsNull())
    return;

  ModelViz::BuildConnector connectBuilder(shape, resultContainer);
  connector = connectBuilder.getConnector();
  connector.outputGraphviz("connectorGraph");

  ModelViz::Build builder(shape, resultContainer);
  if (builder.go(0.01, 0.05))
  {
      mainSwitch->addChild(builder.getViz().get());
  }
  
  setVisualClean();
}

void Base::show3D()
{
  assert(mainSwitch->getNumChildren());
  if (isVisible3D())
    return; //already on.
  if (isVisualDirty())
    updateVisual();
  mainSwitch->setAllChildrenOn();
  state.set(StateOffset::Hidden3D, false);
  stateChangedSignal(id, StateOffset::Hidden3D);
}

void Base::hide3D()
{
  assert(mainSwitch->getNumChildren());
  if (isHidden3D())
    return; //already off.
  mainSwitch->setAllChildrenOff();
  state.set(StateOffset::Hidden3D, true);
  stateChangedSignal(id, StateOffset::Hidden3D);
}

void Base::toggle3D()
{
  assert(mainSwitch->getNumChildren());
  if (isVisible3D())
    hide3D();
  else
    show3D();
  stateChangedSignal(id, StateOffset::Hidden3D);
}

void Base::setSuccess()
{
  if (isSuccess())
    return; //already success
  state.set(StateOffset::Failure, false);
  stateChangedSignal(id, StateOffset::Failure);
}

void Base::setFailure()
{
  if (isFailure())
    return; //already failure
  state.set(StateOffset::Failure, true);
  stateChangedSignal(id, StateOffset::Failure);
}

void Base::setActive()
{
  if (isActive())
    return; //already active.
  state.set(StateOffset::Inactive, false);
  stateChangedSignal(id, StateOffset::Inactive);
}

void Base::setInActive()
{
  if (isInactive())
    return; //already inactive.
  state.set(StateOffset::Inactive, true);
  stateChangedSignal(id, StateOffset::Inactive);
}



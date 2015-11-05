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

#include <boost/uuid/random_generator.hpp>

#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>

#include <application/application.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <nodemaskdefs.h>
#include <modelviz/graph.h>
#include <globalutilities.h>
#include <feature/base.h>


using namespace ftr;

std::size_t Base::nextConstructionIndex = 0;

Base::Base()
{
  id = boost::uuids::basic_random_generator<boost::mt19937>()();
  constructionIndex = nextConstructionIndex;
  nextConstructionIndex++;
  
  name = QObject::tr("Empty");
  
  mainSwitch = new osg::Switch();
  mainSwitch->setName("feature");
  mainSwitch->setNodeMask(NodeMaskDef::object);
  mainSwitch->setUserValue(GU::idAttributeTitle, boost::uuids::to_string(id));
  
  state.set(ftr::StateOffset::ModelDirty, true);
  state.set(ftr::StateOffset::VisualDirty, true);
  state.set(ftr::StateOffset::Hidden3D, false);
  state.set(ftr::StateOffset::Failure, false);
  state.set(ftr::StateOffset::Inactive, false);
  state.set(ftr::StateOffset::NonLeaf, false);
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
    state.set(ftr::StateOffset::ModelDirty, true);
    stateChangedSignal(id, ftr::StateOffset::ModelDirty);
  }
  setVisualDirty();
}

void Base::setModelClean()
{
  if (isModelClean())
    return;
  state.set(ftr::StateOffset::ModelDirty, false);
  stateChangedSignal(id, ftr::StateOffset::ModelDirty);
}

void Base::setVisualClean()
{
  if (isVisualClean())
    return;
  state.set(ftr::StateOffset::VisualDirty, false);
  stateChangedSignal(id, ftr::StateOffset::VisualDirty);
}

void Base::setVisualDirty()
{
  if(isVisualDirty())
    return;
  state.set(ftr::StateOffset::VisualDirty, true);
  stateChangedSignal(id, ftr::StateOffset::VisualDirty);
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

  mdv::BuildConnector connectBuilder(shape, resultContainer);
  connector = connectBuilder.getConnector();
  connector.outputGraphviz();
  
  //get deflection values.
  app::Application *app = dynamic_cast<app::Application*>(qApp);
  assert(app);
  double linear = app->getPreferencesManager()->rootPtr->visual().mesh().linearDeflection();
  double angular = app->getPreferencesManager()->rootPtr->visual().mesh().angularDeflection();

  mdv::Build builder(shape, resultContainer);
  if (builder.go(linear, angular))
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
  state.set(ftr::StateOffset::Hidden3D, false);
  stateChangedSignal(id, ftr::StateOffset::Hidden3D);
}

void Base::hide3D()
{
  assert(mainSwitch->getNumChildren());
  if (isHidden3D())
    return; //already off.
  mainSwitch->setAllChildrenOff();
  state.set(ftr::StateOffset::Hidden3D, true);
  stateChangedSignal(id, ftr::StateOffset::Hidden3D);
}

void Base::toggle3D()
{
  assert(mainSwitch->getNumChildren());
  if (isVisible3D())
    hide3D();
  else
    show3D();
  stateChangedSignal(id, ftr::StateOffset::Hidden3D);
}

void Base::setSuccess()
{
  if (isSuccess())
    return; //already success
  state.set(ftr::StateOffset::Failure, false);
  stateChangedSignal(id, ftr::StateOffset::Failure);
}

void Base::setFailure()
{
  if (isFailure())
    return; //already failure
  state.set(ftr::StateOffset::Failure, true);
  stateChangedSignal(id, ftr::StateOffset::Failure);
}

void Base::setActive()
{
  if (isActive())
    return; //already active.
  state.set(ftr::StateOffset::Inactive, false);
  stateChangedSignal(id, ftr::StateOffset::Inactive);
}

void Base::setInActive()
{
  if (isInactive())
    return; //already inactive.
  state.set(ftr::StateOffset::Inactive, true);
  stateChangedSignal(id, ftr::StateOffset::Inactive);
}

void Base::setLeaf()
{
  if (isLeaf())
    return; //already a leaf.
  state.set(ftr::StateOffset::NonLeaf, false);
  stateChangedSignal(id, ftr::StateOffset::NonLeaf);
}

void Base::setNonLeaf()
{
  if (isNonLeaf())
    return; //already nonLeaf.
  state.set(ftr::StateOffset::NonLeaf, true);
  stateChangedSignal(id, ftr::StateOffset::NonLeaf);
}

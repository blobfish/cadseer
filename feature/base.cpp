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

Base::Base()
{
  id = boost::uuids::basic_random_generator<boost::mt19937>()();
  
  mainSwitch = new osg::Switch();
  mainSwitch->setNodeMask(NodeMask::object);
  mainSwitch->setUserValue(GU::idAttributeTitle, GU::idToString(id));
}

Base::~Base()
{
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

/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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

#include <BOPAlgo_Builder.hxx>
#include <BOPDS_DS.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

#include <globalutilities.h>
#include <feature/base.h>
#include <feature/booleanidmapper.h>

using namespace ftr;

BooleanIdMapper::BooleanIdMapper
(
  const UpdateMap& updateMapIn,
  const BOPAlgo_Builder &builderIn,
  ResultContainerWrapper &outContainerIn 
):
updateMap(updateMapIn),
builder(builderIn),
outContainer(outContainerIn)
{

}

void BooleanIdMapper::go()
{
  //we are assuming that the outContainer has all the shapes added
  //and the ids, that can be, have been set by the 'general'
  //shape id mapper function.
  
  ResultContainer &work = outContainer.container;
  const BOPCol_DataMapOfShapeShape &origins = builder.Origins();
  
  gu::ShapeVector shapeVector;
  typedef ResultContainer::index<ResultRecord::ById>::type List;
  List &list = work.get<ResultRecord::ById>();
  auto rangeItPair = list.equal_range(boost::uuids::nil_generator()());
  for (; rangeItPair.first != rangeItPair.second; ++rangeItPair.first)
  {
    if (origins.IsBound(rangeItPair.first->shape))
      shapeVector.push_back(rangeItPair.first->shape);
  }
  
  const ResultContainer &targetContainer = updateMap.find(ftr::InputTypes::target)->second->getResultContainer();
  const ResultContainer &toolContainer = updateMap.find(ftr::InputTypes::tool)->second->getResultContainer();
  for (const auto &shape : shapeVector)
  {
    if (hasResult(targetContainer, origins(shape)))
    {
      updateId(work, findResultByShape(targetContainer, origins(shape)).id, shape);
    }
    else if(hasResult(toolContainer, origins(shape)))
    {
      updateId(work, findResultByShape(toolContainer, origins(shape)).id, shape);
    }
  }
}

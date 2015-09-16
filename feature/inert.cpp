/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2015  tanderson <email>
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

#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

#include "inert.h"

using namespace Feature;
using namespace boost::uuids;

QIcon Inert::icon;

Inert::Inert(const TopoDS_Shape &shapeIn)
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionInert.svg");
  
  name = QObject::tr("Inert");
  
  if (shapeIn.ShapeType() == TopAbs_COMPOUND)
    shape = shapeIn;
  else
    shape = compoundWrap(shapeIn);
  
  TopTools_IndexedMapOfShape shapeMap;
  TopExp::MapShapes(shape, shapeMap);
  
  for (int index = 1; index <= shapeMap.Extent(); ++index)
  {
    uuid tempId = basic_random_generator<boost::mt19937>()();
    
    ResultRecord record;
    record.id = tempId;
    record.shape = shapeMap(index);
    resultContainer.insert(record);
    
    EvolutionRecord evolutionRecord;
    evolutionRecord.outId = tempId;
    evolutionContainer.insert(evolutionRecord);
  }
  
  //not using feature container;
}

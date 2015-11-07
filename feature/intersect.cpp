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

#include <BRepAlgoAPI_Common.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

#include "intersect.h"

using namespace ftr;

QIcon Intersect::icon;

Intersect::Intersect() : Base()
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionIntersect.svg");
  
  name = QObject::tr("Intersect");
}

void Intersect::updateModel(const UpdateMap &mapIn)
{
  shape = TopoDS_Shape();
  setFailure(); //assume failure until success.
  
  try
  {
    if
    (
      mapIn.count(InputTypes::target) != 1 ||
      mapIn.count(InputTypes::tool) < 1
    )
    {
      //we should have a status message in the base class.
      std::ostringstream stream;
      stream << "wrong number of arguments.   " <<
        "target count is: " << mapIn.count(InputTypes::target) << "   " <<
        "tool count is: " << mapIn.count(InputTypes::tool);
      throw std::runtime_error(stream.str());
    }
      
    //UpdateMap is going to have to be a multimap for multiple tools.
    const TopoDS_Shape &targetShape = mapIn.at(InputTypes::target)->getShape();
    const TopoDS_Shape &toolShape = mapIn.at(InputTypes::tool)->getShape();
    assert(!targetShape.IsNull() && !toolShape.IsNull());
    
    BRepAlgoAPI_Common commoner(targetShape, toolShape);
    if (!commoner.IsDone())
      throw std::runtime_error("OCC Common failed");
    
    shape = commoner.Shape();
    
    
    
    //this is just temp so that the viz generation works.
    //this will have to be completely reworked to follow id scheme.
    TopTools_IndexedMapOfShape shapeMap;
    TopExp::MapShapes(shape, shapeMap);
    
    for (int index = 1; index <= shapeMap.Extent(); ++index)
    {
      boost::uuids::uuid tempId = boost::uuids::basic_random_generator<boost::mt19937>()();
      
      ResultRecord record;
      record.id = tempId;
      record.shape = shapeMap(index);
      resultContainer.insert(record);
      
      EvolutionRecord evolutionRecord;
      evolutionRecord.outId = tempId;
      evolutionContainer.insert(evolutionRecord);
    }
    
    
    
    setModelClean();
    setSuccess();
  }
  catch (Standard_Failure)
  {
    Handle_Standard_Failure e = Standard_Failure::Caught();
    std::cout << std::endl << "Error in union update. " << e->GetMessageString() << std::endl;
  }
  catch (std::exception &e)
  {
    std::cout << std::endl << "Error in union update. " << e.what() << std::endl;
  }
}
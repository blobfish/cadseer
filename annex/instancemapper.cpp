/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2018  Thomas S. Anderson blobfish.at.gmx.com
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

#include <iostream>

#include <tools/idtools.h>
#include <tools/occtools.h>
#include <feature/shapehistory.h>
#include <annex/seershape.h>
#include <annex/instancemapper.h>

using boost::uuids::uuid;
namespace BMI = boost::multi_index;

using namespace ann;


/** @brief data for mapper
* 
* When we have a new shape to map, we store the shapes history
* in the history vector and then make an entry in the map container
*/
struct InstanceMapper::Data
{
  //maps the history to the instance out ids.
  struct HistoryOut
  {
    HistoryOut() = delete;
    HistoryOut(const ftr::ShapeHistory &i){history = i;}
    ftr::ShapeHistory history;
    bool used = false;
    std::vector<uuid> outIds; //position in vector is the instance number.
  };
  std::vector<HistoryOut> historyOuts;
  std::vector<std::size_t> IndexHistories; //parallel with shapes. IndexHistories.at(0) is for shape 0. etc..
  std::vector<uuid> sourceIds; //used to fill in evolution container.
  uuid getOutId(std::size_t shapeIndex, std::size_t instance)
  {
    //we have already checked to make sure shapeIndex is the same size as IndexHistories.
    HistoryOut &ho = historyOuts.at(IndexHistories.at(shapeIndex));
    while (ho.outIds.size() <= instance)
      ho.outIds.push_back(gu::createRandomId());
    return ho.outIds.at(instance);
  }
};

InstanceMapper::InstanceMapper() : Base(), data(new Data()){}

InstanceMapper::~InstanceMapper(){}

void InstanceMapper::startMapping(const SeerShape &sShapeIn, const uuid &idIn, const ftr::ShapeHistory &shapeHistory)
{
  if (!idIn.is_nil())
  {
    assert(sShapeIn.hasShapeIdRecord(idIn));
    if (!sShapeIn.hasShapeIdRecord(idIn))
    {
      std::cerr << "WARNING: seershape doesn't have shape with id passed in: " << gu::idToString(idIn) << std::endl;
      return;
    }
  }
  
  for (auto &sih : data->historyOuts)
    sih.used = false;
  
  data->sourceIds.clear();
  
  data->IndexHistories.clear();
  occt::ShapeVector shapes;
  if (idIn.is_nil())
    shapes = occt::mapShapes(sShapeIn.getRootOCCTShape());
  else
  {
    shapes.push_back(sShapeIn.getOCCTShape(idIn));
    occt::ShapeVector subShapes = occt::mapShapes(shapes.back());
    std::copy(subShapes.begin(), subShapes.end(), std::back_inserter(shapes));
  }
  for (const auto &s : shapes)
  {
    uuid id = sShapeIn.findShapeIdRecord(s).id;
    bool foundHistoryMatch = false;
    std::size_t historyCount = 0;
    for (auto &ho : data->historyOuts)
    {
      if (ho.history.hasShape(id) && (!ho.used))
      {
        foundHistoryMatch = true;
        ho.used = true;
        data->IndexHistories.push_back(historyCount);
        break;
      }
      historyCount++;
    }
    if (!foundHistoryMatch)
    {
      Data::HistoryOut nh(shapeHistory.createDevolveHistory(id));
      nh.used = true;
      data->IndexHistories.push_back(data->historyOuts.size());
      data->historyOuts.push_back(nh);
    }
    data->sourceIds.push_back(id);
  }
}

void InstanceMapper::mapIndex(SeerShape &sShape, const TopoDS_Shape &dsShape, std::size_t instance)
{
  occt::ShapeVector shapes;
  if (dsShape.ShapeType() != TopAbs_COMPOUND)
    shapes.push_back(dsShape);
  occt::ShapeVector subShapes = occt::mapShapes(dsShape);
  std::copy(subShapes.begin(), subShapes.end(), std::back_inserter(shapes));
  
  assert(data->IndexHistories.size() == shapes.size());
  if (data->IndexHistories.size() != shapes.size())
  {
    std::cerr << "WARNING: wrong number of shapes to map vs initialized shape in InstanceMapper::mapIndex" << std::endl;
    return;
  }
  
  assert(data->sourceIds.size() == shapes.size());
  std::size_t shapeCount = 0;
  for (const auto &s : shapes)
  {
    uuid outId = data->getOutId(shapeCount, instance);
    sShape.updateShapeIdRecord(s, outId);
    sShape.insertEvolve(data->sourceIds.at(shapeCount), outId);
    shapeCount++;
  }
}

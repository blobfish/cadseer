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
#include <BOPTools_AlgoTools.hxx>
#include <BOPTools.hxx>
#include <BOPDS_DS.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopTools_MapOfShape.hxx>
#include <BRepTools.hxx>

#include <globalutilities.h>
#include <feature/base.h>
#include <feature/intersectionmapping.h>
#include <feature/booleanidmapper.h>

using namespace ftr;
using boost::uuids::uuid;

static boost::uuids::basic_random_generator<boost::mt19937> idGenerator;

BooleanIdMapper::BooleanIdMapper
(
  const UpdateMap& updateMapIn,
  BOPAlgo_Builder &builderIn,
  IMapWrapper &iMapWrapperIn,
  ResultContainerWrapper &outContainerIn 
):
updateMap(updateMapIn),
builder(builderIn),
iMapWrapper(iMapWrapperIn),
outContainer(outContainerIn)
{
  //result containers are expected to have 1 compound root shape.
  for (const auto &c : outContainer.container)
  {
    if (c.shape.ShapeType() == TopAbs_COMPOUND)
    {
      rootShape = c.shape;
      break;
    }
  }
  assert(!rootShape.IsNull());
}

void BooleanIdMapper::go()
{
  //we are assuming that the outContainer has all the shapes added
  //and the ids, that can be, have been set by the 'general'
  //shape id mapper function.
  
  goSingleSplits();
  goIntersectionEdges();
  goSplitFaces();
  
//   std::cout << std::endl << "iEdges: " << std::endl;
//   for (const auto &edge : iMapWrapper.intersectionEdges)
//   {
//     std::cout << edge << std::endl;
//   }
  
}

void BooleanIdMapper::goIntersectionEdges()
{
  ResultContainer &work = outContainer.container;
  const ResultContainer &targetContainer = updateMap.find(ftr::InputTypes::target)->second->getResultContainer();
  const ResultContainer &toolContainer = updateMap.find(ftr::InputTypes::tool)->second->getResultContainer();
  const BOPDS_DS &bopDS = *(builder.PDS());
  const BOPCol_DataMapOfShapeShape &origins = builder.Origins();
  boost::uuids::nil_generator ng;
  using boost::uuids::to_string;
  using boost::uuids::uuid;
  
  TopTools_IndexedDataMapOfShapeListOfShape eToF; //edges to faces
  TopExp::MapShapesAndAncestors(rootShape, TopAbs_EDGE, TopAbs_FACE, eToF);
  TopTools_IndexedDataMapOfShapeListOfShape vToF; //vertices to faces
  TopExp::MapShapesAndAncestors(rootShape, TopAbs_VERTEX, TopAbs_FACE, vToF);
  std::set<IntersectionEdge> cIEdges; //current intersection edges.
  for (int index = 0; index < bopDS.NbShapes(); ++index)
  {
    if (!bopDS.IsNewShape(index))
      continue;
    const TopoDS_Shape &shape = bopDS.Shape(index);
    if (shape.ShapeType() != TopAbs_EDGE)
      continue;
    if (!hasResult(work, shape))
      continue;
    if (!findResultByShape(work, shape).id.is_nil())
      continue;
    
    IntersectionEdge tempIntersection;
    const TopTools_ListOfShape &parents = eToF.FindFromKey(shape);
    TopTools_ListIteratorOfListOfShape pIt;
    for (pIt.Initialize(parents); pIt.More(); pIt.Next())
    {
      if (!origins.IsBound(pIt.Value()))
	continue;
      
      uuid sourceFaceId = ng();
      if (hasResult(targetContainer, origins(pIt.Value())))
	sourceFaceId = findResultByShape(targetContainer, origins(pIt.Value())).id;
      if (hasResult(toolContainer, origins(pIt.Value())))
	sourceFaceId = findResultByShape(toolContainer, origins(pIt.Value())).id;
      if (sourceFaceId.is_nil())
	continue;
      
      tempIntersection.faces.insert(sourceFaceId);
    }
    if (tempIntersection.faces.size() != 2)
      continue;
    
    auto it = cIEdges.find(tempIntersection);
    if (it != cIEdges.end())
    {
      //TODO push this to some list to keep track of intersections that result in multiple edges.
      cIEdges.erase(it);
      continue;
    }
    
    const auto itt = iMapWrapper.intersectionEdges.find(tempIntersection);
    if (itt != iMapWrapper.intersectionEdges.end())
    {
      tempIntersection.resultEdge = itt->resultEdge;
    }
    else
    {
      tempIntersection.resultEdge = idGenerator();
      iMapWrapper.intersectionEdges.insert(tempIntersection);
    }
    updateId(work, tempIntersection.resultEdge, shape);
    iEdgeCache.insert(tempIntersection.resultEdge);
    cIEdges.insert(tempIntersection);
  }
}

void BooleanIdMapper::goSingleSplits()
{
  //it appears splits only contain faces that have been split. It ignores edges.
  //key is in original solid. values maybe in the output solid.
  
  ResultContainer &work = outContainer.container;
  const ResultContainer &targetContainer = updateMap.find(ftr::InputTypes::target)->second->getResultContainer();
  const ResultContainer &toolContainer = updateMap.find(ftr::InputTypes::tool)->second->getResultContainer();
  
  //loop through splits and find occurences where a face is split
  //but has only one of it's split faces in the output shape.
  const BOPCol_DataMapOfShapeListOfShape &splits = builder.Splits();
  BOPCol_DataMapOfShapeListOfShape::Iterator splitIt(splits);
  for (; splitIt.More(); splitIt.Next())
  {
    const TopoDS_Shape &key = splitIt.Key();
    
    const BOPCol_ListOfShape &shapeList = splitIt.Value();
    BOPCol_ListOfShape::Iterator shapeListIt(shapeList);
    TopoDS_Shape value;
    std::size_t count = 0;
    for (;shapeListIt.More();shapeListIt.Next())
    {
      if (hasResult(work, shapeListIt.Value()))
      {
	count++;
	value = shapeListIt.Value();
      }
    }
    if (count != 1)
      continue;
    
    //assign face id.
    uuid faceId = boost::uuids::nil_generator()();
    if (hasResult(targetContainer, key))
      faceId = findResultByShape(targetContainer, key).id;
    else if(hasResult(toolContainer, key))
      faceId = findResultByShape(toolContainer, key).id;
    assert(!faceId.is_nil());
    assert(hasResult(work, value));
    updateId(work, faceId, value);
    
    //i could assign the outside wireid here, but should happen in general shape mapping
  }
}

void BooleanIdMapper::goSplitFaces()
{
  ResultContainer &work = outContainer.container;
  const ResultContainer &targetContainer = updateMap.find(ftr::InputTypes::target)->second->getResultContainer();
  const ResultContainer &toolContainer = updateMap.find(ftr::InputTypes::tool)->second->getResultContainer();
  const BOPCol_DataMapOfShapeShape &origins = builder.Origins();
  const BOPCol_DataMapOfShapeListOfShape &splits = builder.Splits();
  
  gu::ShapeVector nilShapes;
  typedef ResultContainer::index<ResultRecord::ById>::type List;
  const List &list = work.get<ResultRecord::ById>();
  auto rangeItPair = list.equal_range(boost::uuids::nil_generator()());
  for (; rangeItPair.first != rangeItPair.second; ++rangeItPair.first)
    nilShapes.push_back(rangeItPair.first->shape);
  
  std::vector<std::pair<SplitFace, TopoDS_Shape> > weakSplits;
  
  for (const auto &currentShape : nilShapes)
  {
    if (currentShape.ShapeType() != TopAbs_FACE)
      continue;
    if (!origins.IsBound(currentShape))
      continue;
    const TopoDS_Shape &origin = origins(currentShape);
    if (!splits.IsBound(origin))
      continue;
    //now we should have proved out current shape is a split.
    
    uuid originId = boost::uuids::nil_generator()();
    if (hasResult(targetContainer, origin))
      originId = findResultByShape(targetContainer, origin).id;
    else if (hasResult(toolContainer, origin))
      originId = findResultByShape(toolContainer, origin).id;
    else
      assert(0); //no origin id in input containers.
      
    //TODO we will need the origin id and the split id into evolution container.
    
    //find the edges of current shape that are intersection edges.
    std::set<uuid> faceIEdges;
    BOPCol_IndexedMapOfShape edges;
    BOPTools::MapShapes(currentShape, TopAbs_EDGE, edges);
    for (int index = 1; index <= edges.Extent(); ++index)
    {
      assert(hasResult(work, edges(index)));
      uuid tempId = findResultByShape(work, edges(index)).id;
      if (iEdgeCache.count(tempId) == 0)
	continue;
      faceIEdges.insert(tempId);
    }
    
    SplitFace splitFace;
    splitFace.sourceFace = originId;
    splitFace.edges = faceIEdges;
    splitFace.resultFace = idGenerator();
    splitFace.resultWire = idGenerator();
    
    bool results;
    SplitFace splitFaceOut;
    std::tie(splitFaceOut, results) = iMapWrapper.matchStrong(splitFace);
    if (results)
    {
      //have a strong match.
      assert(!hasResult(work, splitFaceOut.resultFace));
      updateId(work, splitFaceOut.resultFace, currentShape);
      
      assert(!hasResult(work, splitFaceOut.resultWire));
      TopoDS_Shape outerWire = BRepTools::OuterWire(TopoDS::Face(currentShape));
      assert(!outerWire.IsNull());
      assert(hasResult(work, outerWire));
      updateId(work, splitFaceOut.resultWire, outerWire);
    }
    else
      weakSplits.push_back(std::make_pair(splitFace, currentShape));
  }
  
  //we are still constrained by the order of the iteration.
  //we have made it better by ording the output, but equal match
  //counts will be decided by order of traversal.
  for (const auto &sFace : weakSplits)
  {
    bool foundMatch = false;
    auto matches = iMapWrapper.matchWeak(sFace.first);
    for (const auto &match : matches)
    {
      if (!hasResult(work, match.resultFace))
      {
	updateId(work, match.resultFace, sFace.second);
	
	assert(!hasResult(work, match.resultWire));
	TopoDS_Shape outerWire = BRepTools::OuterWire(TopoDS::Face(sFace.second));
	assert(!outerWire.IsNull());
	assert(hasResult(work, outerWire));
	updateId(work, match.resultWire, outerWire);
	foundMatch = true;
	break;
      }
    }
    if (foundMatch)
      continue;
    iMapWrapper.add(sFace.first);
    updateId(work, sFace.first.resultFace, sFace.second);
    
    assert(!hasResult(work, sFace.first.resultWire));
    TopoDS_Shape outerWire = BRepTools::OuterWire(TopoDS::Face(sFace.second));
    assert(!outerWire.IsNull());
    assert(hasResult(work, outerWire));
    updateId(work, sFace.first.resultWire, outerWire);
  }
}

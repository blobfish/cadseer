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
#include <feature/seershape.h>
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
  SeerShape *seerShapeOutIn 
):
updateMap(updateMapIn),
builder(builderIn),
iMapWrapper(iMapWrapperIn),
seerShapeOut(seerShapeOutIn),
inputTarget(updateMap.find(ftr::InputTypes::target)->second->getSeerShape()),
inputTool(updateMap.find(ftr::InputTypes::tool)->second->getSeerShape())
{
  assert(!inputTarget.isNull());
  assert(!inputTool.isNull());
  assert(!seerShapeOut->isNull());
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
  const BOPDS_DS &bopDS = *(builder.PDS());
  const BOPCol_DataMapOfShapeShape &origins = builder.Origins();
  boost::uuids::nil_generator ng;
  using boost::uuids::to_string;
  using boost::uuids::uuid;
  
  std::set<IntersectionEdge> cIEdges; //current intersection edges.
  for (int index = 0; index < bopDS.NbShapes(); ++index)
  {
    if (!bopDS.IsNewShape(index))
      continue;
    const TopoDS_Shape &shape = bopDS.Shape(index);
    if (shape.ShapeType() != TopAbs_EDGE)
      continue;
    if (!seerShapeOut->hasShapeIdRecord(shape))
      continue;
    if (!seerShapeOut->findShapeIdRecord(shape).id.is_nil())
      continue;
    
    IntersectionEdge tempIntersection;
    gu::ShapeVector parentShapes = seerShapeOut->useGetParentsOfType(shape, TopAbs_FACE);
    
    for (const auto &cShape : parentShapes)
    {
      if (!origins.IsBound(cShape))
	continue;
      
      uuid sourceFaceId = ng();
      if (inputTarget.hasShapeIdRecord(origins(cShape)))
	sourceFaceId = inputTarget.findShapeIdRecord(origins(cShape)).id;
      if (inputTool.hasShapeIdRecord(origins(cShape)))
	sourceFaceId = inputTool.findShapeIdRecord(origins(cShape)).id;
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
    seerShapeOut->updateShapeIdRecord(shape, tempIntersection.resultEdge);
    iEdgeCache.insert(tempIntersection.resultEdge);
    cIEdges.insert(tempIntersection);
  }
}

void BooleanIdMapper::goSingleSplits()
{
  //it appears splits only contain faces that have been split. It ignores edges.
  //key is in original solid. values maybe in the output solid.
  
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
      if (seerShapeOut->hasShapeIdRecord(shapeListIt.Value()))
      {
	count++;
	value = shapeListIt.Value();
      }
    }
    if (count != 1)
      continue;
    
    //assign face id.
    uuid faceId = boost::uuids::nil_generator()();
    if (inputTarget.hasShapeIdRecord(key))
      faceId = inputTarget.findShapeIdRecord(key).id;
    else if(inputTool.hasShapeIdRecord(key))
      faceId = inputTool.findShapeIdRecord(key).id;
    assert(!faceId.is_nil());
    assert(seerShapeOut->hasShapeIdRecord(value));
    seerShapeOut->updateShapeIdRecord(value, faceId);
    
    //i could assign the outside wireid here, but should happen in general shape mapping
  }
}

void BooleanIdMapper::goSplitFaces()
{
  const BOPCol_DataMapOfShapeShape &origins = builder.Origins();
  const BOPCol_DataMapOfShapeListOfShape &splits = builder.Splits();
  
  gu::ShapeVector nilShapes = seerShapeOut->getAllNilShapes();
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
    if (inputTarget.hasShapeIdRecord(origin))
      originId = inputTarget.findShapeIdRecord(origin).id;
    else if (inputTool.hasShapeIdRecord(origin))
      originId = inputTool.findShapeIdRecord(origin).id;
    else
      assert(0); //no origin id in input containers.
      
    //TODO we will need the origin id and the split id into evolution container.
    
    //find the edges of current shape that are intersection edges.
    std::set<uuid> faceIEdges;
    BOPCol_IndexedMapOfShape edges;
    BOPTools::MapShapes(currentShape, TopAbs_EDGE, edges);
    for (int index = 1; index <= edges.Extent(); ++index)
    {
      assert(seerShapeOut->hasShapeIdRecord(edges(index)));
      uuid tempId = seerShapeOut->findShapeIdRecord(edges(index)).id;
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
      assert(!seerShapeOut->hasShapeIdRecord(splitFaceOut.resultFace));
      seerShapeOut->updateShapeIdRecord(currentShape, splitFaceOut.resultFace);
      
      assert(!seerShapeOut->hasShapeIdRecord(splitFaceOut.resultWire));
      TopoDS_Shape outerWire = BRepTools::OuterWire(TopoDS::Face(currentShape));
      assert(!outerWire.IsNull());
      assert(seerShapeOut->hasShapeIdRecord(outerWire));
      seerShapeOut->updateShapeIdRecord(outerWire, splitFaceOut.resultWire);
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
      if (!seerShapeOut->hasShapeIdRecord(match.resultFace))
      {
	seerShapeOut->updateShapeIdRecord(sFace.second, match.resultFace);
	
	assert(!seerShapeOut->hasShapeIdRecord(match.resultWire));
	TopoDS_Shape outerWire = BRepTools::OuterWire(TopoDS::Face(sFace.second));
	assert(!outerWire.IsNull());
	assert(seerShapeOut->hasShapeIdRecord(outerWire));
	seerShapeOut->updateShapeIdRecord(outerWire, match.resultWire);
	foundMatch = true;
	break;
      }
    }
    if (foundMatch)
      continue;
    iMapWrapper.add(sFace.first);
    seerShapeOut->updateShapeIdRecord(sFace.second, sFace.first.resultFace);
    
    assert(!seerShapeOut->hasShapeIdRecord(sFace.first.resultWire));
    TopoDS_Shape outerWire = BRepTools::OuterWire(TopoDS::Face(sFace.second));
    assert(!outerWire.IsNull());
    assert(seerShapeOut->hasShapeIdRecord(outerWire));
    seerShapeOut->updateShapeIdRecord(outerWire, sFace.first.resultWire);
  }
}

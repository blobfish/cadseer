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

#include <assert.h>

#include <boost/uuid/random_generator.hpp>

#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <TopExp.hxx> //maybe temp.
#include <TopTools_IndexedMapOfShape.hxx> //maybe temp.
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <BRepTools.hxx>

#include "../globalutilities.h"
#include "blend.h"

using namespace Feature;
using boost::uuids::uuid;

QIcon Blend::icon;

Blend::Blend() : Base(), radius(1.0)
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionBlend.svg");
  
  name = QObject::tr("Blend");
}

void Blend::setRadius(const double& radiusIn)
{
  if (radius == radiusIn)
    return;
  assert(radiusIn > Precision::Confusion());
  setModelDirty();
  radius = radiusIn;
}

void Blend::setEdgeIds(const std::vector<boost::uuids::uuid>& edgeIdsIn)
{
  edgeIds = edgeIdsIn;
  setModelDirty();
}

void Blend::update(const UpdateMap& mapIn)
{
  //clear shape so if we fail the feature will be empty. what about maps?
  shape = TopoDS_Shape();
  setFailure(); //assume failure until succes.
  
  if (mapIn.count(InputTypes::target) < 1)
    return; //much better error handeling.
  
  const TopoDS_Shape &targetShape = mapIn.at(InputTypes::target)->getShape();
  const ResultContainer& targetResultContainer = mapIn.at(InputTypes::target)->getResultContainer();
  
  try
  {
    BRepFilletAPI_MakeFillet blendMaker(targetShape);
    for (const auto &currentId : edgeIds)
    {
      TopoDS_Shape tempShape = findResultById(targetResultContainer, currentId).shape;
      assert(!tempShape.IsNull());
      assert(tempShape.ShapeType() == TopAbs_EDGE);
      blendMaker.Add(radius, TopoDS::Edge(tempShape));
    }
    blendMaker.Build();
    if (!blendMaker.IsDone())
      return;
    shape = blendMaker.Shape();
//     dumpInfo(blendMaker, mapIn.at(InputTypes::target));
//     std::cout << std::endl << "target result container:" << std::endl << targetResultContainer << std::endl;
    shapeMatch(mapIn.at(InputTypes::target));
//     std::cout << std::endl << "after shape match:"; dumpResultStats();
    modifiedMatch(blendMaker, mapIn.at(InputTypes::target));
//     std::cout << std::endl << "after modified match:"; dumpResultStats();
    generatedMatch(blendMaker, mapIn.at(InputTypes::target));
//     std::cout << std::endl << "after generated match:"; dumpResultStats();
//     std::cout << std::endl << "edgeToFaceMap: " << std::endl << shapeMap << std::endl;
    uniqueTypeMatch(mapIn.at(InputTypes::target));
//     std::cout << std::endl << "after unique type match:"; dumpResultStats();
    outerWireMatch(mapIn.at(InputTypes::target));
//     std::cout << std::endl << "after outer wire match:"; dumpResultStats();
    derivedMatch(blendMaker, mapIn.at(InputTypes::target));
//     std::cout << std::endl << "after derived match:"; dumpResultStats();
    
    dumpResultStats(); //only if there are shapes with nil ids.
    
    //at this point the only thing left should be new edges and vertices created by
    //the blend feature. We will use derivedContainer to map these to known faces.
    setModelClean();
    setSuccess();
  }
  catch (Standard_Failure)
  {
    Handle_Standard_Failure e = Standard_Failure::Caught();
    std::cout << std::endl << "Error in cylinder update. " << e->GetMessageString() << std::endl;
  }
}

/* assigns ids by simply matching on shapes between
 * the target container shapes and this container shapes.
 * this works through the TopoDS_Shape hashes */
void Blend::shapeMatch(const Base *targetFeatureIn)
{
  resultContainer.clear();
  
  const ResultContainer &targetResultContainer = targetFeatureIn->getResultContainer();
  
  TopTools_IndexedMapOfShape freshShapeMap;
  TopExp::MapShapes(shape, freshShapeMap);
//   std::cout << std::endl << "freshShapeMap size is: " << freshShapeMap.Extent() << std::endl;
  for (int index = 1; index <= freshShapeMap.Extent(); ++index)
  {
    const TopoDS_Shape &currentShape = freshShapeMap(index);
    
    //if the shape can be found in parent get it's id otherwise the id will be nil.
    ResultRecord record;
    if (Feature::hasResult(targetResultContainer, currentShape))
    {
      record.id = Feature::findResultByShape(targetResultContainer, currentShape).id;
    }
    record.shape = currentShape;
    resultContainer.insert(record);
    
//     EvolutionRecord evolutionRecord;
//     evolutionRecord.outId = tempId;
//     evolutionContainer.insert(evolutionRecord);
    
//     uuid tempId = boost::uuids::basic_random_generator<boost::mt19937>()();
    
  }
//   std::cout << std::endl << "resultContainer after shapeMatch" << std::endl << resultContainer << std::endl;
}

/* matching by interogating the MakeFillet objects modfied method */
void Blend::modifiedMatch(BRepFilletAPI_MakeFillet &blendMakerIn, const Base *targetFeatureIn)
{
  const ResultContainer &targetResultContainer = targetFeatureIn->getResultContainer();

  const TopoDS_Shape &targetShape = targetFeatureIn->getShape();
  TopTools_IndexedMapOfShape targetShapeMap;
  TopExp::MapShapes(targetShape, targetShapeMap);
  for (int index = 1; index <= targetShapeMap.Extent(); ++index)
  {
    const TopoDS_Shape &currentShape = targetShapeMap(index);
    //generated will be in feature map.
    const TopTools_ListOfShape &modifiedList = blendMakerIn.Modified(currentShape);
    if (!modifiedList.IsEmpty())
    {
      TopTools_ListIteratorOfListOfShape it(modifiedList);
      for(;it.More(); it.Next())
      {
        const TopoDS_Shape &listShape = it.Value();
        uuid targetId = Feature::findResultByShape(targetResultContainer, currentShape).id;
        assert(Feature::hasResult(resultContainer, listShape));
        Feature::updateId(resultContainer, targetId, listShape);
      }
    }
  }
//   std::cout << std::endl << "result after match on modified list" << std::endl << resultContainer << std::endl;
}

/* matching by interogating the MakeFillet objects generated method.
 * generated only outputs the fillet faces from the edge. Nothing else.
 * tried iterating over MakeFillet contours and edges, but in certain situations
 * blends are made from vertices. blending 3 edges of a box corner is such a situation.
 * so we will iterate over all the shapes of the targe object.
 */
void Blend::generatedMatch(BRepFilletAPI_MakeFillet &blendMakerIn, const Base *targetFeatureIn)
{
  TopTools_IndexedMapOfShape targetShapeMap;
  TopExp::MapShapes(targetFeatureIn->getShape(), targetShapeMap);
  
  for (int index = 1; index <= targetShapeMap.Extent(); ++index)
  {
    const TopoDS_Shape &currentShape = targetShapeMap(index);
    TopTools_ListOfShape generated = blendMakerIn.Generated(currentShape);
    if (generated.IsEmpty())
      continue;
    assert(generated.Extent() == 1);
    const TopoDS_Shape &blendedFace = generated.First();
    assert(!blendedFace.IsNull());
    assert(blendedFace.ShapeType() == TopAbs_FACE);
    
    //targetEdgeId should also be in member edgeIds
    uuid targetEdgeId = Feature::findResultByShape(targetFeatureIn->getResultContainer(), currentShape).id;
    uuid blendedFaceId = boost::uuids::nil_generator()();
    //first time edge has been blended
    if (!Feature::hasInId(shapeMap, targetEdgeId))
    {
      //build new record.
      EvolutionRecord record;
      record.inId = targetEdgeId;
      record.outId = boost::uuids::basic_random_generator<boost::mt19937>()();
      shapeMap.insert(record);
      
      blendedFaceId = record.outId;
    }
    else
      blendedFaceId = Feature::findRecordByIn(shapeMap, targetEdgeId).outId;
    
    //now we have the id for the face, just update the result map.
    Feature::updateId(resultContainer, blendedFaceId, blendedFace);
    
    //now look for outerwire for newly generated face.
    uuid blendedFaceWireId = boost::uuids::nil_generator()();
    if (!Feature::hasInId(shapeMap, blendedFaceId))
    {
      //this means that the face id is in both columns.
      EvolutionRecord record;
      record.inId = blendedFaceId;
      record.outId = boost::uuids::basic_random_generator<boost::mt19937>()();
      shapeMap.insert(record);
      
      blendedFaceWireId = record.outId;
    }
    else
      blendedFaceWireId = Feature::findRecordByIn(shapeMap, blendedFaceId).outId;
    
    //now get the wire and update the result to id.
    const TopoDS_Shape &blendedFaceWire = BRepTools::OuterWire(TopoDS::Face(blendedFace));
    Feature::updateId(resultContainer, blendedFaceWireId, blendedFaceWire);
  }
}

/* matches shape types between the target result container
 * and "this" result container. There can only be one shape
 * of the type in each container to be considered a match
 */
void Blend::uniqueTypeMatch(const Base *targetFeatureIn)
{
  static const std::vector<TopAbs_ShapeEnum> searchTypes
  {
    TopAbs_COMPOUND,
    TopAbs_COMPSOLID,
    TopAbs_SOLID,
    TopAbs_SHELL,
    TopAbs_FACE,
    TopAbs_WIRE,
    TopAbs_EDGE,
    TopAbs_VERTEX
  };
  
  //returns id of the only occurence of shapetype. else nil id.
  auto getUniqueRecord = [](const ResultContainer &containerIn, TopAbs_ShapeEnum shapeTypeIn)
  {
    std::size_t count = 0;
    ResultRecord outRecord;
    
    for (const auto &record : containerIn)
    {
      if (record.shape.ShapeType() == shapeTypeIn)
      {
        ++count;
        outRecord = record;
      }
      if (count > 1)
      {
        outRecord = ResultRecord();
        break;
      }
    }
    
    return outRecord;
  };
  
  for (const auto &currentShapeType : searchTypes)
  {
    ResultRecord blendRecord = getUniqueRecord(resultContainer, currentShapeType);
    ResultRecord targetRecord = getUniqueRecord(targetFeatureIn->getResultContainer(), currentShapeType);
    if (!blendRecord.id.is_nil()) //if the blend record has already been set, skip.
      continue;
    if (blendRecord.shape.IsNull()) //couldn't find unique record
      continue;
    if (targetRecord.id.is_nil()) //couldn't find unique record
      continue;
    
    //here we have an acceptable match.
    Feature::updateId(resultContainer, targetRecord.id, blendRecord.shape);
  }
}

/*matches the outerWire of faces between the target container
 * and "this" container. This does not include the outer wire
 * of generated faces.
 */
void Blend::outerWireMatch(const Base *targetFeatureIn)
{
  const TopoDS_Shape &targetShape = targetFeatureIn->getShape();
  TopTools_IndexedMapOfShape shapeMap;
  TopExp::MapShapes(targetShape, TopAbs_FACE, shapeMap);
  for (int index = 1; index <= shapeMap.Extent(); ++index)
  {
    const TopoDS_Shape &targetFace = shapeMap(index);
    TopoDS_Shape targetOuterWire = BRepTools::OuterWire(TopoDS::Face(targetFace));
    uuid targetOuterWireId = Feature::findResultByShape(targetFeatureIn->getResultContainer(), targetOuterWire).id;
    uuid targetFaceId = Feature::findResultByShape(targetFeatureIn->getResultContainer(), targetFace).id;
    
    if (Feature::hasResult(resultContainer, targetOuterWireId))
      continue;
    if (!Feature::hasResult(resultContainer, targetFaceId))
    {
      std::cout << "can't find faceId" << std::endl;
      continue;
    }
    
    const TopoDS_Shape &outFace = Feature::findResultById(resultContainer, targetFaceId).shape;
    assert(!outFace.IsNull());
    TopoDS_Shape outOuterWire = BRepTools::OuterWire(TopoDS::Face(outFace));
    Feature::updateId(resultContainer, targetOuterWireId, outOuterWire);
  }
}

/* after all of the above the only shapes left are the new edges and
 * vertices created on the new blend faces. We need to id these.
 * the ids for these are based upon a set of faces. Usually 2 faces
 * for edges and 3 faces for vertices. This might not be enough and we
 * to have the shape type involved. we will see.
 */
void Blend::derivedMatch(BRepFilletAPI_MakeFillet &blendMakerIn, const Base *targetFeatureIn)
{
  TopTools_IndexedMapOfShape edgeShapes; //edges of new blend faces.
  TopTools_IndexedMapOfShape vertexShapes; //vertices of new blend faces.
  
  TopTools_IndexedMapOfShape targetShapeMap;
  TopExp::MapShapes(targetFeatureIn->getShape(), targetShapeMap);
  
  for (int index = 1; index <= targetShapeMap.Extent(); ++index)
  {
    const TopoDS_Shape &currentShape = targetShapeMap(index);
    assert(!currentShape.IsNull());
    
    //find the resultant face of edge.
    TopTools_ListOfShape generated = blendMakerIn.Generated(currentShape);
    if (generated.IsEmpty())
      continue;
    assert(generated.Extent() == 1);
    const TopoDS_Shape &blendedFace = generated.First();
    assert(!blendedFace.IsNull());
    assert(blendedFace.ShapeType() == TopAbs_FACE);
    
    TopExp::MapShapes(blendedFace, TopAbs_EDGE, edgeShapes);
    TopExp::MapShapes(blendedFace, TopAbs_VERTEX, vertexShapes);
  }
  
  TopTools_IndexedDataMapOfShapeListOfShape eToF; //edges to face
  TopExp::MapShapesAndAncestors(shape, TopAbs_EDGE, TopAbs_FACE, eToF);
  for (int edgeIndex = 1; edgeIndex <= edgeShapes.Extent(); ++edgeIndex)
  {
    IdSet idSet;
    const TopTools_ListOfShape& faceList = eToF.FindFromKey(edgeShapes(edgeIndex));
    assert(!faceList.IsEmpty());
    TopTools_ListIteratorOfListOfShape it;
    for (it.Initialize(faceList); it.More(); it.Next())
    {
      uuid faceId = Feature::findResultByShape(resultContainer, it.Value()).id;
      idSet.insert(faceId);
    }
    
    uuid edgeId;
    if (derivedContainer.count(idSet) < 1)
    {
      DerivedContainer::value_type newEntry
      (
        idSet,
        boost::uuids::basic_random_generator<boost::mt19937>()()
      );
      derivedContainer.insert(newEntry);
      
      edgeId = newEntry.second;
    }
    else
      edgeId = derivedContainer.at(idSet);
    
    Feature::updateId(resultContainer, edgeId, edgeShapes(edgeIndex));
  }
  
  TopTools_IndexedDataMapOfShapeListOfShape vToF; //edges to face
  TopExp::MapShapesAndAncestors(shape, TopAbs_VERTEX, TopAbs_FACE, vToF);
  for (int vertexIndex = 1; vertexIndex <= vertexShapes.Extent(); ++vertexIndex)
  {
    IdSet idSet;
    const TopTools_ListOfShape& faceList = vToF.FindFromKey(vertexShapes(vertexIndex));
    assert(!faceList.IsEmpty());
    TopTools_ListIteratorOfListOfShape it;
    for (it.Initialize(faceList); it.More(); it.Next())
    {
      uuid faceId = Feature::findResultByShape(resultContainer, it.Value()).id;
      idSet.insert(faceId);
    }
    
    uuid vertexId;
    if (derivedContainer.count(idSet) < 1)
    {
      DerivedContainer::value_type newEntry
      (
        idSet,
        boost::uuids::basic_random_generator<boost::mt19937>()()
      );
      derivedContainer.insert(newEntry);
      
      vertexId = newEntry.second;
    }
    else
      vertexId = derivedContainer.at(idSet);
    
    Feature::updateId(resultContainer, vertexId, vertexShapes(vertexIndex));
  }
}

void Blend::dumpInfo(BRepFilletAPI_MakeFillet &blendMakerIn, const Base *targetFeatureIn)
{
  std::cout << std::endl << std::endl <<
    "shape out type is: " << GU::getShapeTypeString(blendMakerIn.Shape()) << std::endl <<
    "fillet dump:" << std::endl;
  
  const TopoDS_Shape &targetShape = targetFeatureIn->getShape();
  TopTools_IndexedMapOfShape shapeMap;
  TopExp::MapShapes(targetShape, shapeMap);
  for (int index = 1; index <= shapeMap.Extent(); ++index)
  {
    const TopoDS_Shape &currentShape = shapeMap(index);
    std::cout << "ShapeType is: " << std::setw(10) << GU::getShapeTypeString(currentShape) << 
      "      Generated Count is: " << blendMakerIn.Generated(currentShape).Extent() <<
      "      Modified Count is: " << blendMakerIn.Modified(currentShape).Extent() <<
      "      is deleted: " << ((blendMakerIn.IsDeleted(currentShape)) ? "true" : "false") << std::endl; 
      
      if (blendMakerIn.Generated(currentShape).Extent() > 0)
        std::cout << "   generated type is: " << 
          GU::getShapeTypeString(blendMakerIn.Generated(currentShape).First()) << std::endl;
  }
  
  std::cout << std::endl << std::endl << "output of blend: " << std::endl;
  TopTools_IndexedMapOfShape outShapeMap;
  TopExp::MapShapes(blendMakerIn.Shape(), outShapeMap);
  for (int index = 1; index <= outShapeMap.Extent(); ++index)
  {
    const TopoDS_Shape &currentShape = outShapeMap(index);
    std::cout << "ShapteType is: " << std::setw(10) << GU::getShapeTypeString(currentShape) <<
    "      is in targetResults: " <<
      ((Feature::hasResult(targetFeatureIn->getResultContainer(), currentShape)) ? "true" : "false") << std::endl;
  }
}

/* inform the number of nil shapes left in result container, if any. */
void Blend::dumpResultStats()
{
  std::ostringstream stream;
  stream << std::endl << "result container stats:" << std::endl;
  int counter = 0;
  for (const auto &record : resultContainer)
  {
    if (record.id.is_nil())
    {
      stream << std::setw(10) << GU::getShapeTypeString(record.shape) << "      is nil" << std::endl;
      ++counter;
    }
  }
  if (counter > 0)
  {
    stream << counter << " nil shapes" << std::endl;
    std::cout << stream.str();
  }
}

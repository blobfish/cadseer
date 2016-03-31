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
#include <boost/uuid/random_generator.hpp>

#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <BRepTools.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <BRepBuilderAPI_MakeShape.hxx>

#include <globalutilities.h>
#include <feature/base.h>
#include <feature/shapeidmapper.h>

static const std::vector<std::string> shapeStrings
({
     "Compound",
     "CompSolid",
     "Solid",
     "Shell",
     "Face",
     "Wire",
     "Edge",
     "Vertex",
     "Shape",
 });

static boost::uuids::basic_random_generator<boost::mt19937> idGenerator;

ftr::ResultContainer ftr::createInitialContainer(const TopoDS_Shape &shapeIn)
{
  ResultContainer resultContainer;
  boost::uuids::nil_generator ng;
  TopTools_IndexedMapOfShape freshShapeMap;
  TopExp::MapShapes(shapeIn, freshShapeMap);
  for (int index = 1; index <= freshShapeMap.Extent(); ++index)
  {
    ResultRecord record;
    record.id = ng();
    record.shape = freshShapeMap(index);
    
    resultContainer.insert(record);
  }
  return resultContainer;
}

void ftr::shapeMatch(const ResultContainer &source, ResultContainer &target)
{
  typedef ResultContainer::index<ResultRecord::ByShape>::type List;
  const List &list = source.get<ResultRecord::ByShape>();
  
  for (const auto &record : list)
  {
    if (!hasResult(target, record.shape))
      continue;
    updateId(target, record.id, record.shape);
  }
}

void ftr::uniqueTypeMatch(const ResultContainer &source, ResultContainer &target)
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
  auto getUniqueRecord = [](const ResultContainer &containerIn, ResultRecord &recordOut, TopAbs_ShapeEnum shapeTypeIn) -> bool
  {
    std::size_t count = 0;
    
    for (const auto &record : containerIn)
    {
      if (record.shape.ShapeType() == shapeTypeIn)
      {
        ++count;
        recordOut = record;
      }
      if (count > 1)
        break;
    }
    
    if (count == 1)
      return true;
    else
      return false;
  };
  
  for (const auto &currentShapeType : searchTypes)
  {
    ResultRecord sourceRecord;
    ResultRecord targetRecord;
    if
    (
      (!getUniqueRecord(source, sourceRecord, currentShapeType)) ||
      (!getUniqueRecord(target, targetRecord, currentShapeType))
    )
      continue;
      
    updateId(target, sourceRecord.id, targetRecord.shape);
  }
}

void ftr::outerWireMatch(const ResultContainer &source, ResultContainer &target)
{
  typedef ResultContainer::index<ResultRecord::ById>::type List;
  const List &targetList = target.get<ResultRecord::ById>();
  const List &sourceList = source.get<ResultRecord::ById>();
  std::vector<std::pair<boost::uuids::uuid, TopoDS_Shape> > pairVector;
  for (const auto &targetFaceRecord : targetList)
  {
    if (targetFaceRecord.shape.ShapeType() != TopAbs_FACE)
      continue;
    const TopoDS_Shape &targetOuterWire = BRepTools::OuterWire(TopoDS::Face(targetFaceRecord.shape));
    assert(hasResult(target, targetOuterWire)); //shouldn't have a result container with a face and no outer wire.
    if (!findResultByShape(target, targetOuterWire).id.is_nil())
      continue; //only set id for nil wires.
      
    //now find entries in source.
    List::const_iterator faceIt = sourceList.find(targetFaceRecord.id);
    if (faceIt == sourceList.end())
      continue;
    const TopoDS_Shape &sourceOuterWire = BRepTools::OuterWire(TopoDS::Face(faceIt->shape));
    assert(!sourceOuterWire.IsNull());
    auto sourceId = findResultByShape(source, sourceOuterWire).id;
    
    //save for later
    pairVector.push_back(std::make_pair(sourceId, targetOuterWire));
  }
  
  for (const auto &pair : pairVector)
    updateId(target, pair.first, pair.second);
}

void ftr::modifiedMatch
(
  BRepBuilderAPI_MakeShape &shapeMakerIn,
  const ResultContainer &source,
  ResultContainer &target
)
{
  typedef ResultContainer::index<ResultRecord::ById>::type List;
  const List &sourceList = source.get<ResultRecord::ById>();
  for (const auto &sourceRecord : sourceList)
  {
    const TopTools_ListOfShape &modifiedList = shapeMakerIn.Modified(sourceRecord.shape);
    if (modifiedList.IsEmpty())
      continue;
    TopTools_ListIteratorOfListOfShape it(modifiedList);
    //what happens here when we have more than one in the shape.
    //we end up with multiple entries in the target with the
    //same id? Evolution container? Need to consider this.
    for(;it.More(); it.Next())
    {
      //could be situations where members of the modified list
      //are not in the target container. Booleans operations have
      //this situation. In short, no assert on shape not present.
      if(!hasResult(target, it.Value()))
	continue;
      updateId(target, sourceRecord.id, it.Value());
    }
  }
}

void ftr::derivedMatch(const TopoDS_Shape &shapeIn, ResultContainer &target, DerivedContainer &derivedIn)
{
  gu::ShapeVector nilVertices, nilEdges;
  
  typedef ResultContainer::index<ResultRecord::ById>::type List;
  List &list = target.get<ResultRecord::ById>();
  auto rangeItPair = list.equal_range(boost::uuids::nil_generator()());
  for (; rangeItPair.first != rangeItPair.second; ++rangeItPair.first)
  {
    if (rangeItPair.first->shape.ShapeType() == TopAbs_VERTEX)
      nilVertices.push_back(rangeItPair.first->shape);
    else if (rangeItPair.first->shape.ShapeType() == TopAbs_EDGE)
      nilEdges.push_back(rangeItPair.first->shape);
  }
  
  if (nilVertices.empty() && nilEdges.empty())
    return;
  
  TopTools_IndexedDataMapOfShapeListOfShape vToE, eToF; //vertices to edges, edges to faces
  TopExp::MapShapesAndAncestors(shapeIn, TopAbs_VERTEX, TopAbs_EDGE, vToE);
  TopExp::MapShapesAndAncestors(shapeIn, TopAbs_EDGE, TopAbs_FACE, eToF);
  
  auto go = [&](const gu::ShapeVector &nilShapes, const TopTools_IndexedDataMapOfShapeListOfShape &ancestors)
  {
    for (const auto &shape : nilShapes)
    {
      bool bail = false;
      IdSet set;
      const TopTools_ListOfShape &parents = ancestors.FindFromKey(shape);
      TopTools_ListIteratorOfListOfShape it;
      for (it.Initialize(parents); it.More(); it.Next())
      {
	assert(hasResult(target, it.Value()));
	boost::uuids::uuid id = findResultByShape(target, it.Value()).id;
	if (id.is_nil())
	{
	  std::cout << "empty parent Id in: " << __PRETTY_FUNCTION__ << std::endl;
	  bail = true;
	  break;
	}
	set.insert(id);
      }
      if (bail)
	continue;
      DerivedContainer::iterator derivedIt = derivedIn.find(set);
      if (derivedIt == derivedIn.end())
      {
	auto id = idGenerator();
	updateId(target, id, shape);
	DerivedContainer::value_type newEntry(set, id);
	derivedIn.insert(newEntry);
      }
      else
      {
	updateId(target, derivedIt->second, shape);
      }
    }
  };
  
  go(nilEdges, eToF);
  go(nilVertices, vToE);
}

void ftr::dumpNils(const ResultContainer &target, const std::string &headerIn)
{
  std::ostringstream stream;
  
  typedef ResultContainer::index<ResultRecord::ById>::type List;
  const List &list = target.get<ResultRecord::ById>();
  auto rangeItPair = list.equal_range(boost::uuids::nil_generator()());
  for (; rangeItPair.first != rangeItPair.second; ++rangeItPair.first)
   stream << boost::uuids::to_string(rangeItPair.first->id) << "    "
    << rangeItPair.first->shape << std::endl;
  
  
  if (!stream.str().empty())
    std::cout << std::endl << headerIn << " nils" << std::endl << stream.str() << std::endl;
}

void ftr::dumpDuplicates(const ftr::ResultContainer &target, const std::string &headerIn)
{
  std::ostringstream stream;
  
  std::set<boost::uuids::uuid> processed;
  typedef ResultContainer::index<ResultRecord::ById>::type List;
  const List &list = target.get<ResultRecord::ById>();
  for (const auto &record : list)
  {
    if (processed.count(record.id) > 0)
      continue;
    std::size_t count = target.count(record.id);
    if (count > 1)
    {
      processed.insert(record.id);
      stream << record.id << "    " << count << "    " << record.shape << std::endl;
    }
  }
  if (!stream.str().empty())
    std::cout << std::endl << headerIn << " duplicates" << std::endl << stream.str() << std::endl;
}

void ftr::ensureNoNils(ResultContainer &target)
{
  gu::ShapeVector nilShapes;
  
  typedef ResultContainer::index<ResultRecord::ById>::type List;
  List &list = target.get<ResultRecord::ById>();
  auto rangeItPair = list.equal_range(boost::uuids::nil_generator()());
  for (; rangeItPair.first != rangeItPair.second; ++rangeItPair.first)
    nilShapes.push_back(rangeItPair.first->shape);
  
  for (const auto &shape : nilShapes)
    updateId(target, idGenerator(), shape);
}

void ftr::ensureNoDuplicates(ResultContainer &target)
{
  std::set<boost::uuids::uuid> processed;
  gu::ShapeVector shapes;
  typedef ResultContainer::index<ResultRecord::ById>::type List;
  const List &list = target.get<ResultRecord::ById>();
  for (const auto &record : list)
  {
    if (processed.count(record.id) > 0)
      shapes.push_back(record.shape);
    else
      processed.insert(record.id);
  }
  for (const auto &shape : shapes)
    updateId(target, idGenerator(), shape);
}

void ftr::faceEdgeMatch(const ResultContainer &sourceContainer, ResultContainer &freshContainer)
{
  using boost::uuids::uuid;
  
  gu::ShapeVector nilEdges;
  typedef ResultContainer::index<ResultRecord::ById>::type List;
  List &list = freshContainer.get<ResultRecord::ById>();
  auto rangeItPair = list.equal_range(boost::uuids::nil_generator()());
  for (; rangeItPair.first != rangeItPair.second; ++rangeItPair.first)
  {
    if (rangeItPair.first->shape.ShapeType() != TopAbs_EDGE)
      continue;
    nilEdges.push_back(rangeItPair.first->shape);
  }
  
  const TopoDS_Shape &rootShape = ftr::findRootShape(freshContainer);
  TopTools_IndexedDataMapOfShapeListOfShape eToF;
  TopExp::MapShapesAndAncestors(rootShape, TopAbs_EDGE, TopAbs_FACE, eToF);
  for (const auto &nilEdge : nilEdges)
  {
    //get 2 parent faces id. need 2 to make edge unique.
    const TopTools_ListOfShape &parentFaces = eToF.FindFromKey(nilEdge);
    if (parentFaces.Extent() != 2)
      continue;
    std::vector<uuid> faceIds;
    TopTools_ListIteratorOfListOfShape faceIt(parentFaces);
    for (; faceIt.More(); faceIt.Next())
    {
      const uuid &faceId = findResultByShape(freshContainer, faceIt.Value()).id;
      if (faceId.is_nil())
	continue;
      faceIds.push_back(faceId);
    }
    if (faceIds.size() != 2)
      continue;
    
    if
    (!(
      (hasResult(sourceContainer, faceIds.front())) &&
      (hasResult(sourceContainer, faceIds.back()))
    ))
      continue;
    
    TopTools_IndexedMapOfShape face1Edges, face2Edges;
    TopExp::MapShapes(findResultById(sourceContainer, faceIds.front()).shape, TopAbs_EDGE,  face1Edges);
    TopExp::MapShapes(findResultById(sourceContainer, faceIds.back()).shape, TopAbs_EDGE, face2Edges);
    
    uuid edgeId = boost::uuids::nil_generator()();
    for (int index = 1; index <= face1Edges.Extent(); ++index)
    {
      const TopoDS_Shape &edge = face1Edges(index);
      if (face2Edges.Contains(edge))
      {
	assert(hasResult(sourceContainer, edge));
	edgeId = findResultByShape(sourceContainer, edge).id;
	break;
      }
    }
    if (!edgeId.is_nil())
      updateId(freshContainer, edgeId, nilEdge);
  }
}

void ftr::edgeVertexMatch(const ResultContainer &sourceContainer, ResultContainer &freshContainer)
{
  using boost::uuids::uuid;
  
  gu::ShapeVector nilVertices;
  typedef ResultContainer::index<ResultRecord::ById>::type List;
  List &list = freshContainer.get<ResultRecord::ById>();
  auto rangeItPair = list.equal_range(boost::uuids::nil_generator()());
  for (; rangeItPair.first != rangeItPair.second; ++rangeItPair.first)
  {
    if (rangeItPair.first->shape.ShapeType() != TopAbs_VERTEX)
      continue;
    nilVertices.push_back(rangeItPair.first->shape);
  }
  
  const TopoDS_Shape &rootShape = ftr::findRootShape(freshContainer);
  TopTools_IndexedDataMapOfShapeListOfShape vToE;
  TopExp::MapShapesAndAncestors(rootShape, TopAbs_VERTEX, TopAbs_EDGE, vToE);
  for (const auto &nilVertex : nilVertices)
  {
    //get 2 parent faces id. need 2 to make edge unique.
    const TopTools_ListOfShape &parentEdgesList = vToE.FindFromKey(nilVertex);
    TopTools_IndexedMapOfShape parentEdges;
    TopTools_ListIteratorOfListOfShape edgeIt(parentEdgesList);
    for (; edgeIt.More(); edgeIt.Next())
      parentEdges.Add(edgeIt.Value());
    std::size_t parentEdgeCount = parentEdges.Extent();
    if (parentEdgeCount != 2 && parentEdgeCount != 3)
      continue;
    std::vector<uuid> edgeIds;
    for (int index = 1; index <= parentEdges.Extent(); index++)
    {
      const uuid &edgeId = findResultByShape(freshContainer, parentEdges(index)).id;
      if (edgeId.is_nil())
	continue;
      edgeIds.push_back(edgeId);
    }
    if (edgeIds.size() != parentEdgeCount)
      continue;
    
    std::vector<TopTools_IndexedMapOfShape> maps;
    for (const auto edgeId : edgeIds)
    {
      maps.push_back(TopTools_IndexedMapOfShape());
      TopExp::MapShapes(findResultById(sourceContainer, edgeId).shape, TopAbs_VERTEX, maps.back());
    }
    
    uuid vertexId = boost::uuids::nil_generator()();
    for (int index = 1; index <= maps.front().Extent(); ++index)
    {
      const TopoDS_Shape &vertex = maps.front()(index);
      bool missing = false;
      std::vector<TopTools_IndexedMapOfShape>::const_iterator it = maps.begin();
      it++; //don't check against itself
      while ((it != maps.end()) && (missing == false))
      {
	if (!(it->Contains(vertex)))
	  missing = true;
	++it;
      }
      if (!missing)
      {
	assert(hasResult(sourceContainer, vertex));
	vertexId = findResultByShape(sourceContainer, vertex).id;
	break;
      }
    }
    if (!vertexId.is_nil())
      updateId(freshContainer, vertexId, nilVertex);
  }
}
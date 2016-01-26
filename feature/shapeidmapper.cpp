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
    std::cout << std::endl << headerIn << std::endl << stream.str() << std::endl;
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

void ftr::updateSplits(ResultContainer &target, EvolutionContainer &evo)
{
  //this all seems a little precarious. The assignment of
  //the duplicate ids in the result container from the evo container is
  //dependent on the order of both lists. We will be back here.
  
  std::vector<std::pair<TopoDS_Shape, boost::uuids::uuid> > updateVector;
  
  std::set<boost::uuids::uuid> doneIds;
  typedef EvolutionContainer::index<EvolutionRecord::ByInId>::type EvoList;
  EvoList &evoList = evo.get<EvolutionRecord::ByInId>();
  typedef ResultContainer::index<ResultRecord::ById>::type List;
  const List &list = target.get<ResultRecord::ById>();
  for (const auto &record : list)
  {
    if (doneIds.count(record.id) > 0)
      continue;
    doneIds.insert(record.id);
    
    auto resultRange = list.equal_range(record.id);
    resultRange.first++; //we know there is a least one and that first one won't be in the evo
    
    auto evoRange = evoList.equal_range(record.id);
    //ensure we have enough ids in evorange to do a parallel iteration.
    int difference = std::distance(resultRange.first, resultRange.second) - std::distance(evoRange.first, evoRange.second);
    for (int index = 0; index < difference; ++index)
      evoList.insert(EvolutionRecord(record.id, idGenerator()));
    evoRange = evoList.equal_range(record.id); //update range.
    
    for (;resultRange.first != resultRange.second; ++resultRange.first, ++evoRange.first)
      updateVector.push_back(std::make_pair(resultRange.first->shape, evoRange.first->outId));
  }
  
  for (const auto &couple : updateVector)
    updateId(target, couple.second, couple.first);
}
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
#include <functional>
#include <stack>
#include <algorithm>

#include <boost/graph/iteration_macros.hpp>
#include <boost/graph/graphviz.hpp>

#include <gp_Circ.hxx>
#include <gp_Elips.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopExp.hxx>
#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepTools.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <BRepBuilderAPI_Copy.hxx>

#include <osg/Vec3d>

#include <tools/idtools.h>
#include <tools/occtools.h>
#include <project/serial/xsdcxxoutput/featurebase.h>
#include <feature/shapehistory.h>
#include <annex/seershape.h>

using namespace ann;

using boost::uuids::uuid;

std::ostream& ann::operator<<(std::ostream& os, const ShapeIdRecord& record)
{
  os << gu::idToString(record.id) << "      " << 
    ShapeIdKeyHash()(record.shape) << "      " <<
    record.graphVertex << "      " <<
    shapeStrings.at(record.shape.ShapeType()) << std::endl;
  return os;
}

std::ostream& ann::operator<<(std::ostream& os, const ShapeIdContainer& container)
{
  typedef ShapeIdContainer::index<ShapeIdRecord::ById>::type List;
  const List &list = container.get<ShapeIdRecord::ById>();
  for (List::const_iterator it = list.begin(); it != list.end(); ++it)
    os << *it;
  return os;
}



ostream& ann::operator<<(ostream& os, const EvolveRecord& record)
{
  os << gu::idToString(record.inId) << "      " << gu::idToString(record.outId) << std::endl;
  return os;
}

ostream& ann::operator<<(ostream& os, const EvolveContainer& container)
{
  typedef EvolveContainer::index<EvolveRecord::ByInId>::type List;
  const List &list = container.get<EvolveRecord::ByInId>();
  for (List::const_iterator it = list.begin(); it != list.end(); ++it)
    os << *it;
  return os;
}



ostream& ann::operator<<(ostream& os, const FeatureTagRecord& record)
{
  os << gu::idToString(record.id) << "      " << record.tag << std::endl;
  return os;
}

ostream& ann::operator<<(ostream& os, const FeatureTagContainer& container)
{
  typedef FeatureTagContainer::index<FeatureTagRecord::ById>::type List;
  const List &list = container.get<FeatureTagRecord::ById>();
  for (List::const_iterator it = list.begin(); it != list.end(); ++it)
    os << *it;
  return os;
}

SeerShape::SeerShape() : Base()
{
  reset();
}

SeerShape::~SeerShape(){}

void SeerShape::setOCCTShape(const TopoDS_Shape& shapeIn)
{
  reset();
  
  if (shapeIn.IsNull())
    return;
  
  //make sure we have a compound as root of shape.
  TopoDS_Shape workShape = shapeIn;
  if (workShape.ShapeType() != TopAbs_COMPOUND)
  {
    TopoDS_Compound compound;
    BRep_Builder builder;
    builder.MakeCompound(compound);
    builder.Add(compound, workShape);
    workShape = compound;
  }
  
  //fill in container with shapes and nil ids and new vertices.
  for (const auto &s : occt::mapShapes(workShape))
  {
    ShapeIdRecord record;
    record.shape = s;
    record.graphVertex = boost::add_vertex(graph);
    //id is nil. set by record constructor.
    
    shapeIdContainer.insert(record);
  }
  
  //root compound shape needs an id even though it maybe temp.
  rootShapeId = gu::createRandomId();
  updateShapeIdRecord(workShape, rootShapeId);
  rGraph = graph; //now graph and rGraph are equal, edgeless graphs.
  
  updateGraphs();
}

void SeerShape::reset()
{
  rootShapeId = gu::createNilId();
  shapeIdContainer.get<ShapeIdRecord::ById>().clear();
  graph = Graph();
  rGraph = graph;
}

void SeerShape::updateGraphs()
{
  //expects that graph and rGraph are equal and edgeless.
  //expects that the graph and rGraph contain vertices for all shapes in container.
  //expects that the rootShapeId is set even temporarily.
  
  //recursive function to build graph.
  std::stack<TopoDS_Shape> shapeStack;
  std::function <void (const TopoDS_Shape &)> recursion = [&](const TopoDS_Shape &shapeIn)
  {
    for (TopoDS_Iterator it(shapeIn); it.More(); it.Next())
    {
      const TopoDS_Shape &currentShape = it.Value();
      if (!(hasShapeIdRecord(currentShape)))
        continue; //topods_iterator doesn't ignore orientation like mapShapes does. probably seam edge.

      //add edge to previous graph vertex if stack is not empty.
      if (!shapeStack.empty())
      {
        Vertex pVertex = findShapeIdRecord(shapeStack.top()).graphVertex;
        Vertex cVertex = findShapeIdRecord(currentShape).graphVertex;
        
        if (!boost::edge(pVertex, cVertex, graph).second)
        {
          boost::add_edge(pVertex, cVertex, graph);
          boost::add_edge(cVertex, pVertex, rGraph);
        }
      }
      shapeStack.push(currentShape);
      recursion(currentShape);
      shapeStack.pop();
    }
  };
  
  const TopoDS_Shape &rootWorkShape = getRootOCCTShape();
  shapeStack.push(rootWorkShape);
  recursion(rootWorkShape);
}

bool SeerShape::hasShapeIdRecord(const uuid& idIn) const
{
  typedef ShapeIdContainer::index<ShapeIdRecord::ById>::type List;
  const List &list = shapeIdContainer.get<ShapeIdRecord::ById>();
  List::const_iterator it = list.find(idIn);
  return (it != list.end());
}

bool SeerShape::hasShapeIdRecord(const Vertex& vertexIn) const
{
  typedef ShapeIdContainer::index<ShapeIdRecord::ByVertex>::type List;
  const List &list = shapeIdContainer.get<ShapeIdRecord::ByVertex>();
  List::const_iterator it = list.find(vertexIn);
  return (it != list.end());
}

bool SeerShape::hasShapeIdRecord(const TopoDS_Shape& shapeIn) const
{
  typedef ShapeIdContainer::index<ShapeIdRecord::ByShape>::type List;
  const List &list = shapeIdContainer.get<ShapeIdRecord::ByShape>();
  List::const_iterator it = list.find(shapeIn);
  return (it != list.end());
}

const ShapeIdRecord& SeerShape::findShapeIdRecord(const uuid& idIn) const
{
  typedef ShapeIdContainer::index<ShapeIdRecord::ById>::type List;
  const List &list = shapeIdContainer.get<ShapeIdRecord::ById>();
  List::const_iterator it = list.find(idIn);
  assert(it != list.end());
  return *it;
}

const ShapeIdRecord& SeerShape::findShapeIdRecord(const Vertex& vertexIn) const
{
  typedef ShapeIdContainer::index<ShapeIdRecord::ByVertex>::type List;
  const List &list = shapeIdContainer.get<ShapeIdRecord::ByVertex>();
  List::const_iterator it = list.find(vertexIn);
  assert(it != list.end());
  return *it;
}

const ShapeIdRecord& SeerShape::findShapeIdRecord(const TopoDS_Shape& shapeIn) const
{
  typedef ShapeIdContainer::index<ShapeIdRecord::ByShape>::type List;
  const List &list = shapeIdContainer.get<ShapeIdRecord::ByShape>();
  List::const_iterator it = list.find(shapeIn);
  assert(it != list.end());
  return *it;
}

//! updates the shape by matching id.
void SeerShape::updateShapeIdRecord(const uuid& idIn, const TopoDS_Shape& shapeIn)
{
  typedef ShapeIdContainer::index<ShapeIdRecord::ById>::type List;
  List &list = shapeIdContainer.get<ShapeIdRecord::ById>();
  List::iterator it = list.find(idIn);
  assert(it != list.end());
  ShapeIdRecord record = *it;
  record.shape = shapeIn;
  list.replace(it, record);
}

//! updates the vertex by matching id.
void SeerShape::updateShapeIdRecord(const uuid& idIn, const Vertex& vertexIn)
{
  typedef ShapeIdContainer::index<ShapeIdRecord::ById>::type List;
  List &list = shapeIdContainer.get<ShapeIdRecord::ById>();
  List::iterator it = list.find(idIn);
  assert(it != list.end());
  ShapeIdRecord record = *it;
  record.graphVertex = vertexIn;
  list.replace(it, record);
}

//! updates the id by matching shape.
void SeerShape::updateShapeIdRecord(const TopoDS_Shape& shapeIn, const uuid& idIn)
{
  typedef ShapeIdContainer::index<ShapeIdRecord::ByShape>::type List;
  List &list = shapeIdContainer.get<ShapeIdRecord::ByShape>();
  List::iterator it = list.find(shapeIn);
  assert(it != list.end());
  ShapeIdRecord record = *it;
  record.id = idIn;
  list.replace(it, record);
}

void SeerShape::updateShapeIdRecord(const TopoDS_Shape& shapeIn, const Vertex& vertexIn)
{
  typedef ShapeIdContainer::index<ShapeIdRecord::ByShape>::type List;
  List &list = shapeIdContainer.get<ShapeIdRecord::ByShape>();
  List::iterator it = list.find(shapeIn);
  assert(it != list.end());
  ShapeIdRecord record = *it;
  record.graphVertex = vertexIn;
  list.replace(it, record);
}

void SeerShape::updateShapeIdRecord(const Vertex& vertexIn, const uuid& idIn)
{
  typedef ShapeIdContainer::index<ShapeIdRecord::ByVertex>::type List;
  List &list = shapeIdContainer.get<ShapeIdRecord::ByVertex>();
  List::iterator it = list.find(vertexIn);
  assert(it != list.end());
  ShapeIdRecord record = *it;
  record.id = idIn;
  list.replace(it, record);
}

void SeerShape::updateShapeIdRecord(const Vertex& vertexIn, const TopoDS_Shape& shapeIn)
{
  typedef ShapeIdContainer::index<ShapeIdRecord::ByVertex>::type List;
  List &list = shapeIdContainer.get<ShapeIdRecord::ByVertex>();
  List::iterator it = list.find(vertexIn);
  assert(it != list.end());
  ShapeIdRecord record = *it;
  record.shape = shapeIn;
  list.replace(it, record);
}

std::vector<uuid> SeerShape::getAllShapeIds() const
{
  std::vector<uuid> out;
  
  typedef ShapeIdContainer::index<ShapeIdRecord::ById>::type List;
  const List &list = shapeIdContainer.get<ShapeIdRecord::ById>();
  for (const auto &entry : list)
    out.push_back(entry.id);
  return out;
}

occt::ShapeVector SeerShape::getAllShapes() const
{
  occt::ShapeVector out;
  
  typedef ShapeIdContainer::index<ShapeIdRecord::ById>::type List;
  const List &list = shapeIdContainer.get<ShapeIdRecord::ById>();
  for (const auto &entry : list)
    out.push_back(entry.shape);
  return out;
}

occt::ShapeVector SeerShape::getAllNilShapes() const
{
  occt::ShapeVector out;
  typedef ShapeIdContainer::index<ShapeIdRecord::ById>::type List;
  const List &list = shapeIdContainer.get<ShapeIdRecord::ById>();
  auto rangeItPair = list.equal_range(gu::createNilId());
  for (; rangeItPair.first != rangeItPair.second; ++rangeItPair.first)
    out.push_back(rangeItPair.first->shape);
  
  return out;
}

bool SeerShape::hasEvolveRecordIn(const uuid &idIn) const
{
  typedef EvolveContainer::index<EvolveRecord::ByInId>::type List;
  const List &list = evolveContainer.get<EvolveRecord::ByInId>();
  List::const_iterator it = list.find(idIn);
  return (it != list.end());
}

bool SeerShape::hasEvolveRecordOut(const uuid &idOut) const
{
  typedef EvolveContainer::index<EvolveRecord::ByOutId>::type List;
  const List &list = evolveContainer.get<EvolveRecord::ByOutId>();
  List::const_iterator it = list.find(idOut);
  return (it != list.end());
}

bool SeerShape::hasEvolveRecord(const BID::uuid &inId, const BID::uuid &outId) const
{
  typedef EvolveContainer::index<EvolveRecord::ByInOutIds>::type List;
  const List &list = evolveContainer.get<EvolveRecord::ByInOutIds>();
  List::const_iterator it = list.find(std::make_tuple(inId, outId));
  return (it != list.end());
}

std::vector<uuid> SeerShape::evolve(const uuid &idIn) const
{
  std::vector<uuid> out;
  typedef EvolveContainer::index<EvolveRecord::ByInId>::type List;
  const List &list = evolveContainer.get<EvolveRecord::ByInId>();
  auto rangeItPair = list.equal_range(idIn);
  for (; rangeItPair.first != rangeItPair.second; ++rangeItPair.first)
    out.push_back(rangeItPair.first->outId);
  
  return out;
}

std::vector<uuid> SeerShape::devolve(const uuid &idOut) const
{
  std::vector<uuid> out;
  typedef EvolveContainer::index<EvolveRecord::ByOutId>::type List;
  const List &list = evolveContainer.get<EvolveRecord::ByOutId>();
  auto rangeItPair = list.equal_range(idOut);
  for (; rangeItPair.first != rangeItPair.second; ++rangeItPair.first)
    out.push_back(rangeItPair.first->inId);
  
  return out;
}

void SeerShape::insertEvolve(const uuid& idIn, const uuid& idOut)
{
  evolveContainer.insert(EvolveRecord(idIn, idOut));
}

void SeerShape::insertEvolve(const EvolveRecord &recordIn)
{
  if (hasEvolveRecord(recordIn.inId, recordIn.outId))
    return;
  evolveContainer.insert(recordIn);
}

void SeerShape::fillInHistory(ftr::ShapeHistory &historyIn, const BID::uuid &featureId) const
{
  /* we need to create ids in the history graph for this shape.
   * these would be the out ids. We need to remember that the evolution container
   * is not cleared each update and may contain a large number of entries not
   * relevant to the current update.
   */
  
  typedef EvolveContainer::index<EvolveRecord::ByInId>::type List;
  const List &list = evolveContainer.get<EvolveRecord::ByInId>();
  for (List::const_iterator it = list.begin(); it != list.end(); ++it)
  {
    //if the outid is nil, that means that a shape didn't make it through operation.
    //if the outid doesn't exist in the actually finished shape, that means this entry is from another update.
    //we don't need to worry about these conditions.
    if (it->outId.is_nil() || !hasShapeIdRecord(it->outId))
      continue;
    
    if (!it->inId.is_nil())
    {
      //in this case we have a valid shape with a valid id in the out column, but the
      //in column id doesn't exist in the graph. A prior feature didn't update the history graph correctly.
      if(!historyIn.hasShape(it->inId))
        std::cout << "warning: shape id: " << gu::idToString(it->inId) << " should be in shape history in: " << __PRETTY_FUNCTION__ << std::endl;
    }
    
    if (!historyIn.hasShape(it->outId)) //might be there already, like a 'merge' situation.
      historyIn.addShape(featureId, it->outId);
    
    if (historyIn.hasShape(it->inId) && historyIn.hasShape(it->outId))
      historyIn.addConnection(it->outId, it->inId); //child points to parent.
  }
}

void SeerShape::replaceId(const uuid &staleId, const uuid &freshId, const ftr::ShapeHistory &)
{
  //not sure shape history should be passed into seershape?
  //don't see using this involving nil ids.
  assert(!staleId.is_nil());
  assert(!freshId.is_nil());
  
  typedef EvolveContainer::index<EvolveRecord::ByInId>::type ListIn;
  ListIn &listIn = evolveContainer.get<EvolveRecord::ByInId>();
  
  //iterator over equal_range was causing infinite loop. iterator invalidation I suspect.
  auto it = listIn.find(staleId);
  while (it != listIn.end())
  {
    EvolveRecord replacement = *it;
    replacement.inId = freshId;
    listIn.replace(it, replacement);
    it = listIn.find(staleId);
  }
  
//   auto rangeIn = listIn.equal_range(staleId);
//   for (; rangeIn.first != rangeIn.second; ++rangeIn.first)
//   {
//     EvolveRecord replacement = *(rangeIn.first);
//     replacement.inId = freshId;
//     listIn.replace(rangeIn.first, replacement);
//   }
  
  //don't think I need this for output. 
//   typedef EvolveContainer::index<EvolveRecord::ByOutId>::type ListOut;
//   ListOut &listOut = evolveContainer.get<EvolveRecord::ByOutId>();
//   auto rangeOut = listOut.equal_range(staleId);
//   for (; rangeOut.first != rangeOut.second; ++rangeOut.first)
//   {
//     EvolveRecord replacement = *rangeOut.first;
//     replacement.outId = freshId;
//     listOut.replace(rangeOut.first, replacement);
//   }
  
  //why would I ever need to replace an id in the feature tag?
//   typedef FeatureTagContainer::index<FeatureTagRecord::ById>::type TagList;
//   TagList &tagList = featureTagContainer.get<FeatureTagRecord::ById>();
//   auto tagRange = tagList.equal_range(staleId);
//   for (; tagRange.first != tagRange.second; ++tagRange.first)
//   {
//     FeatureTagRecord replacement = *tagRange.first;
//     replacement.id = freshId;
//     tagList.replace(tagRange.first, replacement);
//   }

  //I don't think I need to update the IdSet is either?
}

uuid SeerShape::featureTagId(const std::string& tagIn)
{
  typedef FeatureTagContainer::index<FeatureTagRecord::ByTag>::type List;
  const List &list = featureTagContainer.get<FeatureTagRecord::ByTag>();
  List::const_iterator it = list.find(tagIn);
  assert(it != list.end());
  return it->id;
}

void SeerShape::insertFeatureTag(const FeatureTagRecord &recordIn)
{
  featureTagContainer.insert(recordIn);
}

const TopoDS_Shape& SeerShape::getRootOCCTShape() const
{
  return findShapeIdRecord(rootShapeId).shape;
}

const uuid& SeerShape::getRootShapeId() const
{
  return rootShapeId;
}

const TopoDS_Shape& SeerShape::getOCCTShape(const uuid& idIn) const
{
  return findShapeIdRecord(idIn).shape;
}

void SeerShape::setRootShapeId(const uuid& idIn)
{
  rootShapeId = idIn;
}

bool SeerShape::isNull() const
{
  return rootShapeId.is_nil() || (!hasShapeIdRecord(rootShapeId));
}

std::vector<uuid> SeerShape::useGetParentsOfType
  (const uuid &idIn, const TopAbs_ShapeEnum &shapeTypeIn) const
{
  assert(hasShapeIdRecord(idIn));

  std::vector<Vertex> vertices;
  TypeCollectionVisitor vis(shapeTypeIn, *this, vertices);
  boost::breadth_first_search(rGraph, findShapeIdRecord(idIn).graphVertex, boost::visitor(vis));

  std::vector<Vertex>::const_iterator vit;
  std::vector<uuid> idsOut;
  for (vit = vertices.begin(); vit != vertices.end(); ++vit)
      idsOut.push_back(findShapeIdRecord(*vit).id);
  return idsOut;
}

occt::ShapeVector SeerShape::useGetParentsOfType
  (const TopoDS_Shape &shapeIn, const TopAbs_ShapeEnum& shapeTypeIn) const
{
  assert(hasShapeIdRecord(shapeIn));
  
  std::vector<Vertex> vertices;
  TypeCollectionVisitor vis(shapeTypeIn, *this, vertices);
  boost::breadth_first_search(rGraph, findShapeIdRecord(shapeIn).graphVertex, boost::visitor(vis));

  occt::ShapeVector shapesOut;
  for (const auto &gVertex : vertices)
    shapesOut.push_back(findShapeIdRecord(gVertex).shape);
  
  return shapesOut;
}

std::vector<uuid> SeerShape::useGetChildrenOfType
  (const uuid &idIn, const TopAbs_ShapeEnum &shapeTypeIn) const
{
  assert(hasShapeIdRecord(idIn));

  std::vector<Vertex> vertices;
  TypeCollectionVisitor vis(shapeTypeIn, *this, vertices);
  boost::breadth_first_search(graph, findShapeIdRecord(idIn).graphVertex, boost::visitor(vis));

  std::vector<Vertex>::const_iterator vit;
  std::vector<uuid> idsOut;
  for (vit = vertices.begin(); vit != vertices.end(); ++vit)
      idsOut.push_back(findShapeIdRecord(*vit).id);
  return idsOut;
}

occt::ShapeVector SeerShape::useGetChildrenOfType
  (const TopoDS_Shape &shapeIn, const TopAbs_ShapeEnum &shapeTypeIn) const
{
  assert(hasShapeIdRecord(shapeIn));

  std::vector<Vertex> vertices;
  TypeCollectionVisitor vis(shapeTypeIn, *this, vertices);
  boost::breadth_first_search(graph, findShapeIdRecord(shapeIn).graphVertex, boost::visitor(vis));

  std::vector<Vertex>::const_iterator vit;
  occt::ShapeVector out;
  for (vit = vertices.begin(); vit != vertices.end(); ++vit)
      out.push_back(findShapeIdRecord(*vit).shape);
  return out;
}

uuid SeerShape::useGetWire
  (const uuid &edgeIdIn, const uuid &faceIdIn) const
{
  assert(hasShapeIdRecord(edgeIdIn));
  Vertex edgeVertex = findShapeIdRecord(edgeIdIn).graphVertex;

  assert(hasShapeIdRecord(faceIdIn));
  Vertex faceVertex = findShapeIdRecord(faceIdIn).graphVertex;

  VertexAdjacencyIterator wireIt, wireItEnd;
  for (boost::tie(wireIt, wireItEnd) = boost::adjacent_vertices(faceVertex, graph); wireIt != wireItEnd; ++wireIt)
  {
    VertexAdjacencyIterator edgeIt, edgeItEnd;
    for (boost::tie(edgeIt, edgeItEnd) = boost::adjacent_vertices((*wireIt), graph); edgeIt != edgeItEnd; ++edgeIt)
    {
      if (edgeVertex == (*edgeIt))
	return findShapeIdRecord(*wireIt).id;
    }
  }
  return gu::createNilId();
}

uuid SeerShape::useGetClosestWire(const uuid& faceIn, const osg::Vec3d& pointIn) const
{
  //possible speed up. if face has only one wire, don't run the distance check.
  
  TopoDS_Vertex point = BRepBuilderAPI_MakeVertex(gp_Pnt(pointIn.x(), pointIn.y(), pointIn.z()));
  
  const ShapeIdRecord &faceInRecord = findShapeIdRecord(faceIn);
  Vertex faceVertex = faceInRecord.graphVertex;
  assert(faceInRecord.shape.ShapeType() == TopAbs_FACE);
  VertexAdjacencyIterator it, itEnd;
  uuid wireOut = gu::createNilId();
  double distance = std::numeric_limits<double>::max();
  for (boost::tie(it, itEnd) = boost::adjacent_vertices(faceVertex, graph); it != itEnd; ++it)
  {
    const ShapeIdRecord &cRecord = findShapeIdRecord(*it);
    assert(cRecord.shape.ShapeType() == TopAbs_WIRE);
    double deflection = 0.1; //hopefully makes fast. factor of bounding box?
    BRepExtrema_DistShapeShape distanceCalc(point, cRecord.shape, deflection, Extrema_ExtFlag_MIN);
    if
    (
      (!distanceCalc.IsDone()) ||
      (distanceCalc.NbSolution() < 1)
    )
      continue;
    if (distanceCalc.Value() < distance)
    {
      distance = distanceCalc.Value();
      wireOut = cRecord.id;
    }
  }
  
  return wireOut;
}

std::vector<uuid> SeerShape::useGetFacelessWires(const uuid& edgeIn) const
{
  //get first faceless wire of edge.
  std::vector<uuid> out;
  auto wires = useGetParentsOfType(edgeIn, TopAbs_WIRE);
  for (const auto &wire : wires)
  {
    auto faces = useGetParentsOfType(wire, TopAbs_FACE);
    if (faces.empty())
      out.push_back(wire);
  }
  return out;
}

bool SeerShape::useIsEdgeOfFace(const uuid& edgeIn, const uuid& faceIn) const
{
  //note edge and face might belong to totally different solids.
  
  //we know the edge will be here. Not anymore. we are now selecting
  //wires with the face first. so 'this' maybe the face feature and not the edge.
//   assert(vertexMap.count(edgeIn) > 0);
  if(!hasShapeIdRecord(edgeIn))
    return false;
  std::vector<uuid> faceParents = useGetParentsOfType(edgeIn, TopAbs_FACE);
  std::vector<uuid>::const_iterator it = std::find(faceParents.begin(), faceParents.end(), faceIn);
  return (it != faceParents.end());
}

std::vector<osg::Vec3d> SeerShape::useGetEndPoints(const uuid &edgeIdIn) const
{
  const TopoDS_Shape &shape = findShapeIdRecord(edgeIdIn).shape;
  assert(!shape.IsNull());
  assert(shape.ShapeType() == TopAbs_EDGE);
  BRepAdaptor_Curve curveAdaptor(TopoDS::Edge(shape));
  
  std::vector<osg::Vec3d> out;
  
  gp_Pnt tempPoint;
  tempPoint = curveAdaptor.Value(curveAdaptor.FirstParameter());
  out.push_back(osg::Vec3d(tempPoint.X(), tempPoint.Y(), tempPoint.Z()));
  tempPoint = curveAdaptor.Value(curveAdaptor.LastParameter());
  out.push_back(osg::Vec3d(tempPoint.X(), tempPoint.Y(), tempPoint.Z()));
  
  //I don't believe curve adaptor respects orientation.
  if(shape.Orientation() == TopAbs_REVERSED)
    std::reverse(out.begin(), out.end());
  
  return out;
}

std::vector<osg::Vec3d> SeerShape::useGetMidPoint(const uuid &edgeIdIn) const
{
  //all types of curves for mid point?
  const TopoDS_Shape &shape = findShapeIdRecord(edgeIdIn).shape;
  assert(!shape.IsNull());
  assert(shape.ShapeType() == TopAbs_EDGE);
  BRepAdaptor_Curve curveAdaptor(TopoDS::Edge(shape));
  
  std::vector<osg::Vec3d> out;
  GeomAbs_CurveType curveType = curveAdaptor.GetType();
  //no end points for conics
  if (curveType == GeomAbs_Circle || curveType == GeomAbs_Ellipse)
    return out;
  
  Standard_Real firstParameter = curveAdaptor.FirstParameter();
  Standard_Real lastParameter = curveAdaptor.LastParameter();
  Standard_Real midParameter = (lastParameter - firstParameter) / 2.0 + firstParameter;
  gp_Pnt tempPoint = curveAdaptor.Value(midParameter);
  out.push_back(osg::Vec3d(tempPoint.X(), tempPoint.Y(), tempPoint.Z()));
  
  return out;
}

std::vector< osg::Vec3d > SeerShape::useGetCenterPoint(const uuid& edgeIdIn) const
{
  const TopoDS_Shape &shape = findShapeIdRecord(edgeIdIn).shape;
  assert(!shape.IsNull());
  assert(shape.ShapeType() == TopAbs_EDGE);
  BRepAdaptor_Curve curveAdaptor(TopoDS::Edge(shape));
  
  std::vector<osg::Vec3d> out;
  GeomAbs_CurveType curveType = curveAdaptor.GetType();
  if (curveType == GeomAbs_Circle)
  {
    gp_Circ circle = curveAdaptor.Circle();
    gp_Pnt tempPoint = circle.Location();
    out.push_back(osg::Vec3d(tempPoint.X(), tempPoint.Y(), tempPoint.Z()));
  }
  if (curveType == GeomAbs_Ellipse)
  {
    gp_Elips ellipse = curveAdaptor.Ellipse();
    gp_Pnt tempPoint = ellipse.Location();
    out.push_back(osg::Vec3d(tempPoint.X(), tempPoint.Y(), tempPoint.Z()));
  }
  
  return out;
}

std::vector< osg::Vec3d > SeerShape::useGetQuadrantPoints(const uuid &edgeIdIn) const
{
  const TopoDS_Shape &shape = findShapeIdRecord(edgeIdIn).shape;
  assert(!shape.IsNull());
  assert(shape.ShapeType() == TopAbs_EDGE);
  BRepAdaptor_Curve curveAdaptor(TopoDS::Edge(shape));
  
  std::vector<osg::Vec3d> out;
  GeomAbs_CurveType curveType = curveAdaptor.GetType();
  if (curveType == GeomAbs_Circle || curveType == GeomAbs_Ellipse)
  {
    gp_Pnt tempPoint;
    tempPoint = curveAdaptor.Value(0.0);
    out.push_back(osg::Vec3d(tempPoint.X(), tempPoint.Y(), tempPoint.Z()));
    tempPoint = curveAdaptor.Value(M_PI / 2.0);
    out.push_back(osg::Vec3d(tempPoint.X(), tempPoint.Y(), tempPoint.Z()));
    tempPoint = curveAdaptor.Value(M_PI);
    out.push_back(osg::Vec3d(tempPoint.X(), tempPoint.Y(), tempPoint.Z()));
    tempPoint = curveAdaptor.Value(3.0 * M_PI / 2.0);
    out.push_back(osg::Vec3d(tempPoint.X(), tempPoint.Y(), tempPoint.Z()));
  }
  
  return out;
}

std::vector< osg::Vec3d > SeerShape::useGetNearestPoint(const uuid &shapeIn, const osg::Vec3d &pointIn) const
{
  const TopoDS_Shape &shape = findShapeIdRecord(shapeIn).shape;
  assert(!shape.IsNull());
  TopAbs_ShapeEnum type = shape.ShapeType();
  assert((type == TopAbs_EDGE) || (type == TopAbs_FACE));
  if (type != TopAbs_EDGE && type != TopAbs_FACE)
    throw std::runtime_error("expection edge or face in SeerShape::useGetNearestPoint");
  
  TopoDS_Vertex vertex = BRepBuilderAPI_MakeVertex(gp_Pnt(pointIn.x(), pointIn.y(), pointIn.z()));
  std::vector<osg::Vec3d> out;
  
  BRepExtrema_DistShapeShape dist(shape, vertex, Extrema_ExtFlag_MIN);
  if (!dist.IsDone())
    return out;
  if (dist.NbSolution() < 1)
    return out;
  gp_Pnt tempPoint = dist.PointOnShape1(1);
  out.push_back(osg::Vec3d(tempPoint.X(), tempPoint.Y(), tempPoint.Z()));
  
  return out;
}

uuid SeerShape::useGetStartVertex(const uuid &edgeIdIn) const
{
  assert(hasShapeIdRecord(edgeIdIn));
  const TopoDS_Shape& edgeShape = getOCCTShape(edgeIdIn);
  assert(edgeShape.ShapeType() == TopAbs_EDGE);
  TopoDS_Vertex v = TopExp::FirstVertex(TopoDS::Edge(edgeShape), Standard_True);
  assert(hasShapeIdRecord(v));
  return findShapeIdRecord(v).id;
}

uuid SeerShape::useGetEndVertex(const uuid &edgeIdIn) const
{
  assert(hasShapeIdRecord(edgeIdIn));
  const TopoDS_Shape& edgeShape = getOCCTShape(edgeIdIn);
  assert(edgeShape.ShapeType() == TopAbs_EDGE);
  TopoDS_Vertex v = TopExp::LastVertex(TopoDS::Edge(edgeShape), Standard_True);
  assert(hasShapeIdRecord(v));
  return findShapeIdRecord(v).id;
}

occt::ShapeVector SeerShape::useGetNonCompoundChildren() const
{
  occt::ShapeVector out;
  for (auto its = boost::vertices(graph); its.first != its.second; ++its.first)
  {
    const TopoDS_Shape &shape = findShapeIdRecord(*its.first).shape;
    if (shape.ShapeType() != TopAbs_COMPOUND)
      continue;
    for (auto aits = boost::adjacent_vertices(*its.first, graph); aits.first != aits.second; ++aits.first)
    {
      const TopoDS_Shape &subShape = findShapeIdRecord(*aits.first).shape;
      if (subShape.ShapeType() == TopAbs_COMPOUND)
        continue;
      out.push_back(subShape);
    }
  }
  return out;
}

void SeerShape::shapeMatch(const SeerShape &source)
{
  typedef ShapeIdContainer::index<ShapeIdRecord::ByShape>::type List;
  const List &list = source.shapeIdContainer.get<ShapeIdRecord::ByShape>();
  
  //every feature shape has unique id even if it is the same topoDS_shape.
  //all tracking of shapes between feature will have to use evolve container.
  for (const auto &record : list)
  {
    if (!hasShapeIdRecord(record.shape))
      continue;
    if (!findShapeIdRecord(record.shape).id.is_nil())
      continue;
    uuid freshId = gu::createNilId();
    if (hasEvolveRecordIn(record.id))
    {
      if (evolve(record.id).size() > 1)
      {
        std::cout << "WARNING: multiple evolution match in SeerShape::shapeMatch" << std::endl;
        continue;
      }
      freshId = evolve(record.id).front();
    }
    else
    {
      freshId = gu::createRandomId();
      insertEvolve(record.id, freshId);
    }
    updateShapeIdRecord(record.shape, freshId);
  }
}

void SeerShape::uniqueTypeMatch(const SeerShape &source)
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
  
  //looks for a unique shape type.
  auto getUniqueRecord = [](const ShapeIdContainer &containerIn, ShapeIdRecord &recordOut, TopAbs_ShapeEnum shapeTypeIn) -> bool
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
    ShapeIdRecord sourceRecord;
    ShapeIdRecord targetRecord;
    if
    (
      (!getUniqueRecord(source.shapeIdContainer, sourceRecord, currentShapeType)) ||
      (!getUniqueRecord(shapeIdContainer, targetRecord, currentShapeType))
    )
      continue;
      
    uuid freshId = gu::createNilId();
    if (hasEvolveRecordIn(sourceRecord.id))
      freshId = evolve(sourceRecord.id).front(); //multiple returns?
    else
    {
      freshId = gu::createRandomId();
      insertEvolve(sourceRecord.id, freshId);
    }
    updateShapeIdRecord(targetRecord.shape, freshId);
    if (currentShapeType == TopAbs_COMPOUND) //no compound of compounds? I think not.
      setRootShapeId(freshId);
  }
}

void SeerShape::outerWireMatch(const SeerShape &source)
{
  for (const auto &id : getAllShapeIds())
  {
    const ShapeIdRecord &faceRecord = findShapeIdRecord(id);
    if (faceRecord.shape.ShapeType() != TopAbs_FACE)
      continue;
    const TopoDS_Shape &thisOuterWire = BRepTools::OuterWire(TopoDS::Face(faceRecord.shape));
    assert(hasShapeIdRecord(thisOuterWire)); //shouldn't have a result container with a face and no outer wire.
    const ShapeIdRecord &wireRecord = findShapeIdRecord(thisOuterWire);
    if (!wireRecord.id.is_nil())
      continue; //only set id for nil wires.
      
    if (!hasEvolveRecordOut(faceRecord.id))
      continue;
    uuid sourceFaceId = devolve(faceRecord.id).front();
      
    //now find entries in source.
    if (!source.hasShapeIdRecord(sourceFaceId))
      continue;
    const ShapeIdRecord &sourceFaceRecord = source.findShapeIdRecord(sourceFaceId);
    const TopoDS_Shape &sourceOuterWire = BRepTools::OuterWire(TopoDS::Face(sourceFaceRecord.shape));
    assert(!sourceOuterWire.IsNull());
    auto sourceWireId = source.findShapeIdRecord(sourceOuterWire).id;
    
    uuid freshId;
    if (hasEvolveRecordIn(sourceWireId))
      freshId = evolve(sourceWireId).front(); //multiple returns?
    else
    {
      freshId = gu::createRandomId();
      insertEvolve(sourceWireId, freshId);
    }
    updateShapeIdRecord(thisOuterWire, freshId);
  }
}

void SeerShape::modifiedMatch
(
  BRepBuilderAPI_MakeShape &shapeMakerIn,
  const SeerShape &source
)
{
  typedef ShapeIdContainer::index<ShapeIdRecord::ById>::type List;
  const List &sourceList = source.shapeIdContainer.get<ShapeIdRecord::ById>();
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
      if(!hasShapeIdRecord(it.Value()))
        continue;
      
      uuid freshId = gu::createNilId();
      if (hasEvolveRecordIn(sourceRecord.id))
        freshId = evolve(sourceRecord.id).front(); //multiple returns?
      else
      {
        freshId = gu::createRandomId();
        insertEvolve(sourceRecord.id, freshId);
      }
      updateShapeIdRecord(it.Value(), freshId);
    }
  }
}

void SeerShape::derivedMatch()
{
  typedef ShapeIdContainer::index<ShapeIdRecord::ById>::type List;
  List &list = shapeIdContainer.get<ShapeIdRecord::ById>();
  occt::ShapeVector nilShapes;
  auto rangeItPair = list.equal_range(gu::createNilId());
  for (; rangeItPair.first != rangeItPair.second; ++rangeItPair.first)
    nilShapes.push_back(rangeItPair.first->shape);
  
  auto match = [&](TopAbs_ShapeEnum shapeType, TopAbs_ShapeEnum parentType)
  {
    for (const auto &shape : nilShapes)
    {
      if (shape.ShapeType() != shapeType)
        continue;
      
      bool bail = false;
      IdSet set;
      occt::ShapeVector parents = useGetParentsOfType(shape, parentType);
      for (const auto &parent : parents)
      {
        assert(hasShapeIdRecord(parent));
        boost::uuids::uuid id = findShapeIdRecord(parent).id;
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
      uuid id = gu::createNilId();
      DerivedContainer::iterator derivedIt = derivedContainer.find(set);
      if (derivedIt == derivedContainer.end())
      {
        id = gu::createRandomId();
        DerivedContainer::value_type newEntry(set, id);
        derivedContainer.insert(newEntry);
        insertEvolve(gu::createNilId(), id);
      }
      else
      {
        id = derivedIt->second;
      }
      updateShapeIdRecord(shape, id);
    }
  };
  
  match(TopAbs_EDGE, TopAbs_FACE);
  match(TopAbs_VERTEX, TopAbs_EDGE);
}

void SeerShape::dumpNils(const std::string &headerIn)
{
  std::ostringstream stream;
  
  typedef ShapeIdContainer::index<ShapeIdRecord::ById>::type List;
  const List &list = shapeIdContainer.get<ShapeIdRecord::ById>();
  auto rangeItPair = list.equal_range(gu::createNilId());
  for (; rangeItPair.first != rangeItPair.second; ++rangeItPair.first)
    stream << gu::idToString(rangeItPair.first->id) << "    "
    << rangeItPair.first->shape << std::endl;
  
  
  if (!stream.str().empty())
    std::cout << std::endl << headerIn << " nils" << std::endl << stream.str() << std::endl;
}

void SeerShape::dumpDuplicates(const std::string &headerIn)
{
  std::ostringstream stream;
  
  std::set<boost::uuids::uuid> processed;
  typedef ShapeIdContainer::index<ShapeIdRecord::ById>::type List;
  const List &list = shapeIdContainer.get<ShapeIdRecord::ById>();
  for (const auto &record : list)
  {
    if (processed.count(record.id) > 0)
      continue;
    std::size_t count = shapeIdContainer.count(record.id);
    if (count > 1)
    {
      processed.insert(record.id);
      stream << gu::idToString(record.id) << "    " << count << "    " << record.shape << std::endl;
    }
  }
  if (!stream.str().empty())
    std::cout << std::endl << headerIn << " duplicates" << std::endl << stream.str() << std::endl;
}

void SeerShape::ensureNoNils()
{
  occt::ShapeVector nilShapes;
  
  typedef ShapeIdContainer::index<ShapeIdRecord::ById>::type List;
  List &list = shapeIdContainer.get<ShapeIdRecord::ById>();
  auto rangeItPair = list.equal_range(gu::createNilId());
  for (; rangeItPair.first != rangeItPair.second; ++rangeItPair.first)
    nilShapes.push_back(rangeItPair.first->shape);
  
  for (const auto &shape : nilShapes)
  {
    auto freshId = gu::createRandomId();
    updateShapeIdRecord(shape, freshId);
    insertEvolve(gu::createNilId(), freshId);
  }
}

void SeerShape::ensureNoDuplicates()
{
  std::set<boost::uuids::uuid> processed;
  occt::ShapeVector shapes;
  typedef ShapeIdContainer::index<ShapeIdRecord::ById>::type List;
  const List &list = shapeIdContainer.get<ShapeIdRecord::ById>();
  for (const auto &record : list)
  {
    if (processed.count(record.id) > 0)
      shapes.push_back(record.shape);
    else
      processed.insert(record.id);
  }
  for (const auto &shape : shapes)
  {
    auto freshId = gu::createRandomId();
    updateShapeIdRecord(shape, freshId);
    insertEvolve(gu::createNilId(), freshId);
  }
}

void SeerShape::faceEdgeMatch(const SeerShape &source)
{
  using boost::uuids::uuid;
  
  occt::ShapeVector nilEdges;
  typedef ShapeIdContainer::index<ShapeIdRecord::ById>::type List;
  List &list = shapeIdContainer.get<ShapeIdRecord::ById>();
  auto rangeItPair = list.equal_range(gu::createNilId());
  for (; rangeItPair.first != rangeItPair.second; ++rangeItPair.first)
  {
    if (rangeItPair.first->shape.ShapeType() != TopAbs_EDGE)
      continue;
    nilEdges.push_back(rangeItPair.first->shape);
  }
  
  for (const auto &nilEdge : nilEdges)
  {
    //get 2 parent face ids. need 2 to make edge unique.
    occt::ShapeVector parentFaces = useGetParentsOfType(nilEdge, TopAbs_FACE);
    if (parentFaces.size() != 2)
      continue;
    std::vector<uuid> parentFacesIds;
    parentFacesIds.push_back(findShapeIdRecord(parentFaces.front()).id);
    parentFacesIds.push_back(findShapeIdRecord(parentFaces.back()).id);
    if
    (
      (parentFacesIds.front().is_nil()) ||
      (parentFacesIds.back().is_nil())
    )
      continue;
    
    if
    (!(
      (source.hasShapeIdRecord(parentFacesIds.front())) &&
      (source.hasShapeIdRecord(parentFacesIds.back()))
    ))
      continue;
    
    std::vector<uuid> face1Edges = source.useGetChildrenOfType(parentFacesIds.front(), TopAbs_EDGE);
    std::vector<uuid> face2Edges = source.useGetChildrenOfType(parentFacesIds.back(), TopAbs_EDGE);
    std::vector<uuid> commonEdges;
    std::sort(face1Edges.begin(), face1Edges.end());
    std::sort(face2Edges.begin(), face2Edges.end());
    std::set_intersection
    (
      face1Edges.begin(), face1Edges.end(),
      face2Edges.begin(), face2Edges.end(), 
      std::back_inserter(commonEdges)
    );

    if (commonEdges.size() == 1)
    {
      uuid freshId = gu::createNilId();
      if (hasEvolveRecordIn(commonEdges.front()))
        freshId = evolve(commonEdges.front()).front(); //multiple returns?
      else
      {
        freshId = gu::createRandomId();
        insertEvolve(commonEdges.front(), freshId);
      }
      updateShapeIdRecord(nilEdge, freshId);
    }
  }
}

void SeerShape::edgeVertexMatch(const SeerShape &source)
{
  using boost::uuids::uuid;
  
  occt::ShapeVector nilVertices;
  typedef ShapeIdContainer::index<ShapeIdRecord::ById>::type List;
  List &list = shapeIdContainer.get<ShapeIdRecord::ById>();
  auto rangeItPair = list.equal_range(gu::createNilId());
  for (; rangeItPair.first != rangeItPair.second; ++rangeItPair.first)
  {
    if (rangeItPair.first->shape.ShapeType() != TopAbs_VERTEX)
      continue;
    nilVertices.push_back(rangeItPair.first->shape);
  }
  
  for (const auto &nilVertex : nilVertices)
  {
    bool nilDetected = false;
    bool missingInSource = false;
    occt::ShapeVector parentEdges = useGetParentsOfType(nilVertex, TopAbs_EDGE);
    std::vector<uuid> parentEdgeIds;
    for (const auto &pEdge : parentEdges)
      parentEdgeIds.push_back(findShapeIdRecord(pEdge).id);
    std::vector<std::vector<uuid> >sourceVertices;
    
    for (const auto parentEdgeId : parentEdgeIds)
    {
      if (parentEdgeId.is_nil())
      {
        nilDetected = true;
        break;
      }
      if (!source.hasShapeIdRecord(parentEdgeId))
      {
        missingInSource = true;
        break;
      }
      
      std::vector<uuid> tempVertexIds = source.useGetChildrenOfType(parentEdgeId, TopAbs_VERTEX);
      std::sort(tempVertexIds.begin(), tempVertexIds.end());
      sourceVertices.push_back(tempVertexIds);
    }
    if (nilDetected || missingInSource || sourceVertices.size() < 2)
      continue;
    
    std::vector<uuid> intersectedVertices = sourceVertices.back();
    sourceVertices.pop_back();
    for (const auto &sourceGroup : sourceVertices)
    {
      std::vector<uuid> temp;
      std::set_intersection
      (
        sourceGroup.begin(), sourceGroup.end(),
        intersectedVertices.begin(), intersectedVertices.end(), 
        std::back_inserter(temp)
      );
      intersectedVertices = temp;
    }
    
    if (intersectedVertices.size() == 1)
    {
      uuid freshId = gu::createNilId();
      if (hasEvolveRecordIn(intersectedVertices.front()))
        freshId = evolve(intersectedVertices.front()).front(); //multiple returns?
      else
      {
        freshId = gu::createRandomId();
        insertEvolve(intersectedVertices.front(), freshId);
      }
      updateShapeIdRecord(nilVertex, freshId);
    }
  }
}

void SeerShape::dumpGraph(const std::string &filePathIn) const
{
  std::ofstream file(filePathIn.c_str());
  boost::write_graphviz(file, graph, Node_writer<Graph>(graph, *this), boost::default_writer());
}

void SeerShape::dumpReverseGraph(const std::string &filePathIn) const
{
  std::ofstream file(filePathIn.c_str());
  boost::write_graphviz(file, rGraph, Node_writer<Graph>(rGraph, *this), boost::default_writer());
}

void SeerShape::dumpShapeIdContainer(std::ostream &streamIn) const
{
  streamIn << shapeIdContainer << std::endl;
}

void SeerShape::dumpEvolveContainer(std::ostream &streamIn) const
{
  streamIn << evolveContainer << std::endl;
}

void SeerShape::dumpFeatureTagContainer(std::ostream &streamIn) const
{
  streamIn << featureTagContainer << std::endl;
}

prj::srl::SeerShape SeerShape::serialOut()
{
  prj::srl::ShapeIdContainer shapeIdContainerOut;
  if (!rootShapeId.is_nil())
  {
    const TopoDS_Shape &shape = getRootOCCTShape();
    if (!shape.IsNull())
    {
      //if mapShapes only contains subshapes, like the doc says, we don't write out compound?
      occt::ShapeVector shapes = occt::mapShapes(shape);
      std::size_t count = 0;
      for (const auto &s : shapes)
      {
        if (!hasShapeIdRecord(s))
        {
          std::cerr << "WARNING: ShapeId Container doesn't have shape in SeerShape::serialOut" << std::endl;
          count++;
          continue;
        }
        prj::srl::ShapeIdRecord rRecord
        (
          gu::idToString(findShapeIdRecord(s).id),
          count
        );
        shapeIdContainerOut.shapeIdRecord().push_back(rRecord);
        count++;
      }
    }
  }
  
  prj::srl::EvolveContainer eContainerOut;
  typedef EvolveContainer::index<EvolveRecord::ByInId>::type EList;
  const EList &eList = evolveContainer.get<EvolveRecord::ByInId>();
  for (EList::const_iterator it = eList.begin(); it != eList.end(); ++it)
  {
    prj::srl::EvolveRecord eRecord
    (
      gu::idToString(it->inId),
      gu::idToString(it->outId)
    );
    eContainerOut.evolveRecord().push_back(eRecord);
  }
  
  prj::srl::FeatureTagContainer fContainerOut;
  typedef FeatureTagContainer::index<FeatureTagRecord::ById>::type FList;
  const FList &fList = featureTagContainer.get<FeatureTagRecord::ById>();
  for (FList::const_iterator it = fList.begin(); it != fList.end(); ++it)
  {
    prj::srl::FeatureTagRecord fRecord
    (
      gu::idToString(it->id),
      it->tag
    );
    fContainerOut.featureTagRecord().push_back(fRecord);
  }
  
  prj::srl::DerivedContainer dContainerOut;
  for (DerivedContainer::const_iterator dIt = derivedContainer.begin(); dIt != derivedContainer.end(); ++dIt)
  {
    prj::srl::IdSet setIn;
    for (IdSet::const_iterator sIt = dIt->first.begin(); sIt != dIt->first.end(); ++sIt)
      setIn.id().push_back(gu::idToString(*sIt));
    prj::srl::DerivedRecord::IdType mId(gu::idToString(dIt->second));
    prj::srl::DerivedRecord record
    (
      setIn,
      mId
    );
    
    dContainerOut.derivedRecord().push_back(record);
  }
  
  return prj::srl::SeerShape
  (
    gu::idToString(rootShapeId),
    shapeIdContainerOut,
    eContainerOut,
    fContainerOut,
    dContainerOut
  ); 
}

void SeerShape::serialIn(const prj::srl::SeerShape &sSeerShapeIn)
{
  //note: shape has already been set through setOCCTShape, so shapeIdContainer has been populated and don't clear.
  //setOCCTShape assigns an id for the root, so that is valid.
  
  //fill in shapeId container.
  occt::ShapeVector shapes = occt::mapShapes(getRootOCCTShape());
  for (const prj::srl::ShapeIdRecord &sRRecord : sSeerShapeIn.shapeIdContainer().shapeIdRecord())
  {
    std::size_t offset = sRRecord.shapeOffset();
    assert(offset < shapes.size());
    if (offset >= shapes.size())
    {
      std::cerr << "WARNING: invalid shape offset in SeerShape::serialIn" << std::endl;
      continue;
    }
    updateShapeIdRecord(shapes.at(sRRecord.shapeOffset()), gu::stringToId(sRRecord.id()));
  }
  
  evolveContainer.get<EvolveRecord::ByInId>().clear();
  for (const prj::srl::EvolveRecord &sERecord : sSeerShapeIn.evolveContainer().evolveRecord())
  {
    EvolveRecord record;
    record.inId = gu::stringToId(sERecord.idIn());
    record.outId = gu::stringToId(sERecord.idOut());
    evolveContainer.insert(record);
  }
  
  featureTagContainer.get<FeatureTagRecord::ById>().clear();
  for (const prj::srl::FeatureTagRecord &sFRecord : sSeerShapeIn.featureTagContainer().featureTagRecord())
  {
    FeatureTagRecord record;
    record.id = gu::stringToId(sFRecord.id());
    record.tag = sFRecord.tag();
    featureTagContainer.insert(record);
  }
  
  derivedContainer.clear();
  for (const prj::srl::DerivedRecord &sDRecord : sSeerShapeIn.derivedContainer().derivedRecord())
  {
    IdSet setIn;
    for (const auto &idSet : sDRecord.idSet().id())
      setIn.insert(gu::stringToId(idSet));
    boost::uuids::uuid mId = gu::stringToId(sDRecord.id());
    derivedContainer.insert(std::make_pair(setIn, mId));
  }
  
  rootShapeId = gu::stringToId(sSeerShapeIn.rootShapeId());
}

SeerShape SeerShape::createWorkCopy() const
{
  SeerShape target;
  
  BRepBuilderAPI_Copy copier;
  copier.Perform(getRootOCCTShape());
  target.setOCCTShape(copier.Shape());
  target.ensureNoNils(); //give all shapes a new id.
  //ensureNoNils fills in the evolve container also. we have to clear it.
  target.evolveContainer.get<EvolveRecord::ByInId>().clear();
  
  for (const auto &sourceId : getAllShapeIds())
  {
    const TopoDS_Shape &sourceShape = findShapeIdRecord(sourceId).shape;
    TopoDS_Shape targetShape = copier.ModifiedShape(sourceShape);
    assert(target.hasShapeIdRecord(targetShape));
    uuid targetId = target.findShapeIdRecord(targetShape).id;
    target.insertEvolve(sourceId, targetId);
  }
  
  return target;
}

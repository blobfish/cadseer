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

#include <iostream>
#include <assert.h>

#include <boost/graph/graphviz.hpp>
#include <boost/graph/iteration_macros.hpp>

#include <TopoDS_Iterator.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <TopoDS.hxx>
#include <gp_Circ.hxx>
#include <gp_Elips.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>

#include <application.h>
#include <globalutilities.h>
#include <modelviz/connector.h>

using namespace mdv;
using namespace cng;
using namespace boost::uuids;

void Connector::buildStartNode(const TopoDS_Shape &shapeIn, const ftr::ResultContainer &resultContainerIn)
{
    uuid shapeId = findResultByShape(resultContainerIn, shapeIn).id;
    cng::IdVertexMap::iterator it = vertexMap.find(shapeId);
    Vertex previous;
    bool firstNode = vertexStack.empty();
    if (!firstNode)
        previous = vertexStack.top();
    if (it == vertexMap.end())
    {
        Vertex newVertex = boost::add_vertex(graph);
        vertexStack.push(newVertex);
        buildAddShape(shapeIn, resultContainerIn);
    }
    else
        vertexStack.push(it->second);
    if (!firstNode)
        connectVertices(previous, vertexStack.top());
}

void Connector::buildAddShape(const TopoDS_Shape &shapeIn, const ftr::ResultContainer &resultContainerIn)
{
    assert(!vertexStack.empty());
    graph[vertexStack.top()].id = findResultByShape(resultContainerIn, shapeIn).id;
    graph[vertexStack.top()].shapeType = shapeIn.ShapeType();
    graph[vertexStack.top()].shape = shapeIn;
    vertexMap.insert(std::make_pair(graph[vertexStack.top()].id, vertexStack.top()));
}

void Connector::buildEndNode()
{
    vertexStack.pop();
}

void Connector::connectVertices(cng::Vertex from, cng::Vertex to)
{
    bool edgeResult;
    Edge edge;
    boost::tie(edge, edgeResult) = boost::add_edge(from, to, graph);
    assert(edgeResult);
}

std::vector<boost::uuids::uuid> Connector::useGetParentsOfType
  (const boost::uuids::uuid &idIn, const TopAbs_ShapeEnum &shapeTypeIn) const
{
    cng::Graph temp = graph;
    cng::GraphReversed reversed = boost::make_reverse_graph(temp);

    cng::IdVertexMap::const_iterator it;
    it = vertexMap.find(idIn);
    assert(it != vertexMap.end());

    std::vector<cng::Vertex> vertices;
    TypeCollectionVisitor vis(shapeTypeIn, vertices);
    boost::breadth_first_search(reversed, it->second, boost::visitor(vis));

    std::vector<cng::Vertex>::const_iterator vit;
    std::vector<uuid> idsOut;
    for (vit = vertices.begin(); vit != vertices.end(); ++vit)
        idsOut.push_back(reversed[*vit].id);
    return idsOut;
}

std::vector<boost::uuids::uuid> Connector::useGetChildrenOfType
  (const boost::uuids::uuid &idIn, const TopAbs_ShapeEnum &shapeType) const
{
    cng::IdVertexMap::const_iterator it;
    it = vertexMap.find(idIn);
    assert(it != vertexMap.end());

    std::vector<cng::Vertex> vertices;
    TypeCollectionVisitor vis(shapeType, vertices);
    boost::breadth_first_search(graph, it->second, boost::visitor(vis));

    std::vector<cng::Vertex>::const_iterator vit;
    std::vector<uuid> idsOut;
    for (vit = vertices.begin(); vit != vertices.end(); ++vit)
        idsOut.push_back(graph[*vit].id);
    return idsOut;
}

boost::uuids::uuid Connector::useGetWire
  (const boost::uuids::uuid &edgeIdIn, const boost::uuids::uuid &faceIdIn) const
{
    cng::IdVertexMap::const_iterator it;
    it = vertexMap.find(edgeIdIn);
    assert(it != vertexMap.end());
    cng::Vertex edgeVertex = it->second;

    it = vertexMap.find(faceIdIn);
    assert(it != vertexMap.end());
    cng::Vertex faceVertex = it->second;

    cng::Vertex wireVertex;
    cng::VertexAdjacencyIterator wireIt, wireItEnd;
    for (boost::tie(wireIt, wireItEnd) = boost::adjacent_vertices(faceVertex, graph); wireIt != wireItEnd; ++wireIt)
    {
        cng::VertexAdjacencyIterator edgeIt, edgeItEnd;
        for (boost::tie(edgeIt, edgeItEnd) = boost::adjacent_vertices((*wireIt), graph); edgeIt != edgeItEnd; ++edgeIt)
        {
            if (edgeVertex == (*edgeIt))
            {
                wireVertex = *wireIt;
                return graph[wireVertex].id;
            }
        }
    }
    return boost::uuids::nil_generator()();
}

uuid Connector::useGetRoot() const
{
  std::vector<cng::Vertex> roots;
  BGL_FORALL_VERTICES(currentVertex, graph, cng::Graph)
  {
    if (boost::in_degree(currentVertex, graph) == 0)
      roots.push_back(currentVertex);
  }
  
  //all features/objects are expected to have 1 root shape
  //of the type compound. If this isn't the case something is wrong.
  //shape might be empty though. Like a failed update.
  assert(roots.size() < 2);
  
  if (roots.empty())
    return boost::uuids::nil_generator()();
  return graph[roots.at(0)].id;
}

bool Connector::useIsEdgeOfFace(const uuid& edgeIn, const uuid& faceIn) const
{
  //note edge and face might belong to totally different solids.
  
  //we know the edge will be here. Not anymore. we are now selecting
  //wires with the face first. so 'this' maybe the face feature and not the edge.
//   assert(vertexMap.count(edgeIn) > 0);
  if(vertexMap.count(edgeIn) < 1)
    return false;
  std::vector<uuid> faceParents = useGetParentsOfType(edgeIn, TopAbs_FACE);
  std::vector<uuid>::const_iterator it = std::find(faceParents.begin(), faceParents.end(), faceIn);
  return (it != faceParents.end());
}

//do I really  need this?
TopoDS_Shape Connector::getShape(const boost::uuids::uuid &idIn) const
{
    cng::IdVertexMap::const_iterator it;
    it = vertexMap.find(idIn);
    assert(it != vertexMap.end());
    return graph[it->second].shape;
}

void Connector::outputGraphviz()
{
  //something here to check preferences about writing this out.
  QString fileName = static_cast<app::Application *>(qApp)->getApplicationDirectory().path();
  fileName += QDir::separator();
  fileName += "connector.dot";
  std::ofstream file(fileName.toStdString().c_str());

  boost::write_graphviz(file, graph, Node_writer<cng::Graph>(graph), boost::default_writer());
}

std::vector<osg::Vec3d> Connector::useGetEndPoints(const boost::uuids::uuid &edgeIdIn) const
{
  TopoDS_Shape shape = getShape(edgeIdIn);
  assert(!shape.IsNull());
  assert(shape.ShapeType() == TopAbs_EDGE);
  BRepAdaptor_Curve curveAdaptor(TopoDS::Edge(shape));
  
  std::vector<osg::Vec3d> out;
  GeomAbs_CurveType curveType = curveAdaptor.GetType();
  //no end points for conics
  if (curveType == GeomAbs_Circle || curveType == GeomAbs_Ellipse)
    return out;
  
  gp_Pnt tempPoint;
  tempPoint = curveAdaptor.Value(curveAdaptor.FirstParameter());
  out.push_back(osg::Vec3d(tempPoint.X(), tempPoint.Y(), tempPoint.Z()));
  tempPoint = curveAdaptor.Value(curveAdaptor.LastParameter());
  out.push_back(osg::Vec3d(tempPoint.X(), tempPoint.Y(), tempPoint.Z()));
  
  return out;
}

std::vector<osg::Vec3d> Connector::useGetMidPoint(const uuid &edgeIdIn) const
{
  //all types of curves for mid point?
  TopoDS_Shape shape = getShape(edgeIdIn);
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

std::vector< osg::Vec3d > Connector::useGetCenterPoint(const uuid& edgeIdIn) const
{
  TopoDS_Shape shape = getShape(edgeIdIn);
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

std::vector< osg::Vec3d > Connector::useGetQuadrantPoints(const uuid &edgeIdIn) const
{
  TopoDS_Shape shape = getShape(edgeIdIn);
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

std::vector< osg::Vec3d > Connector::useGetNearestPoint(const uuid &shapeIn, const osg::Vec3d &pointIn) const
{
  TopoDS_Shape shape = getShape(shapeIn);
  assert(!shape.IsNull());
  TopAbs_ShapeEnum type = shape.ShapeType();
  assert((type == TopAbs_EDGE) || (type == TopAbs_FACE));
  
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



//when we build the ids in features we treat seam edges as one edge
//through maps. However, here we are using topods_iterator which finds both
//edges of a seam. so when we look for the shape in the
//result map we fail as one of the 2 seam edges isn't in the map.
BuildConnector::BuildConnector(const TopoDS_Shape &shapeIn, const ftr::ResultContainer &resultContainerIn) : connector()
{
    connector.buildStartNode(shapeIn, resultContainerIn);
    buildRecursiveConnector(shapeIn, resultContainerIn);
    connector.buildEndNode();
}

void BuildConnector::buildRecursiveConnector(const TopoDS_Shape &shapeIn, const ftr::ResultContainer &resultContainerIn)
{
    for (TopoDS_Iterator it(shapeIn); it.More(); it.Next())
    {
        const TopoDS_Shape &currentShape = it.Value();
        if (!(hasResult(resultContainerIn, currentShape)))
          continue; //yuck. see note above. probably seam edge.

        connector.buildStartNode(currentShape, resultContainerIn);
        if (currentShape.ShapeType() != TopAbs_VERTEX)
            buildRecursiveConnector(currentShape, resultContainerIn);
        connector.buildEndNode();
    }
}


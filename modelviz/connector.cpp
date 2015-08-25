#include <iostream>
#include <assert.h>
#include <boost/graph/graphviz.hpp>

#include <TopoDS_Iterator.hxx>

#include "../globalutilities.h"
#include "connector.h"

using namespace ModelViz;
using namespace ConnectorGraph;
using namespace boost::uuids;

void Connector::buildStartNode(const TopoDS_Shape &shapeIn, const Feature::ResultContainer &resultContainerIn)
{
    uuid shapeId = Feature::findResultByShape(resultContainerIn, shapeIn).id;
    ConnectorGraph::IdVertexMap::iterator it = vertexMap.find(shapeId);
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

void Connector::buildAddShape(const TopoDS_Shape &shapeIn, const Feature::ResultContainer &resultContainerIn)
{
    assert(!vertexStack.empty());
    graph[vertexStack.top()].id = Feature::findResultByShape(resultContainerIn, shapeIn).id;
    graph[vertexStack.top()].shapeType = shapeIn.ShapeType();
    graph[vertexStack.top()].shape = shapeIn;
    vertexMap.insert(std::make_pair(graph[vertexStack.top()].id, vertexStack.top()));
}

void Connector::buildEndNode()
{
    vertexStack.pop();
}

void Connector::connectVertices(ConnectorGraph::Vertex from, ConnectorGraph::Vertex to)
{
    bool edgeResult;
    Edge edge;
    boost::tie(edge, edgeResult) = boost::add_edge(from, to, graph);
    assert(edgeResult);
}

std::vector<boost::uuids::uuid> Connector::useGetParentsOfType
  (const boost::uuids::uuid &idIn, const TopAbs_ShapeEnum &shapeTypeIn) const
{
    ConnectorGraph::Graph temp = graph;
    ConnectorGraph::GraphReversed reversed = boost::make_reverse_graph(temp);

    ConnectorGraph::IdVertexMap::const_iterator it;
    it = vertexMap.find(idIn);
    assert(it != vertexMap.end());

    std::vector<ConnectorGraph::Vertex> vertices;
    TypeCollectionVisitor vis(shapeTypeIn, vertices);
    boost::breadth_first_search(reversed, it->second, boost::visitor(vis));

    std::vector<ConnectorGraph::Vertex>::const_iterator vit;
    std::vector<uuid> idsOut;
    for (vit = vertices.begin(); vit != vertices.end(); ++vit)
        idsOut.push_back(reversed[*vit].id);
    return idsOut;
}

std::vector<boost::uuids::uuid> Connector::useGetChildrenOfType
  (const boost::uuids::uuid &idIn, const TopAbs_ShapeEnum &shapeType) const
{
    ConnectorGraph::IdVertexMap::const_iterator it;
    it = vertexMap.find(idIn);
    assert(it != vertexMap.end());

    std::vector<ConnectorGraph::Vertex> vertices;
    TypeCollectionVisitor vis(shapeType, vertices);
    boost::breadth_first_search(graph, it->second, boost::visitor(vis));

    std::vector<ConnectorGraph::Vertex>::const_iterator vit;
    std::vector<uuid> idsOut;
    for (vit = vertices.begin(); vit != vertices.end(); ++vit)
        idsOut.push_back(graph[*vit].id);
    return idsOut;
}

boost::uuids::uuid Connector::useGetWire
  (const boost::uuids::uuid &edgeIdIn, const boost::uuids::uuid &faceIdIn) const
{
    ConnectorGraph::IdVertexMap::const_iterator it;
    it = vertexMap.find(edgeIdIn);
    assert(it != vertexMap.end());
    ConnectorGraph::Vertex edgeVertex = it->second;

    it = vertexMap.find(faceIdIn);
    assert(it != vertexMap.end());
    ConnectorGraph::Vertex faceVertex = it->second;

    ConnectorGraph::Vertex wireVertex;
    ConnectorGraph::VertexAdjacencyIterator wireIt, wireItEnd;
    for (boost::tie(wireIt, wireItEnd) = boost::adjacent_vertices(faceVertex, graph); wireIt != wireItEnd; ++wireIt)
    {
        ConnectorGraph::VertexAdjacencyIterator edgeIt, edgeItEnd;
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

//do I really  need this?
TopoDS_Shape Connector::getShape(const boost::uuids::uuid &idIn)
{
    ConnectorGraph::IdVertexMap::const_iterator it;
    it = vertexMap.find(idIn);
    assert(it != vertexMap.end());
    return graph[it->second].shape;
}

void Connector::outputGraphviz(const std::string &name)
{
    std::string fileName;
    fileName += "./";
    fileName += name;
    fileName += ".dot";
    std::ofstream file(fileName);

    //forward graph
    boost::write_graphviz(file, graph, Node_writer<ConnectorGraph::Graph>(graph), boost::default_writer());

    //reversed graph
//    ConnectorGraph::GraphReversed reversed = boost::make_reverse_graph(graph);
//    boost::write_graphviz(file, reversed, Node_writer<ConnectorGraph::GraphReversed>(reversed), boost::default_writer());
}


BuildConnector::BuildConnector(const TopoDS_Shape &shapeIn, const Feature::ResultContainer &resultContainerIn) : connector()
{
    connector.buildStartNode(shapeIn, resultContainerIn);
    buildRecursiveConnector(shapeIn, resultContainerIn);
    connector.buildEndNode();
}

void BuildConnector::buildRecursiveConnector(const TopoDS_Shape &shapeIn, const Feature::ResultContainer &resultContainerIn)
{
    for (TopoDS_Iterator it(shapeIn); it.More(); it.Next())
    {
        const TopoDS_Shape &currentShape = it.Value();

        connector.buildStartNode(currentShape, resultContainerIn);
        if (currentShape.ShapeType() != TopAbs_VERTEX)
            buildRecursiveConnector(currentShape, resultContainerIn);
        connector.buildEndNode();
    }
}

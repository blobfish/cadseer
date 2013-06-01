#include <iostream>
#include <assert.h>
#include <boost/graph/graphviz.hpp>

#include <TopoDS_Iterator.hxx>

#include "../globalutilities.h"
#include "connector.h"

using namespace ModelViz;
using namespace ConnectorGraph;

Connector::Connector()
{
}

void Connector::buildStartNode(const TopoDS_Shape &shapeIn)
{
    int shapeHash = GU::getShapeHash(shapeIn);
    ConnectorGraph::HashVertexMap::iterator it = vertexMap.find(shapeHash);
    Vertex previous;
    bool firstNode = vertexStack.empty();
    if (!firstNode)
        previous = vertexStack.top();
    if (it == vertexMap.end())
    {
        Vertex newVertex = boost::add_vertex(graph);
        vertexStack.push(newVertex);
        buildAddShape(shapeIn);
    }
    else
        vertexStack.push(it->second);
    if (!firstNode)
        connectVertices(previous, vertexStack.top());
}

void Connector::buildAddShape(const TopoDS_Shape &shapeIn)
{
    assert(!vertexStack.empty());
    int shapeHash = GU::getShapeHash(shapeIn);
    graph[vertexStack.top()].hash = shapeHash;
    graph[vertexStack.top()].shapeType = shapeIn.ShapeType();
    graph[vertexStack.top()].shape = shapeIn;
    vertexMap.insert(std::make_pair(shapeHash, vertexStack.top()));
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

std::vector<int> Connector::useGetParentsOfType(const int &shapeHash, const TopAbs_ShapeEnum &shapeType) const
{
    ConnectorGraph::Graph temp = graph;
    ConnectorGraph::GraphReversed reversed = boost::make_reverse_graph(temp);

    ConnectorGraph::HashVertexMap::const_iterator it;
    it = vertexMap.find(shapeHash);
    assert(it != vertexMap.end());

    std::vector<ConnectorGraph::Vertex> vertices;
    TypeCollectionVisitor vis(shapeType, vertices);
    boost::breadth_first_search(reversed, it->second, boost::visitor(vis));

    std::vector<ConnectorGraph::Vertex>::const_iterator vit;
    std::vector<int> hashesOut;
    for (vit = vertices.begin(); vit != vertices.end(); ++vit)
        hashesOut.push_back(reversed[*vit].hash);
    return hashesOut;
}

std::vector<int> Connector::useGetChildrenOfType(const int &shapeHash, const TopAbs_ShapeEnum &shapeType) const
{
    ConnectorGraph::HashVertexMap::const_iterator it;
    it = vertexMap.find(shapeHash);
    assert(it != vertexMap.end());

    std::vector<ConnectorGraph::Vertex> vertices;
    TypeCollectionVisitor vis(shapeType, vertices);
    boost::breadth_first_search(graph, it->second, boost::visitor(vis));

    std::vector<ConnectorGraph::Vertex>::const_iterator vit;
    std::vector<int> hashesOut;
    for (vit = vertices.begin(); vit != vertices.end(); ++vit)
        hashesOut.push_back(graph[*vit].hash);
    return hashesOut;
}

int Connector::useGetWire(const int &edgeHash, const int &faceHash) const
{
    ConnectorGraph::HashVertexMap::const_iterator it;
    it = vertexMap.find(edgeHash);
    assert(it != vertexMap.end());
    ConnectorGraph::Vertex edgeVertex = it->second;

    it = vertexMap.find(faceHash);
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
                return graph[wireVertex].hash;
            }
        }
    }
    return 0;
}

TopoDS_Shape Connector::getShape(const int &shapeHash)
{
    ConnectorGraph::HashVertexMap::const_iterator it;
    it = vertexMap.find(shapeHash);
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


BuildConnector::BuildConnector(const TopoDS_Shape &root)
{
    connector.buildStartNode(root);
    buildRecursiveConnector(root);
    connector.buildEndNode();
}

void BuildConnector::buildRecursiveConnector(const TopoDS_Shape &shapeIn)
{
    for (TopoDS_Iterator it(shapeIn); it.More(); it.Next())
    {
        const TopoDS_Shape &currentShape = it.Value();

        connector.buildStartNode(currentShape);
        if (currentShape.ShapeType() != TopAbs_VERTEX)
            buildRecursiveConnector(currentShape);
        connector.buildEndNode();
    }
}

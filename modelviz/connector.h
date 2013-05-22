#ifndef CONNECTOR_H
#define CONNECTOR_H

#include <stack>

#include "connectorgraph.h"

namespace ModelViz
{

class Connector
{
public:
    Connector();
    void buildStartNode(const TopoDS_Shape &shapeIn);
    void buildEndNode();
    std::vector<int> useGetParentsOfType(const int &shapeHash, const TopAbs_ShapeEnum &shapeType);
    std::vector<int> useGetChildrenOfType(const int &shapeHash, const TopAbs_ShapeEnum &shapeType);
    TopoDS_Shape getShape(const int &shapeHash);
    void outputGraphviz(const std::string &name);
private:
    void buildAddShape(const TopoDS_Shape &shapeIn);
    void connectVertices(ConnectorGraph::Vertex from, ConnectorGraph::Vertex to);
    ConnectorGraph::Graph graph;
    ConnectorGraph::HashVertexMap vertexMap;
    std::stack<ConnectorGraph::Vertex> vertexStack;
};

class BuildConnector
{
public:
    BuildConnector(const TopoDS_Shape &root);
    void buildRecursiveConnector(const TopoDS_Shape &shapeIn);
    Connector getConnector(){return connector;}

private:
    Connector connector;
};

}

#endif // CONNECTOR_H

#ifndef CONNECTOR_H
#define CONNECTOR_H

#include <stack>

#include "../feature/maps.h"
#include "connectorgraph.h"

namespace ModelViz
{

class Connector
{
public:
    Connector() {}
    void buildStartNode(const TopoDS_Shape &shapeIn, const Feature::ResultContainer &resultContainerIn);
    void buildEndNode();
    std::vector<boost::uuids::uuid> useGetParentsOfType(const boost::uuids::uuid &, const TopAbs_ShapeEnum &shapeType) const;
    std::vector<boost::uuids::uuid> useGetChildrenOfType(const boost::uuids::uuid &, const TopAbs_ShapeEnum &shapeType) const;
    boost::uuids::uuid useGetWire(const boost::uuids::uuid &, const boost::uuids::uuid &) const;
    TopoDS_Shape getShape(const boost::uuids::uuid &);
    void outputGraphviz(const std::string &name);
private:
    void buildAddShape(const TopoDS_Shape &shapeIn, const Feature::ResultContainer &resultContainerIn);
    void connectVertices(ConnectorGraph::Vertex from, ConnectorGraph::Vertex to);
    ConnectorGraph::Graph graph;
    ConnectorGraph::IdVertexMap vertexMap;
    std::stack<ConnectorGraph::Vertex> vertexStack;
};

class BuildConnector
{
public:
    BuildConnector(const TopoDS_Shape &, const Feature::ResultContainer &resultContainerIn);
    void buildRecursiveConnector(const TopoDS_Shape &, const Feature::ResultContainer &resultContainerIn);
    Connector getConnector(){return connector;}

private:
    Connector connector;
};

}

#endif // CONNECTOR_H

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
    boost::uuids::uuid useGetRoot() const;
    bool useIsEdgeOfFace(const boost::uuids::uuid &edgeIn, const boost::uuids::uuid &faceIn) const;
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

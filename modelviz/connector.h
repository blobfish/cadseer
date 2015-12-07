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
#include <vector>

#include <osg/Vec3d>

#include <feature/maps.h>
#include <modelviz/connectorgraph.h>

namespace mdv
{

class Connector
{
public:
    Connector() {}
    void buildStartNode(const TopoDS_Shape &shapeIn, const ftr::ResultContainer &resultContainerIn);
    void buildEndNode();
    std::vector<boost::uuids::uuid> useGetParentsOfType(const boost::uuids::uuid &, const TopAbs_ShapeEnum &shapeType) const;
    std::vector<boost::uuids::uuid> useGetChildrenOfType(const boost::uuids::uuid &, const TopAbs_ShapeEnum &shapeType) const;
    boost::uuids::uuid useGetWire(const boost::uuids::uuid &, const boost::uuids::uuid &) const;
    boost::uuids::uuid useGetRoot() const;
    bool useIsEdgeOfFace(const boost::uuids::uuid &edgeIn, const boost::uuids::uuid &faceIn) const;
    TopoDS_Shape getShape(const boost::uuids::uuid &) const;
    void outputGraphviz();
    std::vector<osg::Vec3d> useGetEndPoints(const boost::uuids::uuid &) const;
    std::vector<osg::Vec3d> useGetMidPoint(const boost::uuids::uuid &) const;
    std::vector<osg::Vec3d> useGetCenterPoint(const boost::uuids::uuid &) const;
    std::vector<osg::Vec3d> useGetQuadrantPoints(const boost::uuids::uuid &) const;
    std::vector<osg::Vec3d> useGetNearestPoint(const boost::uuids::uuid &, const osg::Vec3d&) const;
private:
    void buildAddShape(const TopoDS_Shape &shapeIn, const ftr::ResultContainer &resultContainerIn);
    void connectVertices(cng::Vertex from, cng::Vertex to);
    cng::Graph graph;
    cng::IdVertexMap vertexMap;
    std::stack<cng::Vertex> vertexStack;
};

class BuildConnector
{
public:
    BuildConnector(const TopoDS_Shape &, const ftr::ResultContainer &resultContainerIn);
    void buildRecursiveConnector(const TopoDS_Shape &, const ftr::ResultContainer &resultContainerIn);
    const Connector& getConnector(){return connector;}

private:
    Connector connector;
};

}

#endif // CONNECTOR_H

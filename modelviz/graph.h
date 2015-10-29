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

#ifndef MODELVIZ_GRAPH_H
#define MODELVIZ_GRAPH_H

#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <Bnd_Box.hxx>
#include <TopTools_DataMapOfShapeInteger.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_MapOfShape.hxx>

#include <osg/Switch>
#include <osg/Geometry>

#include <feature/maps.h>

namespace mdv
{

class Build
{
public:
    Build(const TopoDS_Shape &, const ftr::ResultContainer &);
    osg::ref_ptr<osg::Switch> getViz();
    bool go(const Standard_Real &deflection, const Standard_Real &angle);
private:
    void setUpGraph();
    osg::ref_ptr<osg::Geometry> createGeometryEdge();
    osg::ref_ptr<osg::Geometry> createGeometryFace();
    osg::ref_ptr<osg::Geode> createGeodeEdge();
    osg::ref_ptr<osg::Geode> createGeodeFace();

    void recursiveConstruct(const TopoDS_Shape &shapeIn);
    void edgeConstruct(const TopoDS_Edge &edgeIn);
    void faceConstruct(const TopoDS_Face &faceIn);
    const TopoDS_Shape &originalShape;
    const ftr::ResultContainer &resultContainer;
    TopoDS_Shape copiedShape;
    TopTools_MapOfShape processed;
    Bnd_Box bound;
    TopTools_IndexedDataMapOfShapeListOfShape edgeToFace;
    osg::ref_ptr<osg::Switch> groupOut;
    osg::ref_ptr<osg::Switch> groupEdges;
    osg::ref_ptr<osg::Switch> groupFaces;
    bool success;
    bool initialized;
};
}

#endif // MODELVIZ_GRAPH_H

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

#include "../feature/maps.h"

namespace ModelViz
{

class Build
{
public:
    Build(const TopoDS_Shape &, const Feature::ResultContainer &);
    osg::ref_ptr<osg::Switch> getViz();
    bool go(const Standard_Real &deflection, const Standard_Real &angle);
private:
    void setUpGraph();
    osg::ref_ptr<osg::Geometry> createGeometryVertex();
    osg::ref_ptr<osg::Geometry> createGeometryEdge();
    osg::ref_ptr<osg::Geometry> createGeometryFace();
    osg::ref_ptr<osg::Geode> createGeodeVertex();
    osg::ref_ptr<osg::Geode> createGeodeEdge();
    osg::ref_ptr<osg::Geode> createGeodeFace();

    void recursiveConstruct(const TopoDS_Shape &shapeIn);
    void vertexConstruct(const TopoDS_Vertex &vertex);
    void edgeConstruct(const TopoDS_Edge &edgeIn);
    void faceConstruct(const TopoDS_Face &faceIn);
    const TopoDS_Shape &originalShape;
    const Feature::ResultContainer &resultContainer;
    TopoDS_Shape copiedShape;
    TopTools_MapOfShape processed;
    Bnd_Box bound;
    TopTools_IndexedDataMapOfShapeListOfShape edgeToFace;
    osg::ref_ptr<osg::Switch> groupOut;
    osg::ref_ptr<osg::Switch> groupVertices;
    osg::ref_ptr<osg::Switch> groupEdges;
    osg::ref_ptr<osg::Switch> groupFaces;
    bool success;
    bool initialized;

};
}

#endif // MODELVIZ_GRAPH_H

#ifndef MODELVIZ_GRAPH_H
#define MODELVIZ_GRAPH_H

#include <map>

#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <Bnd_Box.hxx>
#include <TopTools_DataMapOfShapeInteger.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_MapOfShape.hxx>

#include <osg/Group>

namespace ModelViz
{

class Build
{
public:
    Build(const TopoDS_Shape &shapeIn);
    osg::ref_ptr<osg::Group> getViz();
    bool go(const Standard_Real &deflection, const Standard_Real &angle);
private:
    int getVertexIndex(const gp_Pnt &pointIn);
    void recursiveConstruct(const TopoDS_Shape &shapeIn);
    void vertexConstruct(const TopoDS_Vertex &vertex);
    void edgeConstruct(const TopoDS_Edge &edgeIn);
    void faceConstruct(const TopoDS_Face &faceIn);
    const TopoDS_Shape &originalShape;
    TopoDS_Shape copiedShape;
    TopTools_MapOfShape processed;
    Bnd_Box bound;
    TopTools_IndexedDataMapOfShapeListOfShape edgeToFace;
    osg::ref_ptr<osg::Group> groupOut;
    bool success;
    bool initialized;

};
}

#endif // MODELVIZ_GRAPH_H

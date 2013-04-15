#ifndef MODELVIZ_GRAPH_H
#define MODELVIZ_GRAPH_H

#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>

#include <osg/Group>

namespace ModelViz
{
osg::ref_ptr<osg::Group> buildModel(const TopoDS_Shape &shape);
osg::ref_ptr<osg::Geode> meshFace(const TopoDS_Face &face);
osg::ref_ptr<osg::Geode> meshEdge(const TopoDS_Edge &edge, const TopoDS_Face &face);
}

#endif // MODELVIZ_GRAPH_H

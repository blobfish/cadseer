#include <iostream>
#include <stdexcept>

#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepTools.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepMesh.hxx>
#include <TopExp_Explorer.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_Polygon3D.hxx>
#include <Poly_PolygonOnTriangulation.hxx>
#include <TopoDS.hxx>
#include <Precision.hxx>

#include <osg/ShapeDrawable>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/LineWidth>
#include <osgUtil/SmoothingVisitor>
#include <osg/Depth>

#include "graph.h"


osg::ref_ptr<osg::Group> ModelViz::buildModel(const TopoDS_Shape &shape)
{
    osg::ref_ptr<osg::Group> group = new osg::Group;
    TopTools_IndexedMapOfShape shapeMap;
    TopExp::MapShapes(shape, shapeMap);
    TopTools_MapOfShape edgesDone;
    for (int index = 1; index <= shapeMap.Extent(); ++index)
    {
        try
        {
            if (shapeMap(index).ShapeType() == TopAbs_FACE)
            {
                osg::ref_ptr<osg::Geode> currentFace = ModelViz::meshFace(TopoDS::Face(shapeMap(index)));
                group->addChild(currentFace.get());

                TopTools_IndexedMapOfShape edgesMap;
                TopExp::MapShapes(shapeMap(index), TopAbs_EDGE, edgesMap);
                for (int innerIndex = 1; innerIndex <= edgesMap.Extent(); ++innerIndex)
                {
                    TopoDS_Edge currentEdge = TopoDS::Edge(edgesMap(innerIndex));
                    if (edgesDone.Contains(currentEdge))
                        continue;

                    osg::ref_ptr<osg::Geode> vizEdge = ModelViz::meshEdge(
                                TopoDS::Edge(edgesMap(innerIndex)), TopoDS::Face(shapeMap(index)));
                    group->addChild(vizEdge.get());
                    edgesDone.Add(currentEdge);
                }



            }
//            if (shapeMap(index).ShapeType() == TopAbs_EDGE)
//            {
//                osg::ref_ptr<osg::Geode> currentEdge = ModelViz::meshEdge(TopoDS::Edge(shapeMap(index)));
//                group->addChild(currentEdge.get());
//            }
        }
        catch (std::exception &e)
        {
            std::cout << "Caught an exception of an unexpected type: "
                      << e.what () << std::endl;
        }
        catch(...)
        {
            std::cout << "caught error meshing face" << std::endl;
        }
    }

    return group.release();
}

osg::ref_ptr<osg::Geode> ModelViz::meshEdge(const TopoDS_Edge &edge, const TopoDS_Face &face)
{
    TopLoc_Location location;
    Handle(Poly_Triangulation) triangulation;
    triangulation = BRep_Tool::Triangulation(face, location);

    const Handle(Poly_PolygonOnTriangulation) &segments =
            BRep_Tool::PolygonOnTriangulation(edge, triangulation, location);
    if (segments.IsNull())
        throw std::runtime_error("null edge triangulation");

    gp_Trsf transformation;
    bool identity = true;
    if(!location.IsIdentity())
    {
        identity = false;
        transformation = location.Transformation();
    }

    const TColStd_Array1OfInteger& indexes = segments->Nodes();
    const TColgp_Array1OfPnt& nodes = triangulation->Nodes();


    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    for (int index(indexes.Lower()); index < indexes.Upper() + 1; ++index)
    {
        gp_Pnt point = nodes(indexes(index));
        if(!identity)
            point.Transform(transformation);
        vertices->push_back(osg::Vec3(point.X(), point.Y(), point.Z()));
    }

    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array(1);
    (*colors)[0].set(0.0f, 0.0f, 0.0f, 1.0f);
    osg::ref_ptr<osg::Geometry> border = new osg::Geometry;
    border->setVertexArray(vertices.get());
    border->setColorArray(colors.get());
    border->setColorBinding(osg::Geometry::BIND_OVERALL);
    border->addPrimitiveSet(new osg::DrawArrays(GL_LINE_STRIP, 0, vertices->size()));
    border->getOrCreateStateSet()->setAttribute(new osg::LineWidth(2.0f));
    border->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    osg::Depth *depth = new osg::Depth();
    depth->setRange(0.0, 0.999);
    border->getOrCreateStateSet()->setAttribute(depth);


//    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
//    geom->setVertexArray(vertices.get());
//    geom->addPrimitiveSet(indices.get());
//    osgUtil::SmoothingVisitor::smooth( *geom );
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(border.get());

    return geode.release();
}

osg::ref_ptr<osg::Geode> ModelViz::meshFace(const TopoDS_Face &face)
{

    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::DrawElementsUInt> indices;


    TopLoc_Location location;
    Handle(Poly_Triangulation) triangulation;
    triangulation = BRep_Tool::Triangulation(face, location);

    if (triangulation.IsNull())
        throw std::runtime_error("null face triangulation");
    bool signalOrientation(false);
    if (face.Orientation() == TopAbs_FORWARD)
        signalOrientation = true;

    gp_Trsf transformation;
    bool identity = true;
    if(!location.IsIdentity())
    {
        identity = false;
        transformation = location.Transformation();
    }

    //vertices.
    const TColgp_Array1OfPnt& nodes = triangulation->Nodes();
    for (int index(nodes.Lower()); index < nodes.Upper() + 1; ++index)
    {
        gp_Pnt point = nodes.Value(index);
        if(!identity)
            point.Transform(transformation);
        vertices->push_back(osg::Vec3(point.X(), point.Y(), point.Z()));
    }

    //normals.
    const Poly_Array1OfTriangle& triangles = triangulation->Triangles();
    indices = new osg::DrawElementsUInt(GL_TRIANGLES, triangulation->NbTriangles() * 3);

    for (int index(triangles.Lower()); index < triangles.Upper() + 1; ++index)
    {


        int N1, N2, N3;
        triangles(index).Get(N1, N2, N3);
        int factor = (index - 1) * 3;

        if (!signalOrientation)
        {
            (*indices)[factor] = N3 - 1;
            (*indices)[factor + 1] = N2 - 1;
            (*indices)[factor + 2] = N1 - 1;
        }
        else
        {
            (*indices)[factor] = N1 - 1;
            (*indices)[factor + 1] = N2 - 1;
            (*indices)[factor + 2] = N3 - 1;
        }
    }

    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    geom->setVertexArray(vertices.get());
    geom->addPrimitiveSet(indices.get());

    osg::ref_ptr<osg::Vec4Array> colorArray = new osg::Vec4Array;
    colorArray->push_back(osg::Vec4(.1f, .7f, .1f, .5f));
    geom->setColorArray(colorArray);
    geom->setColorBinding(osg::Geometry::BIND_OVERALL);
    osgUtil::SmoothingVisitor::smooth(*geom);
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(geom.get());

    return geode.release();
}

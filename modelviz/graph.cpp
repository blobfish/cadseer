#include <iostream>
#include <stdexcept>
#include <algorithm>

#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBndLib.hxx>
#include <BRepTools.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepMesh_FastDiscret.hxx>
#include <BRepMesh.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_Polygon3D.hxx>
#include <Poly_PolygonOnTriangulation.hxx>
#include <TopoDS.hxx>
#include <Precision.hxx>
#include <Standard_Failure.hxx>
#include <Standard_ErrorHandler.hxx>

#include <osg/ShapeDrawable>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Point>
#include <osg/LineWidth>
#include <osgUtil/SmoothingVisitor>
#include <osg/Depth>

#include "graph.h"

using namespace ModelViz;
Build::Build(const TopoDS_Shape &shapeIn) : originalShape(shapeIn), success(false),
    initialized(false)
{
    try
    {
        BRepBuilderAPI_Copy copier;
        copier.Perform(originalShape);
        copiedShape = copier.Shape();
        BRepBndLib::Add(copiedShape, bound);
        TopExp::MapShapesAndAncestors(copiedShape, TopAbs_EDGE, TopAbs_FACE, edgeToFace);
        groupOut = new osg::Group();
    //    groupOut->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
    //    groupOut->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

        initialized = true;
    }
    catch(Standard_Failure)
    {
        Handle(Standard_Failure) error = Standard_Failure::Caught();
        std::cout << "OCC Error: construction failure building model vizualization. Message: " <<
                     error->GetMessageString() << std::endl;
    }
    catch(...)
    {
        std::cout << "Unknown Error: construction failure building model vizualization. Message: " << std::endl;
    }
}

bool Build::go(const Standard_Real &deflection, const Standard_Real &angle)
{
    if (!initialized)
        return false;

    try
    {
        BRepTools::Clean(copiedShape);//might call this several times for same shape;
        processed.Clear();

        //I don't see how to get the mesh data without storing it in the faces themselves
        //I am making a copy of the shape in the constructor.
        //    Will have to prove what this does to the hash

        //I have store in shape set to false (2nd to last param) but yet I am able
        //to access the mesh and create the viz? how?

        Handle(BRepMesh_FastDiscret) mesher = new BRepMesh_FastDiscret
                (deflection, copiedShape, bound, angle, Standard_True, Standard_True,
                 Standard_False, Standard_True);

        recursiveConstruct(copiedShape);
        processed.Add(copiedShape);
        success = true;

        return true;
    }
    catch(Standard_Failure)
    {
        Handle(Standard_Failure) error = Standard_Failure::Caught();
        std::cout << "OCC Error: failure building model vizualization. Message: " <<
                     error->GetMessageString() << std::endl;
    }
    catch(const std::exception &error)
    {
        std::cout << "Internal Error: failure building model vizualization. Message: " <<
                     error.what() << std::endl;
    }

    catch(...)
    {
        std::cout << "Unknown Error: failure building model vizualization. Message: " << std::endl;
    }
    return false;
}

void Build::recursiveConstruct(const TopoDS_Shape &shapeIn)
{
    for (TopoDS_Iterator it(shapeIn); it.More(); it.Next())
    {
        const TopoDS_Shape &currentShape = it.Value();
        TopAbs_ShapeEnum currentType = currentShape.ShapeType();
        if (processed.Contains(currentShape))
            continue;
        processed.Add(currentShape);

        if (currentType == TopAbs_COMPOUND || currentType == TopAbs_COMPSOLID ||
                currentType == TopAbs_SOLID || currentType == TopAbs_SHELL ||
                currentType == TopAbs_WIRE)
        {
           recursiveConstruct(currentShape);
           continue;
        }

        if (currentType == TopAbs_FACE)
        {
            faceConstruct(TopoDS::Face(currentShape));
            recursiveConstruct(currentShape);
            continue;
        }
        if (currentType == TopAbs_EDGE)
        {
            edgeConstruct(TopoDS::Edge(currentShape));
            recursiveConstruct(currentShape);
        }
        if (currentType == TopAbs_VERTEX)
        {
            vertexConstruct(TopoDS::Vertex(currentShape));
        }
    }
}

void Build::vertexConstruct(const TopoDS_Vertex &vertex)
{
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;

    gp_Pnt vPoint = BRep_Tool::Pnt(vertex);
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array(1);
    (*vertices)[0] = osg::Vec3Array::value_type(vPoint.X(), vPoint.Y(), vPoint.Z());
    geom->setVertexArray(vertices.get());
    geom->addPrimitiveSet(new osg::DrawArrays
                          (osg::PrimitiveSet::POINTS, 0, vertices->size()));

    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array(1);
    (*colors)[0].set(1.0f, 1.0f, 0.0f, 1.0f);
    geom->setColorArray(colors.get());
    geom->setColorBinding(osg::Geometry::BIND_OVERALL);
    geom->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    osg::Point *point = new osg::Point;
    point->setSize(10.0);
    geom->getOrCreateStateSet()->setAttribute(point);

    osg::Depth *depth = new osg::Depth();
    depth->setRange(-0.001, 0.998);
    geom->getOrCreateStateSet()->setAttribute(depth);

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(geom.get());
    groupOut->addChild(geode.get());
}

void Build::edgeConstruct(const TopoDS_Edge &edgeIn)
{
    TopoDS_Face face = TopoDS::Face(edgeToFace.FindFromKey(edgeIn).First());
    if (face.IsNull())
        return;

    TopLoc_Location location;
    Handle(Poly_Triangulation) triangulation;
    triangulation = BRep_Tool::Triangulation(face, location);

    const Handle(Poly_PolygonOnTriangulation) &segments =
            BRep_Tool::PolygonOnTriangulation(edgeIn, triangulation, location);
    if (segments.IsNull())
        return;

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
    osg::ref_ptr<osg::Geometry> edgeViz = new osg::Geometry;
    edgeViz->setVertexArray(vertices.get());
    edgeViz->setColorArray(colors.get());
    edgeViz->setColorBinding(osg::Geometry::BIND_OVERALL);
    edgeViz->addPrimitiveSet(new osg::DrawArrays(GL_LINE_STRIP, 0, vertices->size()));
    edgeViz->getOrCreateStateSet()->setAttribute(new osg::LineWidth(2.0f));
    edgeViz->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    osg::Depth *depth = new osg::Depth();
    depth->setRange(0.000, 0.999);
    edgeViz->getOrCreateStateSet()->setAttribute(depth);

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(edgeViz.get());
    groupOut->addChild(geode.get());
}

void Build::faceConstruct(const TopoDS_Face &faceIn)
{
    TopLoc_Location location;
    Handle(Poly_Triangulation) triangulation;
    triangulation = BRep_Tool::Triangulation(faceIn, location);

    if (triangulation.IsNull())
    {
        std::cout << "null face triangulation" << std::endl;//not sure what to do just yet.
        return;
    }
    bool signalOrientation(false);
    if (faceIn.Orientation() == TopAbs_FORWARD)
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
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array(nodes.Length());
    for (int index(nodes.Lower()); index < nodes.Upper() + 1; ++index)
    {
        gp_Pnt point = nodes.Value(index);
        if(!identity)
            point.Transform(transformation);
        (*vertices)[index - 1] = osg::Vec3(point.X(), point.Y(), point.Z());
    }

    const Poly_Array1OfTriangle& triangles = triangulation->Triangles();
    osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt
            (GL_TRIANGLES, triangulation->NbTriangles() * 3);

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
    geom->setUseDisplayList(false);
    geom->setUseVertexBufferObjects(true);
    geom->setVertexArray(vertices.get());
    geom->addPrimitiveSet(indices.get());

    osg::ref_ptr<osg::Vec4Array> colorArray = new osg::Vec4Array;
    colorArray->push_back(osg::Vec4(.1f, .7f, .1f, .5f));
    geom->setColorArray(colorArray);
    geom->setColorBinding(osg::Geometry::BIND_OVERALL);
    osgUtil::SmoothingVisitor::smooth(*geom);
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;

    geode->addDrawable(geom.get());
    groupOut->addChild(geode.get());
}

osg::ref_ptr<osg::Group> Build::getViz()
{
    if (success)
        return groupOut;
    else
        return osg::ref_ptr<osg::Group>();

}

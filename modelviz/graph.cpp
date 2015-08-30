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
#include <osg/ValueObject>

#include "graph.h"
#include "../nodemaskdefs.h"
#include "../globalutilities.h"


using namespace boost::uuids;
using namespace ModelViz;

Build::Build(const TopoDS_Shape &shapeIn, const Feature::ResultContainer &resultContainerIn) :
  originalShape(shapeIn), resultContainer(resultContainerIn), success(false), initialized(false)
{
    try
    {
        //copying shape changes the hash values. need consistency to connector.
//        BRepBuilderAPI_Copy copier;
//        copier.Perform(originalShape);
//        copiedShape = copier.Shape();

        copiedShape = originalShape;


        BRepBndLib::Add(copiedShape, bound);
        TopExp::MapShapesAndAncestors(copiedShape, TopAbs_EDGE, TopAbs_FACE, edgeToFace);
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

    setUpGraph();

    try
    {
        BRepTools::Clean(copiedShape);//might call this several times for same shape;
        processed.Clear();

        //I don't see how to get the mesh data without storing it in the faces themselves
        //I am making a copy of the shape in the constructor.
        //    Will have to prove what this does to the hash

        //I have store in shape set to false (2nd to last param) but yet I am able
        //to access the mesh and create the viz? how?

        BRepMesh_IncrementalMesh(copiedShape,deflection,Standard_False,
                angle,Standard_True);

        processed.Add(copiedShape);
        if (copiedShape.ShapeType() == TopAbs_FACE)
            faceConstruct(TopoDS::Face(copiedShape));
        if (copiedShape.ShapeType() == TopAbs_EDGE)
            edgeConstruct(TopoDS::Edge(copiedShape));
        if (copiedShape.ShapeType() == TopAbs_VERTEX)
            vertexConstruct(TopoDS::Vertex(copiedShape));
        recursiveConstruct(copiedShape);
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

void Build::setUpGraph()
{
    groupOut = new osg::Switch();
    groupOut->setNodeMask(NodeMask::lod);
    groupVertices = new osg::Switch();
    groupVertices->setNodeMask(NodeMask::vertex);
    groupEdges = new osg::Switch();
    groupEdges->setNodeMask(NodeMask::edge);
    groupFaces = new osg::Switch();
    groupFaces->setNodeMask(NodeMask::face);
    groupOut->addChild(groupVertices);
    groupOut->addChild(groupEdges);
    groupOut->addChild(groupFaces);
    
    //vertex state
    osg::Point *point = new osg::Point;
    point->setSize(10.0);
    groupVertices->getOrCreateStateSet()->setAttribute(point);

    osg::Depth *depth = new osg::Depth();
    depth->setRange(0.0, 1.0);
    groupVertices->getOrCreateStateSet()->setAttribute(depth);
    
    groupVertices->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    
    //edge state
    depth = new osg::Depth();
    depth->setRange(0.001, 1.001);
    groupEdges->getOrCreateStateSet()->setAttribute(depth);
    groupEdges->getOrCreateStateSet()->setAttribute(new osg::LineWidth(2.0f));
    groupEdges->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    
    //face state
    depth = new osg::Depth();
    depth->setRange(0.002, 1.002);
    groupFaces->getOrCreateStateSet()->setAttribute(depth);
}

osg::ref_ptr<osg::Geometry> Build::createGeometryVertex()
{
    //vertex
    osg::ref_ptr<osg::Geometry> geomVertices = new osg::Geometry();
    geomVertices->setVertexArray(new osg::Vec3Array());

    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    geomVertices->setColorArray(colors.get());
    geomVertices->setColorBinding(osg::Geometry::BIND_OVERALL);

    return geomVertices;
}

osg::ref_ptr<osg::Geometry> Build::createGeometryEdge()
{
    //edges
    osg::ref_ptr<osg::Geometry> geomEdges = new osg::Geometry();
    geomEdges->setVertexArray(new osg::Vec3Array);

    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    geomEdges->setColorArray(colors.get());
    geomEdges->setColorBinding(osg::Geometry::BIND_OVERALL);

    return geomEdges;
}

osg::ref_ptr<osg::Geometry> Build::createGeometryFace()
{
    //faces
    osg::ref_ptr<osg::Geometry> geomFaces = new osg::Geometry();
    geomFaces->setVertexArray(new osg::Vec3Array);
    geomFaces->setUseDisplayList(false);
    geomFaces->setUseVertexBufferObjects(true);

    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    geomFaces->setColorArray(colors.get());
    geomFaces->setColorBinding(osg::Geometry::BIND_OVERALL);

    geomFaces->setNormalArray(new osg::Vec3Array);
    geomFaces->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);

    return geomFaces;
}

osg::ref_ptr<osg::Geode> Build::createGeodeVertex()
{
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->setCullingActive(false);//so we can select. Might not need in a non "points only" environment.
    return geode;
}

osg::ref_ptr<osg::Geode> Build::createGeodeEdge()
{
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    return geode;
}

osg::ref_ptr<osg::Geode> Build::createGeodeFace()
{
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    return geode;
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
        if (!(hasResult(resultContainer, currentShape)))
          continue; //probably seam edge.

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
    osg::ref_ptr<osg::Geode> geode = createGeodeVertex();
    uuid id = Feature::findResultByShape(resultContainer, vertex).id;
    geode->setUserValue(GU::idAttributeTitle, GU::idToString(id));

    osg::ref_ptr<osg::Geometry> geometry = createGeometryVertex();
    geode->addDrawable(geometry);
    groupVertices->addChild(geode);

    gp_Pnt vPoint = BRep_Tool::Pnt(vertex);
    osg::Vec3Array *vertices = dynamic_cast<osg::Vec3Array *>(geometry->getVertexArray());
    vertices->push_back(osg::Vec3Array::value_type(vPoint.X(), vPoint.Y(), vPoint.Z()));
    geometry->addPrimitiveSet(new osg::DrawArrays
                          (osg::PrimitiveSet::POINTS, vertices->size() - 1, 1));
    osg::Vec4Array *colors= dynamic_cast<osg::Vec4Array *>(geometry->getColorArray());
    colors->push_back(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
}

void Build::edgeConstruct(const TopoDS_Edge &edgeIn)
{
    TopoDS_Face face = TopoDS::Face(edgeToFace.FindFromKey(edgeIn).First());
    if (face.IsNull())
        throw std::runtime_error("face is null in edge construction");

    TopLoc_Location location;
    Handle(Poly_Triangulation) triangulation;
    triangulation = BRep_Tool::Triangulation(face, location);

    const Handle(Poly_PolygonOnTriangulation) &segments =
            BRep_Tool::PolygonOnTriangulation(edgeIn, triangulation, location);
    if (segments.IsNull())
        throw std::runtime_error("edge triangulation is null in edge construction");

    gp_Trsf transformation;
    bool identity = true;
    if(!location.IsIdentity())
    {
        identity = false;
        transformation = location.Transformation();
    }

    osg::ref_ptr<osg::Geode> geode = createGeodeEdge();
    uuid id = Feature::findResultByShape(resultContainer, edgeIn).id;
    geode->setUserValue(GU::idAttributeTitle, GU::idToString(id));
    osg::ref_ptr<osg::Geometry> geometry = createGeometryEdge();
    geode->addDrawable(geometry);
    groupEdges->addChild(geode);

    const TColStd_Array1OfInteger& indexes = segments->Nodes();
    const TColgp_Array1OfPnt& nodes = triangulation->Nodes();
    osg::Vec3Array *vertices = dynamic_cast<osg::Vec3Array *>(geometry->getVertexArray());
    int startIndex = vertices->size();
    for (int index(indexes.Lower()); index < indexes.Upper() + 1; ++index)
    {
        gp_Pnt point = nodes(indexes(index));
        if(!identity)
            point.Transform(transformation);
        vertices->push_back(osg::Vec3(point.X(), point.Y(), point.Z()));
    }
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_LINE_STRIP, startIndex, vertices->size()-startIndex));
    osg::Vec4Array *colors= dynamic_cast<osg::Vec4Array *>(geometry->getColorArray());
    colors->push_back(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
}

void Build::faceConstruct(const TopoDS_Face &faceIn)
{
    TopLoc_Location location;
    Handle(Poly_Triangulation) triangulation;
    triangulation = BRep_Tool::Triangulation(faceIn, location);

    if (triangulation.IsNull())
        throw std::runtime_error("null triangulation in face construction");

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

    osg::ref_ptr<osg::Geode> geode = createGeodeFace();
    uuid id = Feature::findResultByShape(resultContainer, faceIn).id;
    geode->setUserValue(GU::idAttributeTitle, GU::idToString(id));
    osg::ref_ptr<osg::Geometry> geometry = createGeometryFace();
    geode->addDrawable(geometry);
    groupFaces->addChild(geode);

    //vertices.
    const TColgp_Array1OfPnt& nodes = triangulation->Nodes();
    osg::Vec3Array *vertices = dynamic_cast<osg::Vec3Array *>(geometry->getVertexArray());
    osg::Vec3Array *normals = dynamic_cast<osg::Vec3Array *>(geometry->getNormalArray());
    for (int index(nodes.Lower()); index < nodes.Upper() + 1; ++index)
    {
        gp_Pnt point = nodes.Value(index);
        if(!identity)
            point.Transform(transformation);
        vertices->push_back(osg::Vec3(point.X(), point.Y(), point.Z()));
        normals->push_back(osg::Vec3(0.0, 0.0, 0.0));
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

        //calculate normal.
        /* let osg::smoothing visitor calc normals.
        osg::Vec3 pointOne(vertices->at((*indices)[factor]));
        osg::Vec3 pointTwo(vertices->at((*indices)[factor + 1]));
        osg::Vec3 pointThree(vertices->at((*indices)[factor + 2]));

        osg::Vec3 axisOne(pointTwo - pointOne);
        osg::Vec3 axisTwo(pointThree - pointOne);
        osg::Vec3 currentNormal(axisOne ^ axisTwo);
        if (currentNormal.isNaN())
            continue;
        currentNormal.normalize();

        osg::Vec3 tempNormal;

        tempNormal = (*normals)[(*indices)[factor]];
        tempNormal += currentNormal;
        tempNormal.normalize();
        (*normals)[(*indices)[factor]] = tempNormal;

        tempNormal = (*normals)[(*indices)[factor + 1]];
        tempNormal += currentNormal;
        tempNormal.normalize();
        (*normals)[(*indices)[factor + 1]] = tempNormal;

        tempNormal = (*normals)[(*indices)[factor + 2]];
        tempNormal += currentNormal;
        tempNormal.normalize();
        (*normals)[(*indices)[factor + 2]] = tempNormal;
        */
    }
    geometry->addPrimitiveSet(indices.get());
    osg::Vec4Array *colors= dynamic_cast<osg::Vec4Array *>(geometry->getColorArray());
    colors->push_back(osg::Vec4(.1f, .7f, .1f, .5f));
    
    osgUtil::SmoothingVisitor::smooth(*geometry);
}

osg::ref_ptr<osg::Switch> Build::getViz()
{
    if (success)
        return groupOut;
    else
        return osg::ref_ptr<osg::Switch>();
}

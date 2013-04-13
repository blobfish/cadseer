#include <stdexcept>
#include <iostream>

#include <QHBoxLayout>
#include <QFileDialog>
#include <QTimer>

#include <osgDB/ReadFile>
#include <osgViewer/Viewer>
#include <osgViewer/View>
#include <osg/ShapeDrawable>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/Group>
#include <osgUtil/SmoothingVisitor>

#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepMesh.hxx>
#include <TopExp_Explorer.hxx>
#include <Poly_Triangulation.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS.hxx>
#include <Precision.hxx>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "viewerwidget.h"
#include "spaceballqevent.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    viewWidget = new ViewerWidget(osgViewer::ViewerBase::SingleThreaded);
    viewWidget->setGeometry( 100, 100, 800, 600 );
    QHBoxLayout *aLayout = new QHBoxLayout();
    aLayout->addWidget(viewWidget);
    ui->centralwidget->setLayout(aLayout);

    connect(ui->actionAppendBrep, SIGNAL(triggered()), this, SLOT(readBrepSlot()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::addNode(osg::Node *node)
{
    viewWidget->addNode(node);
}

void MainWindow::readBrepSlot()
{
    QString fileName = QFileDialog::getOpenFileName(ui->centralwidget, tr("Open File"), "/home", tr("brep (*.brep *.brp)"));
    if (fileName.isEmpty())
        return;

    TopoDS_Shape base;
    BRep_Builder junk;
    std::fstream file(fileName.toStdString().c_str());
    BRepTools::Read(base, file, junk);

    BRepTools::Clean(base);
    BRepMesh_IncrementalMesh mesh(base, 0.25);
    osg::ref_ptr<osg::Group> group = buildModel(base);
    addNode(group.get());
}

osg::ref_ptr<osg::Geode> MainWindow::meshFace(const TopoDS_Face &face)
{

    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::DrawElementsUInt> indices;


    TopLoc_Location location;
    Handle(Poly_Triangulation) triangulation;
    triangulation = BRep_Tool::Triangulation(face, location);

    if (triangulation.IsNull())
        throw std::runtime_error("null triangulation");
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
    osgUtil::SmoothingVisitor::smooth( *geom );
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(geom.get());

    return geode.release();
}

osg::ref_ptr<osg::Group> MainWindow::buildModel(TopoDS_Shape &shape)
{
    osg::ref_ptr<osg::Group> group = new osg::Group;

    TopExp_Explorer it;
    for (it.Init(shape, TopAbs_FACE); it.More(); it.Next())
    {
        try
        {
            osg::ref_ptr<osg::Geode> currentFace = meshFace(TopoDS::Face(it.Current()));
            group->addChild(currentFace.get());
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

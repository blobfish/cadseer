#include <iostream>
#include <assert.h>

#include <QHBoxLayout>
#include <QFileDialog>
#include <QTimer>

#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepMesh.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeCone.hxx>

#include "mainwindow.h"
#include "application.h"
#include "ui_mainwindow.h"
#include "viewerwidget.h"
#include "selectionmanager.h"
#include "selectiondefs.h"
#include "document.h"
#include "command/commandmanager.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    viewWidget = new ViewerWidget(osgViewer::ViewerBase::SingleThreaded);
    viewWidget->setGeometry( 100, 100, 800, 600 );
    connect(ui->actionHide, SIGNAL(triggered()), viewWidget, SLOT(hideSelected()));
    connect(ui->actionShowAll, SIGNAL(triggered()), viewWidget, SLOT(showAll()));
    QHBoxLayout *aLayout = new QHBoxLayout();
    aLayout->addWidget(viewWidget);
    ui->centralwidget->setLayout(aLayout);

    selectionManager = new SelectionManager(this);
    setupSelectionToolbar();
    connect(selectionManager, SIGNAL(setSelectionMask(int)), viewWidget, SLOT(setSelectionMask(int)));
    selectionManager->setState(SelectionMask::all);

    connect(ui->actionAppendBrep, SIGNAL(triggered()), this, SLOT(readBrepSlot()));

    Document *document = new Document(qApp);
    Application *application = dynamic_cast<Application *>(qApp);
    assert(application);
    application->setDocument(document);

    setupCommands();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::readBrepSlot()
{
    QString fileName = QFileDialog::getOpenFileName(ui->centralwidget, tr("Open File"), "/home", tr("brep (*.brep *.brp)"));
    if (fileName.isEmpty())
        return;

    Application *application = dynamic_cast<Application *>(qApp);
    assert(application);
    Document *document = application->getDocument();
    document->readOCC(fileName.toStdString(), viewWidget->getRoot());
    viewWidget->update();
}

void MainWindow::setupSelectionToolbar()
{
    selectionManager->actionSelectObjects = ui->actionSelectObjects;
    selectionManager->actionSelectFeatures = ui->actionSelectFeatures;
    selectionManager->actionSelectSolids = ui->actionSelectSolids;
    selectionManager->actionSelectShells = ui->actionSelectShells;
    selectionManager->actionSelectFaces = ui->actionSelectFaces;
    selectionManager->actionSelectWires = ui->actionSelectWires;
    selectionManager->actionSelectEdges = ui->actionSelectEdges;
    selectionManager->actionSelectVertices = ui->actionSelectVertices;

    connect(ui->actionSelectObjects, SIGNAL(triggered(bool)), selectionManager, SLOT(triggeredObjects(bool)));
    connect(ui->actionSelectFeatures, SIGNAL(triggered(bool)), selectionManager, SLOT(triggeredFeatures(bool)));
    connect(ui->actionSelectSolids, SIGNAL(triggered(bool)), selectionManager, SLOT(triggeredSolids(bool)));
    connect(ui->actionSelectShells, SIGNAL(triggered(bool)), selectionManager, SLOT(triggeredShells(bool)));
    connect(ui->actionSelectFaces, SIGNAL(triggered(bool)), selectionManager, SLOT(triggeredFaces(bool)));
    connect(ui->actionSelectWires, SIGNAL(triggered(bool)), selectionManager, SLOT(triggeredWires(bool)));
    connect(ui->actionSelectEdges, SIGNAL(triggered(bool)), selectionManager, SLOT(triggeredEdges(bool)));
    connect(ui->actionSelectVertices, SIGNAL(triggered(bool)), selectionManager, SLOT(triggeredVertices(bool)));
}

void MainWindow::setupCommands()
{
    QAction *constructionBoxAction = new QAction(qApp);
    connect(constructionBoxAction, SIGNAL(triggered()), this, SLOT(contructionBoxSlot()));
    Command constructionBoxCommand(CommandConstants::ConstructionBox, "Construct Box", constructionBoxAction);
    CommandManager::getManager().addCommand(constructionBoxCommand);

    QAction *constructionSphereAction = new QAction(qApp);
    connect(constructionSphereAction, SIGNAL(triggered()), this, SLOT(contructionSphereSlot()));
    Command constructionSphereCommand(CommandConstants::ConstructionSphere, "Construct Sphere", constructionSphereAction);
    CommandManager::getManager().addCommand(constructionSphereCommand);

    QAction *constructionConeAction = new QAction(qApp);
    connect(constructionConeAction, SIGNAL(triggered()), this, SLOT(contructionConeSlot()));
    Command constructionConeCommand(CommandConstants::ConstructionCone, "Construct Cone", constructionConeAction);
    CommandManager::getManager().addCommand(constructionConeCommand);
}

void MainWindow::contructionBoxSlot()
{
    BRepPrimAPI_MakeBox boxMaker(10.0, 10.0, 10.0);
    boxMaker.Build();
    assert(boxMaker.IsDone());
    TopoDS_Shape boxShape = boxMaker.Shape();

    Application *application = dynamic_cast<Application *>(qApp);
    assert(application);
    Document *document = application->getDocument();
    document->addOCCShape(boxShape, viewWidget->getRoot());
    viewWidget->update();
}

void MainWindow::contructionSphereSlot()
{
    BRepPrimAPI_MakeSphere sphereMaker(5.0);
    sphereMaker.Build();
    assert(sphereMaker.IsDone());
    TopoDS_Shape sphereShape = sphereMaker.Shape();

    Application *application = dynamic_cast<Application *>(qApp);
    assert(application);
    Document *document = application->getDocument();
    document->addOCCShape(sphereShape, viewWidget->getRoot());
    viewWidget->update();
}

void MainWindow::contructionConeSlot()
{
    BRepPrimAPI_MakeCone coneMaker(8.0, 0.0, 10.0);
    coneMaker.Build();
    assert(coneMaker.IsDone());
    TopoDS_Shape coneShape = coneMaker.Shape();

    Application *application = dynamic_cast<Application *>(qApp);
    assert(application);
    Document *document = application->getDocument();
    document->addOCCShape(coneShape, viewWidget->getRoot());
    viewWidget->update();
}

#include <iostream>
#include <assert.h>

#include <QHBoxLayout>
#include <QFileDialog>

#include "mainwindow.h"
#include "application.h"
#include "ui_mainwindow.h"
#include "viewerwidget.h"
#include "selectionmanager.h"
#include "selectiondefs.h"
#include "project/project.h"
#include "command/commandmanager.h"
#include "feature/box.h"
#include "feature/sphere.h"
#include "feature/cone.h"
#include "feature/cylinder.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    viewWidget = new ViewerWidget(osgViewer::ViewerBase::SingleThreaded);
    viewWidget->setGeometry( 100, 100, 800, 600 );
    connect(ui->actionHide, SIGNAL(triggered()), viewWidget, SLOT(hideSelected()));
    connect(ui->actionShowAll, SIGNAL(triggered()), viewWidget, SLOT(showAll()));
    connect(ui->actionExportOSG, SIGNAL(triggered()), viewWidget, SLOT(writeOSGSlot()));
    QHBoxLayout *aLayout = new QHBoxLayout();
    aLayout->addWidget(viewWidget);
    ui->centralwidget->setLayout(aLayout);

    selectionManager = new SelectionManager(this);
    setupSelectionToolbar();
    connect(selectionManager, SIGNAL(setSelectionMask(int)), viewWidget, SLOT(setSelectionMask(int)));
    selectionManager->setState(SelectionMask::all);

    connect(ui->actionAppendBrep, SIGNAL(triggered()), this, SLOT(readBrepSlot()));

    Project *project = new Project();
    Application *application = dynamic_cast<Application *>(qApp);
    assert(application);
    application->setProject(project);

    setupCommands();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::readBrepSlot()
{
    QString fileName = QFileDialog::getOpenFileName(ui->centralwidget, tr("Open File"),
                                                    "/home/tanderson/Programming/cadseer/test/files", tr("brep (*.brep *.brp)"));
    if (fileName.isEmpty())
        return;

    Application *application = dynamic_cast<Application *>(qApp);
    assert(application);
    Project *project = application->getProject();
    project->readOCC(fileName.toStdString(), viewWidget->getRoot());
    project->update();
    project->updateVisual();
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
    connect(constructionBoxAction, SIGNAL(triggered()), this, SLOT(constructionBoxSlot()));
    Command constructionBoxCommand(CommandConstants::ConstructionBox, "Construct Box", constructionBoxAction);
    CommandManager::getManager().addCommand(constructionBoxCommand);

    QAction *constructionSphereAction = new QAction(qApp);
    connect(constructionSphereAction, SIGNAL(triggered()), this, SLOT(constructionSphereSlot()));
    Command constructionSphereCommand(CommandConstants::ConstructionSphere, "Construct Sphere", constructionSphereAction);
    CommandManager::getManager().addCommand(constructionSphereCommand);

    QAction *constructionConeAction = new QAction(qApp);
    connect(constructionConeAction, SIGNAL(triggered()), this, SLOT(constructionConeSlot()));
    Command constructionConeCommand(CommandConstants::ConstructionCone, "Construct Cone", constructionConeAction);
    CommandManager::getManager().addCommand(constructionConeCommand);
    
    QAction *constructionCylinderAction = new QAction(qApp);
    connect(constructionCylinderAction, SIGNAL(triggered(bool)), this, SLOT(constructionCylinderSlot()));
    Command constructionCylinderCommand(CommandConstants::ConstructionCylinder, "Construct Cylinder", constructionCylinderAction);
    CommandManager::getManager().addCommand(constructionCylinderCommand);
}

void MainWindow::constructionBoxSlot()
{
    Application *application = dynamic_cast<Application *>(qApp);
    assert(application);
    Project *project = application->getProject();
    
    std::shared_ptr<Feature::Box> box(new Feature::Box());
    box->setParameters(20.0, 10.0, 2.0);
    project->addFeature(box, viewWidget->getRoot());
    project->update();
    project->updateVisual();
    
    box->setParameters(10.0, 6.0, 1.0);
    project->update();

    viewWidget->update();
}

void MainWindow::constructionSphereSlot()
{
    Application *application = dynamic_cast<Application *>(qApp);
    assert(application);
    Project *project = application->getProject();
    
    std::shared_ptr<Feature::Sphere> sphere(new Feature::Sphere());
    project->addFeature(sphere, viewWidget->getRoot());
    project->update();
    project->updateVisual();

    viewWidget->update();
}

void MainWindow::constructionConeSlot()
{
    Application *application = dynamic_cast<Application *>(qApp);
    assert(application);
    Project *project = application->getProject();
    
    std::shared_ptr<Feature::Cone> cone(new Feature::Cone());
    project->addFeature(cone, viewWidget->getRoot());
    project->update();
    project->updateVisual();

    viewWidget->update();
}

void MainWindow::constructionCylinderSlot()
{
  Application *application = dynamic_cast<Application *>(qApp);
  assert(application);
  Project *project = application->getProject();
  
  std::shared_ptr<Feature::Cylinder> cylinder(new Feature::Cylinder());
  project->addFeature(cylinder, viewWidget->getRoot());
  project->update();
  project->updateVisual();

  viewWidget->update();
}


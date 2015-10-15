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

#include <iostream>
#include <assert.h>
#include <limits>


#include <QHBoxLayout>
#include <QFileDialog>
#include <QSplitter>

#include <BRepTools.hxx>

#include "mainwindow.h"
#include "dagview/dagmodel.h"
#include "dagview/dagview.h"
#include "application.h"
#include "ui_mainwindow.h"
#include "viewerwidget.h"
#include <selection/manager.h>
#include "project/project.h"
#include "command/commandmanager.h"
#include "feature/box.h"
#include "feature/sphere.h"
#include "feature/cone.h"
#include "feature/cylinder.h"
#include "feature/blend.h"
#include "feature/union.h"
#include "dialogs/boxdialog.h"
#include "preferences/dialog.h"

using boost::uuids::uuid;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    viewWidget = new ViewerWidget(osgViewer::ViewerBase::SingleThreaded);
    viewWidget->setGeometry( 100, 100, 800, 600 );
    
    dagModel = new DAG::Model(this);
    dagView = new DAG::View(this);
    dagView->setScene(dagModel);
    
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(viewWidget);
    splitter->addWidget(dagView);
    
    QHBoxLayout *aLayout = new QHBoxLayout();
    aLayout->addWidget(splitter);
    ui->centralwidget->setLayout(aLayout);

    selectionManager = new Selection::Manager(this);
    setupSelectionToolbar();
    connect(selectionManager, SIGNAL(setSelectionMask(int)), viewWidget, SLOT(setSelectionMask(int)));
    selectionManager->setState
    (
      Selection::All &
      ~Selection::ObjectsSelectable &
      ~Selection::FeaturesSelectable &
      ~Selection::SolidsSelectable &
      ~Selection::ShellsSelectable &
      ~Selection::FacesSelectable &
      ~Selection::WiresSelectable &
      ~Selection::EdgesSelectable &
      ~Selection::MidPointsSelectable &
      ~Selection::CenterPointsSelectable &
      ~Selection::QuadrantPointsSelectable &
      ~Selection::NearestPointsSelectable &
      ~Selection::ScreenPointsSelectable
    );

    Project *project = new Project();
    Application *application = dynamic_cast<Application *>(qApp);
    assert(application);
    application->setProject(project);
    
    project->connectFeatureAdded(boost::bind(&DAG::Model::featureAddedSlot, dagModel, _1));
    project->connectProjectUpdated(boost::bind(&DAG::Model::projectUpdatedSlot, dagModel));
    project->connectConnectionAdded(boost::bind(&DAG::Model::connectionAddedSlot, dagModel, _1, _2, _3));

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

void MainWindow::writeBrepSlot()
{
  QString fileName = QFileDialog::getSaveFileName(ui->centralwidget, tr("Save File"),
                           "/home/tanderson/temp", tr("brep (*.brep *.brp)"));

    if (fileName.isEmpty())
        return;
    
  const Selection::Containers &selections = viewWidget->getSelections();
  if (selections.empty())
    return;
  if(selections.at(0).selectionType != Selection::Type::Object)
  {
    viewWidget->clearSelections();
    return;
  }
  
  Application *application = dynamic_cast<Application *>(qApp);
  assert(application);
  Project *project = application->getProject();
  
  BRepTools::Write(project->findFeature(selections.at(0).featureId)->getShape(), fileName.toStdString().c_str());
  
  viewWidget->clearSelections();
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
    selectionManager->actionSelectEndPoints = ui->actionSelectEndPoints;
    selectionManager->actionSelectMidPoints = ui->actionSelectMidPoints;
    selectionManager->actionSelectCenterPoints = ui->actionSelectCenterPoints;
    selectionManager->actionSelectQuadrantPoints = ui->actionSelectQuandrantPoints;
    selectionManager->actionSelectNearestPoints = ui->actionSelectNearestPoints;
    selectionManager->actionSelectScreenPoints = ui->actionSelectScreenPoints;

    connect(ui->actionSelectObjects, SIGNAL(triggered(bool)), selectionManager, SLOT(triggeredObjects(bool)));
    connect(ui->actionSelectFeatures, SIGNAL(triggered(bool)), selectionManager, SLOT(triggeredFeatures(bool)));
    connect(ui->actionSelectSolids, SIGNAL(triggered(bool)), selectionManager, SLOT(triggeredSolids(bool)));
    connect(ui->actionSelectShells, SIGNAL(triggered(bool)), selectionManager, SLOT(triggeredShells(bool)));
    connect(ui->actionSelectFaces, SIGNAL(triggered(bool)), selectionManager, SLOT(triggeredFaces(bool)));
    connect(ui->actionSelectWires, SIGNAL(triggered(bool)), selectionManager, SLOT(triggeredWires(bool)));
    connect(ui->actionSelectEdges, SIGNAL(triggered(bool)), selectionManager, SLOT(triggeredEdges(bool)));
    connect(ui->actionSelectVertices, SIGNAL(triggered(bool)), selectionManager, SLOT(triggeredVertices(bool)));
    connect(ui->actionSelectEndPoints, SIGNAL(triggered(bool)), selectionManager, SLOT(triggeredEndPoints(bool)));
    connect(ui->actionSelectMidPoints, SIGNAL(triggered(bool)), selectionManager, SLOT(triggeredMidPoints(bool)));
    connect(ui->actionSelectCenterPoints, SIGNAL(triggered(bool)), selectionManager, SLOT(triggeredCenterPoints(bool)));
    connect(ui->actionSelectQuandrantPoints, SIGNAL(triggered(bool)), selectionManager, SLOT(triggeredQuadrantPoints(bool)));
    connect(ui->actionSelectNearestPoints, SIGNAL(triggered(bool)), selectionManager, SLOT(triggeredNearestPoints(bool)));
    connect(ui->actionSelectScreenPoints, SIGNAL(triggered(bool)), selectionManager, SLOT(triggeredScreenPoints(bool)));
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
    
    QAction *constructionBlendAction = new QAction(qApp);
    connect(constructionBlendAction, SIGNAL(triggered(bool)), this, SLOT(constructionBlendSlot()));
    Command constructionBlendCommand(CommandConstants::ConstructionBlend, "Construct Blend", constructionBlendAction);
    CommandManager::getManager().addCommand(constructionBlendCommand);
    
    QAction *constructionUnionAction = new QAction(qApp);
    connect(constructionUnionAction, SIGNAL(triggered(bool)), this, SLOT(constructionUnionSlot()));
    Command constructionUnionCommand(CommandConstants::ConstructionUnion, "Construct Union", constructionUnionAction);
    CommandManager::getManager().addCommand(constructionUnionCommand);
    
    QAction *fileImportOCCAction = new QAction(qApp);
    connect(fileImportOCCAction, SIGNAL(triggered(bool)), this, SLOT(readBrepSlot()));
    Command fileImportOCCCommand(CommandConstants::FileImportOCC, "Import BRep", fileImportOCCAction);
    CommandManager::getManager().addCommand(fileImportOCCCommand);
    
    QAction *fileExportOSGAction = new QAction(qApp);
    connect(fileExportOSGAction, SIGNAL(triggered(bool)), viewWidget, SLOT(writeOSGSlot()));
    Command fileExportOSGCommand(CommandConstants::FileExportOSG, "Export OSG", fileExportOSGAction);
    CommandManager::getManager().addCommand(fileExportOSGCommand);
    
    QAction *fileExportOCCAction = new QAction(qApp);
    connect(fileExportOCCAction, SIGNAL(triggered(bool)), this, SLOT(writeBrepSlot()));
    Command fileExportOCCCommand(CommandConstants::FileExportOCC, "Export BRep", fileExportOCCAction);
    CommandManager::getManager().addCommand(fileExportOCCCommand);
    
    QAction *preferencesAction = new QAction(qApp);
    connect(preferencesAction, SIGNAL(triggered(bool)), this, SLOT(preferencesSlot()));
    Command preferencesCommand(CommandConstants::Preferences, "Preferences", preferencesAction);
    CommandManager::getManager().addCommand(preferencesCommand);
}

void MainWindow::constructionBoxSlot()
{
    Application *application = dynamic_cast<Application *>(qApp);
    assert(application);
    Project *project = application->getProject();
    
    Feature::Box *box = nullptr;
    const Selection::Containers &selections = viewWidget->getSelections();
    //find first box.
    for (const auto &currentSelection : selections)
    {
      Feature::Base *feature = project->findFeature(currentSelection.featureId);
      assert(feature);
      if (feature->getType() != Feature::Type::Box)
        continue;
      
      box = dynamic_cast<Feature::Box*>(feature);
      assert(box);
      break;
    }
    
    viewWidget->clearSelections();
    
    BoxDialog dialog;
    dialog.setModal(true);
    if (box)
    {
      //editing.
      dialog.setParameters(box->getLength(), box->getWidth(), box->getHeight());
      if (!dialog.exec())
        return;
    }
    else
    {
      //constructing.
      dialog.setParameters(20.0, 10.0, 2.0);
      
      if (!dialog.exec())
        return;
      
      std::shared_ptr<Feature::Box> boxPtr(new Feature::Box());
      box = boxPtr.get();
      gp_Ax2 location;
      location.SetLocation(gp_Pnt(1.0, 1.0, 1.0));
      boxPtr->setSystem(location);
      project->addFeature(boxPtr, viewWidget->getRoot());
    }
    
    box->setParameters
    (
      dialog.lengthEdit->text().toDouble(),
      dialog.widthEdit->text().toDouble(),
      dialog.heightEdit->text().toDouble()
    );
    project->update();
    project->updateVisual();
    
    viewWidget->update();
}

void MainWindow::constructionSphereSlot()
{
    Application *application = dynamic_cast<Application *>(qApp);
    assert(application);
    Project *project = application->getProject();
    
    std::shared_ptr<Feature::Sphere> sphere(new Feature::Sphere());
    sphere->setRadius(2.0);
    gp_Ax2 location;
    location.SetLocation(gp_Pnt(6.0, 6.0, 5.0));
    sphere->setSystem(location);
    
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
    cone->setRadius1(2.0);
    cone->setRadius2(0.5);
    cone->setHeight(8.0);
    gp_Ax2 location;
    location.SetLocation(gp_Pnt(11.0, 6.0, 3.0));
    cone->setSystem(location);
    
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
  cylinder->setRadius(2.0);
  cylinder->setHeight(8.0);
  gp_Ax2 location;
  location.SetLocation(gp_Pnt(16.0, 6.0, 3.0));
  cylinder->setSystem(location);
  
  
  project->addFeature(cylinder, viewWidget->getRoot());
  project->update();
  project->updateVisual();

  viewWidget->update();
}

void MainWindow::constructionBlendSlot()
{
  const Selection::Containers &selections = viewWidget->getSelections();
  if (selections.empty())
    return;
  
  //get targetId and filter out edges not belonging to first target.
  uuid targetFeatureId = selections.at(0).featureId;
  std::vector<uuid> edgeIds;
  for (const auto &currentSelection : selections)
  {
    if
    (
      currentSelection.featureId != targetFeatureId ||
      currentSelection.selectionType != Selection::Type::Edge //just edges for now.
    )
      continue;
    
    edgeIds.push_back(currentSelection.shapeId);
  }
  
  Application *application = dynamic_cast<Application *>(qApp);
  assert(application);
  Project *project = application->getProject();
  
  std::shared_ptr<Feature::Blend> blend(new Feature::Blend());
  project->addFeature(blend, viewWidget->getRoot());
  project->connect(targetFeatureId, blend->getId(), Feature::InputTypes::target);
  blend->setEdgeIds(edgeIds);
  
  Feature::Base *targetFeature = project->findFeature(targetFeatureId);
  targetFeature->hide3D();
  viewWidget->clearSelections();
  
  project->update();
  project->updateVisual();
}

void MainWindow::constructionUnionSlot()
{
  const Selection::Containers &selections = viewWidget->getSelections();
  if (selections.size() < 2)
    return;
  
  //for now only accept objects.
  if
  (
    selections.at(0).selectionType != Selection::Type::Object ||
    selections.at(1).selectionType != Selection::Type::Object
  )
    return;
    
  uuid targetFeatureId = selections.at(0).featureId;
  uuid toolFeatureId = selections.at(1).featureId; //only 1 tool right now.
  
  Application *application = dynamic_cast<Application *>(qApp);
  assert(application);
  Project *project = application->getProject();
  
  project->findFeature(targetFeatureId)->hide3D();
  project->findFeature(toolFeatureId)->hide3D();
  viewWidget->clearSelections();
  
  //union keyword. whoops
  std::shared_ptr<Feature::Union> onion(new Feature::Union());
  project->addFeature(onion, viewWidget->getRoot());
  project->connect(targetFeatureId, onion->getId(), Feature::InputTypes::target);
  project->connect(toolFeatureId, onion->getId(), Feature::InputTypes::tool);
  
  project->update();
  project->updateVisual();

  viewWidget->update();
}

void MainWindow::preferencesSlot()
{
  std::unique_ptr<Preferences::Dialog> dialog(new Preferences::Dialog(dynamic_cast<Application *>(qApp)->getPreferencesManager()));
  dialog->setModal(true);
  if (!dialog->exec())
    return;
  if (dialog->isVisualDirty())
  {
    Application *application = dynamic_cast<Application *>(qApp);
    assert(application);
    Project *project = application->getProject();
    project->setAllVisualDirty();
    project->updateVisual();
  }
}


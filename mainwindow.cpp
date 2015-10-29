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
#include <QDir>

#include <BRepTools.hxx>

#include <mainwindow.h>
#include <dagview/dagmodel.h>
#include <dagview/dagview.h>
#include <application.h>
#include <ui_mainwindow.h>
#include <viewer/viewerwidget.h>
#include <selection/manager.h>
#include <project/project.h>
#include <command/manager.h>
#include <feature/box.h>
#include <feature/sphere.h>
#include <feature/cone.h>
#include <feature/cylinder.h>
#include <feature/blend.h>
#include <feature/union.h>
#include <dialogs/boxdialog.h>
#include <preferences/dialog.h>
#include <message/dispatch.h>

using boost::uuids::uuid;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    viewWidget = new ViewerWidget(osgViewer::ViewerBase::SingleThreaded);
    viewWidget->setGeometry( 100, 100, 800, 600 );
    
    dagModel = new dag::Model(this);
    dagView = new dag::View(this);
    dagView->setScene(dagModel);
    
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(viewWidget);
    splitter->addWidget(dagView);
    //size setup temp.
    QList<int> sizes;
    sizes.append(1000);
    sizes.append(300);
    splitter->setSizes(sizes);
    
    QHBoxLayout *aLayout = new QHBoxLayout();
    aLayout->addWidget(splitter);
    ui->centralwidget->setLayout(aLayout);

    selectionManager = new slc::Manager(this);
    setupSelectionToolbar();
    connect(selectionManager, SIGNAL(setSelectionMask(int)), viewWidget, SLOT(setSelectionMask(int)));
    selectionManager->setState
    (
      slc::All &
      ~slc::ObjectsSelectable &
      ~slc::FeaturesSelectable &
      ~slc::SolidsSelectable &
      ~slc::ShellsSelectable &
      ~slc::FacesSelectable &
      ~slc::WiresSelectable &
      ~slc::EdgesSelectable &
      ~slc::MidPointsSelectable &
      ~slc::CenterPointsSelectable &
      ~slc::QuadrantPointsSelectable &
      ~slc::NearestPointsSelectable &
      ~slc::ScreenPointsSelectable
    );

    prj::Project *project = new prj::Project();
    app::Application *application = dynamic_cast<app::Application *>(qApp);
    assert(application);
    application->setProject(project);
    
    //new message system.
    project->connectMessageOut(boost::bind(&msg::Dispatch::messageInSlot, &msg::dispatch(), _1));
    msg::dispatch().connectMessageOut(boost::bind(&prj::Project::messageInSlot, project, _1));
    dagModel->connectMessageOut(boost::bind(&msg::Dispatch::messageInSlot, &msg::dispatch(), _1));
    msg::dispatch().connectMessageOut(boost::bind(&dag::Model::messageInSlot, dagModel, _1));
    viewWidget->getSelectionEventHandler()->connectMessageOut
      (boost::bind(&msg::Dispatch::messageInSlot, &msg::dispatch(), _1));
    msg::dispatch().connectMessageOut(boost::bind(&slc::EventHandler::messageInSlot,
						  viewWidget->getSelectionEventHandler(), _1));
    viewWidget->connectMessageOut(boost::bind(&msg::Dispatch::messageInSlot, &msg::dispatch(), _1));
    msg::dispatch().connectMessageOut(boost::bind(&ViewerWidget::messageInSlot, viewWidget, _1));

    setupCommands();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::readBrepSlot()
{
    QString fileName = QFileDialog::getOpenFileName(ui->centralwidget, tr("Open File"),
                                                    QDir::homePath(), tr("brep (*.brep *.brp)"));
    if (fileName.isEmpty())
        return;

    app::Application *application = dynamic_cast<app::Application *>(qApp);
    assert(application);
    prj::Project *project = application->getProject();
    project->readOCC(fileName.toStdString());
    project->update();
    project->updateVisual();
    viewWidget->update();
}

void MainWindow::writeBrepSlot()
{
  QString fileName = QFileDialog::getSaveFileName(ui->centralwidget, tr("Save File"),
                           QDir::homePath(), tr("brep (*.brep *.brp)"));

    if (fileName.isEmpty())
        return;
    
  const slc::Containers &selections = viewWidget->getSelections();
  if (selections.empty())
    return;
  if(selections.at(0).selectionType != slc::Type::Object)
  {
    viewWidget->clearSelections();
    return;
  }
  
  app::Application *application = dynamic_cast<app::Application *>(qApp);
  assert(application);
  prj::Project *project = application->getProject();
  
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
    cmd::Command constructionBoxCommand(cmd::ConstructionBox, "Construct Box", constructionBoxAction);
    cmd::Manager::getManager().addCommand(constructionBoxCommand);

    QAction *constructionSphereAction = new QAction(qApp);
    connect(constructionSphereAction, SIGNAL(triggered()), this, SLOT(constructionSphereSlot()));
    cmd::Command constructionSphereCommand(cmd::ConstructionSphere, "Construct Sphere", constructionSphereAction);
    cmd::Manager::getManager().addCommand(constructionSphereCommand);

    QAction *constructionConeAction = new QAction(qApp);
    connect(constructionConeAction, SIGNAL(triggered()), this, SLOT(constructionConeSlot()));
    cmd::Command constructionConeCommand(cmd::ConstructionCone, "Construct Cone", constructionConeAction);
    cmd::Manager::getManager().addCommand(constructionConeCommand);
    
    QAction *constructionCylinderAction = new QAction(qApp);
    connect(constructionCylinderAction, SIGNAL(triggered(bool)), this, SLOT(constructionCylinderSlot()));
    cmd::Command constructionCylinderCommand(cmd::ConstructionCylinder, "Construct Cylinder", constructionCylinderAction);
    cmd::Manager::getManager().addCommand(constructionCylinderCommand);
    
    QAction *constructionBlendAction = new QAction(qApp);
    connect(constructionBlendAction, SIGNAL(triggered(bool)), this, SLOT(constructionBlendSlot()));
    cmd::Command constructionBlendCommand(cmd::ConstructionBlend, "Construct Blend", constructionBlendAction);
    cmd::Manager::getManager().addCommand(constructionBlendCommand);
    
    QAction *constructionUnionAction = new QAction(qApp);
    connect(constructionUnionAction, SIGNAL(triggered(bool)), this, SLOT(constructionUnionSlot()));
    cmd::Command constructionUnionCommand(cmd::ConstructionUnion, "Construct Union", constructionUnionAction);
    cmd::Manager::getManager().addCommand(constructionUnionCommand);
    
    QAction *fileImportOCCAction = new QAction(qApp);
    connect(fileImportOCCAction, SIGNAL(triggered(bool)), this, SLOT(readBrepSlot()));
    cmd::Command fileImportOCCCommand(cmd::FileImportOCC, "Import BRep", fileImportOCCAction);
    cmd::Manager::getManager().addCommand(fileImportOCCCommand);
    
    QAction *fileExportOSGAction = new QAction(qApp);
    connect(fileExportOSGAction, SIGNAL(triggered(bool)), viewWidget, SLOT(writeOSGSlot()));
    cmd::Command fileExportOSGCommand(cmd::FileExportOSG, "Export OSG", fileExportOSGAction);
    cmd::Manager::getManager().addCommand(fileExportOSGCommand);
    
    QAction *fileExportOCCAction = new QAction(qApp);
    connect(fileExportOCCAction, SIGNAL(triggered(bool)), this, SLOT(writeBrepSlot()));
    cmd::Command fileExportOCCCommand(cmd::FileExportOCC, "Export BRep", fileExportOCCAction);
    cmd::Manager::getManager().addCommand(fileExportOCCCommand);
    
    QAction *preferencesAction = new QAction(qApp);
    connect(preferencesAction, SIGNAL(triggered(bool)), this, SLOT(preferencesSlot()));
    cmd::Command preferencesCommand(cmd::Preferences, "Preferences", preferencesAction);
    cmd::Manager::getManager().addCommand(preferencesCommand);
    
    QAction *removeAction = new QAction(qApp);
    connect(removeAction, SIGNAL(triggered(bool)), this, SLOT(removeSlot()));
    cmd::Command removeCommand(cmd::Remove, "Remove", removeAction);
    cmd::Manager::getManager().addCommand(removeCommand);
}

void MainWindow::constructionBoxSlot()
{
    app::Application *application = dynamic_cast<app::Application *>(qApp);
    assert(application);
    prj::Project *project = application->getProject();
    
    ftr::Box *box = nullptr;
    const slc::Containers &selections = viewWidget->getSelections();
    //find first box.
    for (const auto &currentSelection : selections)
    {
      ftr::Base *feature = project->findFeature(currentSelection.featureId);
      assert(feature);
      if (feature->getType() != ftr::Type::Box)
        continue;
      
      box = dynamic_cast<ftr::Box*>(feature);
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
      
      std::shared_ptr<ftr::Box> boxPtr(new ftr::Box());
      box = boxPtr.get();
      gp_Ax2 location;
      location.SetLocation(gp_Pnt(1.0, 1.0, 1.0));
      boxPtr->setSystem(location);
      project->addFeature(boxPtr);
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
    app::Application *application = dynamic_cast<app::Application *>(qApp);
    assert(application);
    prj::Project *project = application->getProject();
    
    std::shared_ptr<ftr::Sphere> sphere(new ftr::Sphere());
    sphere->setRadius(2.0);
    gp_Ax2 location;
    location.SetLocation(gp_Pnt(6.0, 6.0, 5.0));
    sphere->setSystem(location);
    
    project->addFeature(sphere);
    project->update();
    project->updateVisual();

    viewWidget->update();
}

void MainWindow::constructionConeSlot()
{
    app::Application *application = dynamic_cast<app::Application *>(qApp);
    assert(application);
    prj::Project *project = application->getProject();
    
    std::shared_ptr<ftr::Cone> cone(new ftr::Cone());
    cone->setRadius1(2.0);
    cone->setRadius2(0.5);
    cone->setHeight(8.0);
    gp_Ax2 location;
    location.SetLocation(gp_Pnt(11.0, 6.0, 3.0));
    cone->setSystem(location);
    
    project->addFeature(cone);
    project->update();
    project->updateVisual();

    viewWidget->update();
}

void MainWindow::constructionCylinderSlot()
{
  app::Application *application = dynamic_cast<app::Application *>(qApp);
  assert(application);
  prj::Project *project = application->getProject();
  
  std::shared_ptr<ftr::Cylinder> cylinder(new ftr::Cylinder());
  cylinder->setRadius(2.0);
  cylinder->setHeight(8.0);
  gp_Ax2 location;
  location.SetLocation(gp_Pnt(16.0, 6.0, 3.0));
  cylinder->setSystem(location);
  
  
  project->addFeature(cylinder);
  project->update();
  project->updateVisual();

  viewWidget->update();
}

void MainWindow::constructionBlendSlot()
{
  const slc::Containers &selections = viewWidget->getSelections();
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
      currentSelection.selectionType != slc::Type::Edge //just edges for now.
    )
      continue;
    
    edgeIds.push_back(currentSelection.shapeId);
  }
  
  app::Application *application = dynamic_cast<app::Application *>(qApp);
  assert(application);
  prj::Project *project = application->getProject();
  
  std::shared_ptr<ftr::Blend> blend(new ftr::Blend());
  project->addFeature(blend);
  project->connect(targetFeatureId, blend->getId(), ftr::InputTypes::target);
  blend->setEdgeIds(edgeIds);
  
  ftr::Base *targetFeature = project->findFeature(targetFeatureId);
  targetFeature->hide3D();
  viewWidget->clearSelections();
  
  project->update();
  project->updateVisual();
}

void MainWindow::constructionUnionSlot()
{
  const slc::Containers &selections = viewWidget->getSelections();
  if (selections.size() < 2)
    return;
  
  //for now only accept objects.
  if
  (
    selections.at(0).selectionType != slc::Type::Object ||
    selections.at(1).selectionType != slc::Type::Object
  )
    return;
    
  uuid targetFeatureId = selections.at(0).featureId;
  uuid toolFeatureId = selections.at(1).featureId; //only 1 tool right now.
  
  app::Application *application = dynamic_cast<app::Application *>(qApp);
  assert(application);
  prj::Project *project = application->getProject();
  
  project->findFeature(targetFeatureId)->hide3D();
  project->findFeature(toolFeatureId)->hide3D();
  viewWidget->clearSelections();
  
  //union keyword. whoops
  std::shared_ptr<ftr::Union> onion(new ftr::Union());
  project->addFeature(onion);
  project->connect(targetFeatureId, onion->getId(), ftr::InputTypes::target);
  project->connect(toolFeatureId, onion->getId(), ftr::InputTypes::tool);
  
  project->update();
  project->updateVisual();

  viewWidget->update();
}

void MainWindow::preferencesSlot()
{
  std::unique_ptr<prf::Dialog> dialog(new prf::Dialog(dynamic_cast<app::Application *>(qApp)->getPreferencesManager()));
  dialog->setModal(true);
  if (!dialog->exec())
    return;
  if (dialog->isVisualDirty())
  {
    app::Application *application = dynamic_cast<app::Application *>(qApp);
    assert(application);
    prj::Project *project = application->getProject();
    project->setAllVisualDirty();
    project->updateVisual();
  }
}

void MainWindow::removeSlot()
{
  app::Application *application = dynamic_cast<app::Application *>(qApp);
  assert(application);
  prj::Project *project = application->getProject();
  
  slc::Containers selections = viewWidget->getSelections();
  viewWidget->clearSelections();
  for (const auto &current : selections)
    project->removeFeature(current.featureId);
  
  project->update();
  project->updateVisual();
}



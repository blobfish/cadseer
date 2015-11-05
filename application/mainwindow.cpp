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
#include <QSplitter>
#include <QDir>

#include <application/mainwindow.h>
#include <dagview/dagmodel.h>
#include <dagview/dagview.h>
#include <application/application.h>
#include <ui_mainwindow.h>
#include <viewer/viewerwidget.h>
#include <selection/manager.h>
#include <project/project.h>
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
using namespace app;

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

    //new message system.
    dagModel->connectMessageOut(boost::bind(&msg::Dispatch::messageInSlot, &msg::dispatch(), _1));
    msg::dispatch().connectMessageOut(boost::bind(&dag::Model::messageInSlot, dagModel, _1));
    viewWidget->getSelectionEventHandler()->connectMessageOut
      (boost::bind(&msg::Dispatch::messageInSlot, &msg::dispatch(), _1));
    msg::dispatch().connectMessageOut(boost::bind(&slc::EventHandler::messageInSlot,
						  viewWidget->getSelectionEventHandler(), _1));
    viewWidget->connectMessageOut(boost::bind(&msg::Dispatch::messageInSlot, &msg::dispatch(), _1));
    msg::dispatch().connectMessageOut(boost::bind(&ViewerWidget::messageInSlot, viewWidget, _1));
}

MainWindow::~MainWindow()
{
    delete ui;
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

// void MainWindow::constructionBoxSlot()
// {
//     app::Application *application = dynamic_cast<app::Application *>(qApp);
//     assert(application);
//     prj::Project *project = application->getProject();
//     
//     ftr::Box *box = nullptr;
//     const slc::Containers &selections = viewWidget->getSelections();
//     //find first box.
//     for (const auto &currentSelection : selections)
//     {
//       ftr::Base *feature = project->findFeature(currentSelection.featureId);
//       assert(feature);
//       if (feature->getType() != ftr::Type::Box)
//         continue;
//       
//       box = dynamic_cast<ftr::Box*>(feature);
//       assert(box);
//       break;
//     }
//     
//     viewWidget->clearSelections();
//     
//     BoxDialog dialog;
//     dialog.setModal(true);
//     if (box)
//     {
//       //editing.
//       dialog.setParameters(box->getLength(), box->getWidth(), box->getHeight());
//       if (!dialog.exec())
//         return;
//     }
//     else
//     {
//       //constructing.
//       dialog.setParameters(20.0, 10.0, 2.0);
//       
//       if (!dialog.exec())
//         return;
//       
//       std::shared_ptr<ftr::Box> boxPtr(new ftr::Box());
//       box = boxPtr.get();
//       gp_Ax2 location;
//       location.SetLocation(gp_Pnt(1.0, 1.0, 1.0));
//       boxPtr->setSystem(location);
//       project->addFeature(boxPtr);
//     }
//     
//     box->setParameters
//     (
//       dialog.lengthEdit->text().toDouble(),
//       dialog.widthEdit->text().toDouble(),
//       dialog.heightEdit->text().toDouble()
//     );
//     project->updateModel();
//     project->updateVisual();
// }

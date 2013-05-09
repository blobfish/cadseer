#include <iostream>

#include <QHBoxLayout>
#include <QFileDialog>
#include <QTimer>

#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepMesh.hxx>

#include "mainwindow.h"
#include "application.h"
#include "ui_mainwindow.h"
#include "viewerwidget.h"
#include "selectionmanager.h"
#include "document.h"

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
    connect(ui->actionSelectFaces, SIGNAL(toggled(bool)), selectionManager, SLOT(toggledFaces(bool)));
    connect(ui->actionSelectEdges, SIGNAL(toggled(bool)), selectionManager, SLOT(toggledEdges(bool)));
    connect(ui->actionSelectVertices, SIGNAL(toggled(bool)), selectionManager, SLOT(toggledVertices(bool)));
    connect(selectionManager, SIGNAL(guiFacesEnabled(bool)), ui->actionSelectFaces, SLOT(setEnabled(bool)));
    connect(selectionManager, SIGNAL(guiEdgesEnabled(bool)), ui->actionSelectEdges, SLOT(setEnabled(bool)));
    connect(selectionManager, SIGNAL(guiVerticesEnabled(bool)), ui->actionSelectVertices, SLOT(setEnabled(bool)));
    connect(selectionManager, SIGNAL(guiFacesSelectable(bool)), ui->actionSelectFaces, SLOT(setChecked(bool)));
    connect(selectionManager, SIGNAL(guiEdgesSelectable(bool)), ui->actionSelectEdges, SLOT(setChecked(bool)));
    connect(selectionManager, SIGNAL(guiVerticesSelectable(bool)), ui->actionSelectVertices, SLOT(setChecked(bool)));
    connect(selectionManager, SIGNAL(setSelectionMask(int)), viewWidget, SLOT(setSelectionMask(int)));

    SelectionState state;
    state.facesEnabled = true;
    state.edgesEnabled = true;
    state.verticesEnabled = true;
    state.facesSelectable = true;
    state.edgesSelectable = true;
    state.verticesSelectable = true;
    selectionManager->setState(state);

    connect(ui->actionAppendBrep, SIGNAL(triggered()), this, SLOT(readBrepSlot()));
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

    document = new Document(qApp);
    document->readOCC(fileName.toStdString(), viewWidget->getRoot());
    viewWidget->update();
}

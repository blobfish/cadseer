#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <osg/Node>

class ViewerWidget;
class SelectionManager;
class Project;

namespace DAG{class View; class Model;}

namespace Ui {
class MainWindow;
}

class TopoDS_Face;
class TopoDS_Shape;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public Q_SLOTS:
    void readBrepSlot();
    void constructionBoxSlot();
    void constructionSphereSlot();
    void constructionConeSlot();
    void constructionCylinderSlot();
    void constructionBlendSlot();
    
private:
    void setupSelectionToolbar();
    void setupCommands();
    Ui::MainWindow *ui;
    ViewerWidget* viewWidget;
    DAG::Model *dagModel;
    DAG::View *dagView;
    SelectionManager *selectionManager;
};

#endif // MAINWINDOW_H

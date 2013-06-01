#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <osg/Node>

class ViewerWidget;
class SelectionManager;
class Document;

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

public slots:
    void readBrepSlot();
    
private:
    void setupSelectionToolbar();
    Ui::MainWindow *ui;
    ViewerWidget* viewWidget;
    SelectionManager *selectionManager;
};

#endif // MAINWINDOW_H

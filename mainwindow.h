#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <osg/Node>

class ViewerWidget;

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
    void addNode(osg::Node *node);

public slots:
    void readBrepSlot();
    
private:
    Ui::MainWindow *ui;
    ViewerWidget* viewWidget;
};

#endif // MAINWINDOW_H

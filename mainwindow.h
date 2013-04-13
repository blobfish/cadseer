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

    osg::ref_ptr<osg::Geode> meshFace(const TopoDS_Face &face);
    osg::ref_ptr<osg::Group> buildModel(TopoDS_Shape &shape);
};

#endif // MAINWINDOW_H

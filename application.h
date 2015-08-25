#ifndef APPLICATION_H
#define APPLICATION_H

#include <QApplication>

class MainWindow;
class Project;

class Application : public QApplication
{
    Q_OBJECT
public:
    explicit Application(int &argc, char **argv);
    ~Application();
    bool x11EventFilter(XEvent *event);
    void initializeSpaceball(MainWindow *mainWindowIn);
    void setProject(Project *projectIn){project = projectIn;}
    Project *getProject(){return project;}

public slots:
    void quittingSlot();
private:
    MainWindow *mainWindow = nullptr;
    Project *project = nullptr;
    bool spaceballPresent;
};

#endif // APPLICATION_H

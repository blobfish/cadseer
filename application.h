#ifndef APPLICATION_H
#define APPLICATION_H

#include <QApplication>

class MainWindow;

class Application : public QApplication
{
    Q_OBJECT
public:
    explicit Application(int &argc, char **argv);
    ~Application();
    bool x11EventFilter(XEvent *event);
    void initializeSpaceball(MainWindow *mainWindowIn);
public slots:
    void quittingSlot();
private:
    MainWindow *mainWindow;
    bool spaceballPresent;
};

#endif // APPLICATION_H

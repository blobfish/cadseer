#ifndef APPLICATION_H
#define APPLICATION_H

#include <QApplication>

class MainWindow;
class Document;

class Application : public QApplication
{
    Q_OBJECT
public:
    explicit Application(int &argc, char **argv);
    ~Application();
    bool x11EventFilter(XEvent *event);
    void initializeSpaceball(MainWindow *mainWindowIn);
    void setDocument(Document *documentIn){document = documentIn;}
    Document *getDocument(){return document;}

public slots:
    void quittingSlot();
private:
    MainWindow *mainWindow;
    Document *document;
    bool spaceballPresent;
};

#endif // APPLICATION_H

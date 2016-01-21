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

#ifndef APPLICATION_H
#define APPLICATION_H

#include <memory>

#include <QApplication>
#include <QDir>

#ifndef Q_MOC_RUN
#include <boost/signals2.hpp>
#include <message/message.h>
#endif



namespace prf{class Manager;}
namespace prj{class Project;}

namespace app
{
class MainWindow;
class Factory;
class Application : public QApplication
{
    Q_OBJECT
public:
    explicit Application(int &argc, char **argv);
    ~Application();
    bool x11EventFilter(XEvent *event);
    void initializeSpaceball();
    prj::Project* getProject(){return project.get();}
    MainWindow* getMainWindow(){return mainWindow.get();}
    QDir getApplicationDirectory();
    
    //new messaging system
    void messageInSlot(const msg::Message &);
    typedef boost::signals2::signal<void (const msg::Message &)> MessageOutSignal;
    boost::signals2::connection connectMessageOut(const MessageOutSignal::slot_type &subscriber)
    {
      return messageOutSignal.connect(subscriber);
    }

public Q_SLOTS:
    void quittingSlot();
    void appStartSlot();
private:
    std::unique_ptr<MainWindow> mainWindow;
    std::unique_ptr<prj::Project> project;
    std::unique_ptr<Factory> factory;
    bool spaceballPresent;
    
    void createNewProject(const std::string &);
    void openProject(const std::string &);
    void closeProject();
    void updateTitle();
    
    MessageOutSignal messageOutSignal;
    msg::MessageDispatcher dispatcher;
    void setupDispatcher();
    void newProjectRequestDispatched(const msg::Message &);
    void openProjectRequestDispatched(const msg::Message &);
    void ProjectDialogRequestDispatched(const msg::Message &);
};
}

#endif // APPLICATION_H

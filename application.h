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

#include <QApplication>
#include <QDir>

#include <memory>

class MainWindow;
namespace prf{class Manager;}
namespace prj{class Project;}

namespace app
{
class Application : public QApplication
{
    Q_OBJECT
public:
    explicit Application(int &argc, char **argv);
    ~Application();
    bool x11EventFilter(XEvent *event);
    void initializeSpaceball();
    void setProject(prj::Project *projectIn){project = projectIn;}
    prj::Project* getProject(){return project;}
    prf::Manager* getPreferencesManager(){return preferenceManager.get();}
    MainWindow* getMainWindow(){return mainWindow.get();}
    QDir getApplicationDirectory();

public Q_SLOTS:
    void quittingSlot();
private:
    std::unique_ptr<MainWindow> mainWindow;
    prj::Project *project = nullptr;
    bool spaceballPresent;
    std::unique_ptr<prf::Manager> preferenceManager;
};
}

#endif // APPLICATION_H

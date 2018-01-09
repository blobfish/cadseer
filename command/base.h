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

#ifndef CMD_BASE_H
#define CMD_BASE_H

#include <string>
#include <memory>

#include <QObject> //for string translation.

#include <selection/message.h> //for derived classes

namespace app{class Application; class MainWindow;}
namespace prj{class Project;}
namespace slc{class Manager; class EventHandler;}
namespace msg{class Message; class Observer;}
namespace vwr{class Widget;}

namespace cmd
{
  /*! @brief base class for commands.
   * 
   */
  class Base
  {
  public:
    Base();
    virtual ~Base();
    virtual std::string getCommandName() = 0;
    virtual std::string getStatusMessage() = 0;
    virtual void activate() = 0;
    virtual void deactivate() = 0;
    
  protected:
    std::unique_ptr<msg::Observer> observer;
    void sendDone();
    
    app::Application *application;
    app::MainWindow *mainWindow;
    prj::Project *project;
    slc::Manager *selectionManager;
    slc::EventHandler *eventHandler;
    vwr::Widget *viewer;
    
    bool isActive;
  };
  
  typedef std::shared_ptr<Base> BasePtr;
}

#endif // CMD_BASE_H

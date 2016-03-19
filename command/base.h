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

#include <boost/signals2.hpp>

#include <message/message.h>

namespace app{class Application; class MainWindow;}
namespace prj{class Project;}
namespace slc{class Manager; class EventHandler; class Message;}

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
    typedef boost::signals2::signal<void (const msg::Message&)> MessageOutSignal;
    boost::signals2::connection connectMessageOut(const MessageOutSignal::slot_type &subscriber)
    {
      return messageOutSignal.connect(subscriber);
    }
    boost::signals2::scoped_connection connection; //connection to dispatch.
    void messageInSlot(const msg::Message&);
    MessageOutSignal messageOutSignal;
    msg::MessageDispatcher dispatcher;
    void sendDone();
    
    app::Application *application;
    app::MainWindow *mainWindow;
    prj::Project *project;
    slc::Manager *selectionManager;
    slc::EventHandler *eventHandler;
    
    bool isActive;
  };
  
  typedef std::shared_ptr<Base> BasePtr;
}

#endif // CMD_BASE_H

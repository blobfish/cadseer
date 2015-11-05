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

#ifndef APP_FACTORY_H
#define APP_FACTORY_H

#include <functional>
#include <map>

#include <boost/signals2.hpp>

#include <message/message.h>
#include <application/message.h>
#include <selection/container.h>

namespace prj{class Project;}
class ViewerWidget;

namespace app
{
  class Factory
  {
  public:
    Factory();
    void messageInSlot(const msg::Message &);
    typedef boost::signals2::signal<void (const msg::Message &)> MessageOutSignal;
    boost::signals2::connection connectMessageOut(const MessageOutSignal::slot_type &subscriber)
    {
      return messageOutSignal.connect(subscriber);
    }
//     void setViewer(ViewerWidget *viewerIn){viewer = viewerIn;}
    
  private:
    prj::Project *project = nullptr;
//     ViewerWidget *viewer = nullptr;
    slc::Containers containers;
    
    MessageOutSignal messageOutSignal;
    msg::MessageDispatcher dispatcher;
    void setupDispatcher();
    void triggerUpdate(); //!< just convenience.
    void newProjectDispatched(const msg::Message&);
    void selectionAdditionDispatched(const msg::Message&);
    void selectionSubtractionDispatched(const msg::Message&);
    void newBoxDispatched(const msg::Message&);
    void newCylinderDispatched(const msg::Message&);
    void newSphereDispatched(const msg::Message&);
    void newConeDispatched(const msg::Message&);
    void newUnionDispatched(const msg::Message&);
    void newBlendDispatched(const msg::Message&);
    void importOCCDispatched(const msg::Message&);
    void exportOCCDispatched(const msg::Message&);
    void preferencesDispatched(const msg::Message&);
    void removeDispatched(const msg::Message&);
  };
}

#endif // APP_FACTORY_H

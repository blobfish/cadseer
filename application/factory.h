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
#include <memory>

#include <selection/container.h>

namespace prj{class Project;}
namespace msg{class Message; class Observer;}

namespace app
{
  class Factory
  {
  public:
    Factory();
    
  private:
    prj::Project *project = nullptr;
    slc::Containers containers;
    
    std::unique_ptr<msg::Observer> observer;
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
    void newSubtractDispatched(const msg::Message&);
    void newIntersectDispatched(const msg::Message&);
    void newBlendDispatched(const msg::Message&);
    void newChamferDispatched(const msg::Message&);
    void newDraftDispatched(const msg::Message&);
    void newDatumPlaneDispatched(const msg::Message&);
    void newHollowDispatched(const msg::Message&);
    void importOCCDispatched(const msg::Message&);
    void exportOCCDispatched(const msg::Message&);
    void preferencesDispatched(const msg::Message&);
    void removeDispatched(const msg::Message&);
    void openProjectDispatched(const msg::Message&);
    void closeProjectDispatched(const msg::Message&);
    void debugDumpDispatched(const msg::Message&);
    void debugShapeTrackUpDispatched(const msg::Message&);
    void debugShapeGraphDispatched(const msg::Message&);
    void debugShapeTrackDownDispatched(const msg::Message&);
    void viewInfoDispatched(const msg::Message&);
    void linearMeasureDispatched(const msg::Message&);
    void viewIsolateDispatched(const msg::Message&);
    
    void messageStressTestDispatched(const msg::Message&); //testing
  };
}

#endif // APP_FACTORY_H

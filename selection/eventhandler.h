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

#ifndef SELECTIONEVENTHANDLER_H
#define SELECTIONEVENTHANDLER_H

#include <boost/signals2.hpp>

#include <osgGA/GUIEventHandler>
#include <osgUtil/LineSegmentIntersector>

#include <selection/container.h>
#include <message/message.h>
#include <selection/visitors.h>

namespace osg{class Switch;}
namespace mdv{class ShapeGeometry;}

namespace slc
{
class EventHandler : public osgGA::GUIEventHandler
{
public:
    EventHandler(osg::Group* viewerRootIn);
    const Containers& getSelections() const {return selectionContainers;}
    void clearSelections();
    void setSelectionMask(const unsigned int &maskIn);
    
    //new messaging system
    void messageInSlot(const msg::Message &);
    typedef boost::signals2::signal<void (const msg::Message &)> MessageOutSignal;
    boost::signals2::connection connectMessageOut(const MessageOutSignal::slot_type &subscriber)
    {
      return messageOutSignal.connect(subscriber);
    }
protected:
    virtual bool handle(const osgGA::GUIEventAdapter& eventAdapter,
                        osgGA::GUIActionAdapter& actionAdapter, osg::Object *object,
                        osg::NodeVisitor *nodeVistor);
    void setPrehighlight(Container &selected);
    void clearPrehighlight();
    bool alreadySelected(const Container &testContainer);
    void selectionOperation(const boost::uuids::uuid&, const std::vector<boost::uuids::uuid>&, HighlightVisitor::Operation);
    Container lastPrehighlight;
    osg::Vec4 preHighlightColor;
    osg::Vec4 selectionColor;
    Containers selectionContainers;
    osg::Geometry* buildTempPoint(const osg::Vec3d &pointIn);

    unsigned int nodeMask;
    unsigned int selectionMask;

    osgUtil::LineSegmentIntersector::Intersections currentIntersections;
    
    osg::ref_ptr<osg::Group> viewerRoot;
    
    MessageOutSignal messageOutSignal;
    msg::MessageDispatcher dispatcher;
    void setupDispatcher();
    void requestPreselectionAdditionDispatched(const msg::Message &);
    void requestPreselectionSubtractionDispatched(const msg::Message &);
    void requestSelectionAdditionDispatched(const msg::Message &);
    void requestSelectionSubtractionDispatched(const msg::Message &);
    void requestSelectionClearDispatched(const msg::Message &);
};
}

#endif // SELECTIONEVENTHANDLER_H

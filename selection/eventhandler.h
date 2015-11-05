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

namespace slc
{
class EventHandler : public osgGA::GUIEventHandler
{
public:
    EventHandler();
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
    void setGeometryColor(osg::Geometry *geometryIn, const osg::Vec4 &colorIn);
    bool buildPreSelection(Container &container,
                           const osgUtil::LineSegmentIntersector::Intersections::const_iterator &intersection);
    Container lastPrehighlight;
    osg::Vec4 preHighlightColor;
    osg::Vec4 selectionColor;
    Containers selectionContainers;
    osg::Geode* buildTempPoint(const osg::Vec3d &pointIn);

    unsigned int nodeMask;
    unsigned int selectionMask;

    osgUtil::LineSegmentIntersector::Intersections currentIntersections;
    
    MessageOutSignal messageOutSignal;
    msg::MessageDispatcher dispatcher;
    void setupDispatcher();
    void requestPreselectionAdditionDispatched(const msg::Message &);
    void requestPreselectionSubtractionDispatched(const msg::Message &);
    void requestSelectionAdditionDispatched(const msg::Message &);
    void requestSelectionSubtractionDispatched(const msg::Message &);
    void requestSelectionClearDispatched(const msg::Message &);
    slc::Container buildContainer(const msg::Message &);
};

class getGeometryFromIds : public osg::NodeVisitor
{
public:
    getGeometryFromIds(const std::vector<boost::uuids::uuid> &idsIn, std::vector<osg::Geometry *> &geometryIn);
    virtual void apply(osg::Geode &aGeode);
protected:
    const std::vector<boost::uuids::uuid> &ids;
    std::vector<osg::Geometry *> &geometry;
};
}

#endif // SELECTIONEVENTHANDLER_H

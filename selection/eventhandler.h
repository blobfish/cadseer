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

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/signals2.hpp>

#include <osgGA/GUIEventHandler>
#include <osgUtil/LineSegmentIntersector>

#include <selection/definitions.h>

namespace Selection
{
class Message;

class Selected
{
public:
    Selected(){}
    void initialize(osg::Geometry *geometryIn);
    osg::observer_ptr<osg::Geometry> geometry;
    osg::Vec4 color;
};

class Container
{
public:
    Container(){}
    Type selectionType = Type::None;
    std::vector<Selected> selections;
    boost::uuids::uuid featureId = boost::uuids::nil_generator()();
    boost::uuids::uuid shapeId = boost::uuids::nil_generator()();
    osg::Vec3d pointLocation;
};
inline bool operator==(const Container& lhs, const Container& rhs)
{
  return
  (
    (lhs.selectionType == rhs.selectionType) &&
    (lhs.featureId == rhs.featureId) &&
    (lhs.shapeId == rhs.shapeId) &&
    (lhs.pointLocation == rhs.pointLocation)
  );
}
std::ostream& operator<<(std::ostream& os, const Container& container);

typedef std::vector<Container> Containers;
std::ostream& operator<<(std::ostream& os, const Containers& containers);

class EventHandler : public osgGA::GUIEventHandler
{
public:
    EventHandler();
    const Containers& getSelections() const {return selectionContainers;}
    void clearSelections();
    void setSelectionMask(const unsigned int &maskIn);
    
    typedef boost::signals2::signal<void (const Message &)> SelectionChangedSignal;
    boost::signals2::connection connectSelectionChanged(const SelectionChangedSignal::slot_type &subscriber)
    {
      return selectionChangedSignal.connect(subscriber);
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
    
    SelectionChangedSignal selectionChangedSignal;
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

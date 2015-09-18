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

#include <osgGA/GUIEventHandler>
#include <osgUtil/LineSegmentIntersector>

#include "selectiondefs.h"

class Selected
{
public:
    Selected(){}
    void initialize(osg::Geometry *geometryIn);
    osg::observer_ptr<osg::Geometry> geometry;
    osg::Vec4 color;
};

class SelectionContainer
{
public:
    SelectionContainer(){}
    SelectionTypes::Type selectionType;
    std::vector<Selected> selections;
    boost::uuids::uuid featureId;
    boost::uuids::uuid shapeId;
};
inline bool operator==(const SelectionContainer& lhs, const SelectionContainer& rhs)
{
  return ((lhs.featureId == rhs.featureId) && (lhs.shapeId == rhs.shapeId));
}
std::ostream& operator<<(std::ostream& os, const SelectionContainer& container);

typedef std::vector<SelectionContainer> SelectionContainers;
std::ostream& operator<<(std::ostream& os, const SelectionContainers& containers);

class SelectionEventHandler : public osgGA::GUIEventHandler
{
public:
    SelectionEventHandler();
    const SelectionContainers& getSelections() const {return selectionContainers;}
    void clearSelections();
    void setSelectionMask(const unsigned int &maskIn);
protected:
    virtual bool handle(const osgGA::GUIEventAdapter& eventAdapter,
                        osgGA::GUIActionAdapter& actionAdapter, osg::Object *object,
                        osg::NodeVisitor *nodeVistor);
    void setPrehighlight(SelectionContainer &selected);
    void clearPrehighlight();
    bool alreadySelected(const SelectionContainer &testContainer);
    void setGeometryColor(osg::Geometry *geometryIn, const osg::Vec4 &colorIn);
    bool buildPreSelection(SelectionContainer &container,
                           const osgUtil::LineSegmentIntersector::Intersections::const_iterator &intersection);
    SelectionContainer lastPrehighlight;
    osg::Vec4 preHighlightColor;
    osg::Vec4 selectionColor;
    SelectionContainers selectionContainers;

    unsigned int nodeMask;
    unsigned int selectionMask;

    osgUtil::LineSegmentIntersector::Intersections currentIntersections;
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

#endif // SELECTIONEVENTHANDLER_H

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
    boost::uuids::uuid id;
};
inline bool operator==(const SelectionContainer& lhs, const SelectionContainer& rhs){return (lhs.id == rhs.id);}

typedef std::vector<SelectionContainer> SelectionContainers;

class SelectionEventHandler : public osgGA::GUIEventHandler
{
public:
    SelectionEventHandler();
    SelectionContainers getSelections(){return selectionContainers;}
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

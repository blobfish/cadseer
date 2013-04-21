#include <iostream>

#include <osgUtil/LineSegmentIntersector>
#include <osg/Geometry>
#include <osg/View>

#include "selectioneventhandler.h"
#include "nodemaskdefs.h"
#include "selectionintersector.h"

using namespace osg;

SelectionEventHandler::SelectionEventHandler() : osgGA::GUIEventHandler()
{
    preHighlightColor = Vec4(1.0, 1.0, 0.0, 1.0);
}

bool SelectionEventHandler::handle(const osgGA::GUIEventAdapter& eventAdapter,
                    osgGA::GUIActionAdapter& actionAdapter, osg::Object *object,
                                   osg::NodeVisitor * nodeVistor)
{
    if (eventAdapter.getEventType() == osgGA::GUIEventAdapter::MOVE)
    {
        if (lastPrehighlightGeometry.valid())
        {
           Vec4Array *colors = dynamic_cast<Vec4Array*>(lastPrehighlightGeometry->getColorArray());
           if (colors && colors->size() > 0)
           {
               colors->front() = lastPrehighlightColor;
               colors->dirty();
               lastPrehighlightGeometry->dirtyDisplayList();
           }
           lastPrehighlightGeometry = NULL;
        }

        osg::View* view = actionAdapter.asView();
        if (!view)
            return false;

        SelectionIntersector* picker = new SelectionIntersector(
                    osgUtil::Intersector::WINDOW, eventAdapter.getX(),
                    eventAdapter.getY());
        picker->setPickRadius(16.0); //32 x 32 cursor

        osgUtil::IntersectionVisitor iv(picker);
        iv.setTraversalMask(~NodeMask::noSelect);
        view->getCamera()->accept(iv);
        if (picker->containsIntersections())
        {
//            std::cout << picker->getIntersections().size() << " objects" << std::endl;
            osgUtil::LineSegmentIntersector::Intersection intersection = picker->getFirstIntersection();
//            osg::Vec3d point = intersection.localIntersectionPoint;
//            std::cout << "Picked " << point.x() << "   " << point.y() << "   " << point.z() << std::endl;

            ref_ptr<Geometry> geometry = dynamic_pointer_cast<Geometry>(intersection.drawable);
            if (!geometry.valid())
                return false;

            lastPrehighlightGeometry = geometry;
            Vec4Array *colors = dynamic_cast<Vec4Array*>(lastPrehighlightGeometry->getColorArray());
            if (colors && colors->size() > 0)
            {
                lastPrehighlightColor = colors->front();
                colors->front() = preHighlightColor;
                colors->dirty();
                lastPrehighlightGeometry->dirtyDisplayList();
            }
        }
    }

    return false;
}

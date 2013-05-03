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
           osgUtil::LineSegmentIntersector::Intersections intersections = picker->getIntersections();
           osgUtil::LineSegmentIntersector::Intersections::const_iterator itIt;

           //don't change from something selected if it is already selected.
           for (itIt = intersections.begin(); itIt != intersections.end(); ++itIt)
           {
               ref_ptr<Geometry> geometry = dynamic_pointer_cast<Geometry>(itIt->drawable);
               if (!geometry.valid())
               {
                   clearPrehighlight();
                   return false;
               }

               if (geometry.get() == lastPrehighlightGeometry.get())
                   return false;
           }

           osgUtil::LineSegmentIntersector::Intersection intersection = picker->getFirstIntersection();
           ref_ptr<Geometry> geometry = dynamic_pointer_cast<Geometry>(intersection.drawable);
           if (!geometry.valid())
           {
               clearPrehighlight();
               return false;
           }

           clearPrehighlight();
           setPrehighlight(geometry.get());
        }
        else
            clearPrehighlight();
    }

    return false;
}
void SelectionEventHandler::setPrehighlight(osg::Geometry *geometry)
{
    lastPrehighlightGeometry = geometry;
    Vec4Array *colors = dynamic_cast<Vec4Array*>(lastPrehighlightGeometry->getColorArray());
    if (colors && colors->size() > 0)
    {
        lastPrehighlightColorIndex = 0;
        lastPrehighlightColor = (*colors)[lastPrehighlightColorIndex];
        (*colors)[lastPrehighlightColorIndex] = preHighlightColor;
        colors->dirty();
        lastPrehighlightGeometry->dirtyDisplayList();
    }
}

void SelectionEventHandler::clearPrehighlight()
{
    if (!lastPrehighlightGeometry.valid())
        return;
    Vec4Array *colors = dynamic_cast<Vec4Array*>(lastPrehighlightGeometry->getColorArray());
    if (colors && colors->size() > 0)
    {
        (*colors)[lastPrehighlightColorIndex] = lastPrehighlightColor;
        colors->dirty();
        lastPrehighlightGeometry->dirtyDisplayList();
    }
    lastPrehighlightGeometry = NULL;
}

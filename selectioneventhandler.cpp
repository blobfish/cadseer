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
    selectionColor = Vec4(1.0, 1.0, 1.0, 1.0);
    nodeMask = ~NodeMask::noSelect;
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
        iv.setTraversalMask(nodeMask);
        view->getCamera()->accept(iv);
        if (picker->containsIntersections())
        {
           osgUtil::LineSegmentIntersector::Intersections intersections = picker->getIntersections();
           osgUtil::LineSegmentIntersector::Intersections::const_iterator itIt;

           ref_ptr<Geometry> selectedGeometry;
           for (itIt = intersections.begin(); itIt != intersections.end(); ++itIt)
           {
               ref_ptr<Geometry> geometry = dynamic_pointer_cast<Geometry>(itIt->drawable);
               if (!geometry.valid())
                   continue;

               if (alreadySelected(geometry.get()))
                   continue;

               if (geometry.get() == lastPrehighlightGeometry.get())
                   return false;

               selectedGeometry = geometry;
               break;
           }

           if (!selectedGeometry.valid())
           {
               clearPrehighlight();
               return false;
           }

           clearPrehighlight();
           setPrehighlight(selectedGeometry.get());
        }
        else
            clearPrehighlight();
    }

    if (eventAdapter.getButtonMask() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON &&
            eventAdapter.getEventType() == osgGA::GUIEventAdapter::PUSH)
    {
        if (lastPrehighlightGeometry.valid())
        {
            Selected selected;
            selected.geometry = lastPrehighlightGeometry;
            selected.color = lastPrehighlightColor;
            selected.colorIndex = lastPrehighlightColorIndex;
            selections.push_back(selected);

            Vec4Array *colors = dynamic_cast<Vec4Array*>(selected.geometry->getColorArray());
            if (colors && colors->size() > 0)
            {
                (*colors)[selected.colorIndex] = selectionColor;
                colors->dirty();
                selected.geometry->dirtyDisplayList();
            }
            lastPrehighlightGeometry = NULL;
        }
        else
        {
            std::vector<Selected>::iterator it;
            for (it = selections.begin(); it != selections.end(); ++it)
            {
                if (!(it->geometry.valid()))
                    continue;
                Vec4Array *colors = dynamic_cast<Vec4Array*>(it->geometry->getColorArray());
                if (colors && colors->size() > 0)
                {
                    (*colors)[it->colorIndex] = it->color;
                    colors->dirty();
                    it->geometry->dirtyDisplayList();
                }
            }
            selections.clear();
        }
    }

    return false;
}

bool SelectionEventHandler::alreadySelected(osg::Geometry *geometry)
{
    std::vector<Selected>::iterator it;
    for (it = selections.begin(); it != selections.end(); ++it)
    {
        if (geometry == it->geometry.get())
            return true;
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

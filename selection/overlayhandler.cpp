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

#include <iostream>
#include <assert.h>
#include <memory>

#include <osgViewer/View>

#include <viewer/overlaycamera.h>
#include <library/csysdragger.h>
#include <library/ipgroup.h>
#include <library/plabel.h>
#include <feature/parameter.h>
#include <dialogs/parameterdialog.h>
#include <selection/visitors.h>
#include <modelviz/nodemaskdefs.h>
#include <globalutilities.h>
#include <command/manager.h>
#include <command/csysedit.h>
#include "overlayhandler.h"

using namespace slc;

OverlayHandler::OverlayHandler(vwr::OverlayCamera* cameraIn) : camera(cameraIn)
{

}

bool OverlayHandler::handle
(
  const osgGA::GUIEventAdapter& eventAdapter,
  osgGA::GUIActionAdapter& actionAdapter,
  osg::Object* /*object*/,
  osg::NodeVisitor* /*nodeVistor*/
)
{
  //control key 'shuts off' the overlay handler unless it is in
  //the middle of a drag.
  if
  (
    !isDrag
    && (eventAdapter.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_LEFT_CTRL)
  )
  {
    return false;
  }
  
  auto doIntersection = [&]() -> osgUtil::LineSegmentIntersector::Intersections
  {
    osgViewer::View *viewer = dynamic_cast<osgViewer::View *>(actionAdapter.asView());
    assert(viewer);
    
    osg::NodePath iPath; iPath.push_back(camera.get());
    osgUtil::LineSegmentIntersector::Intersections out;
    
    viewer->computeIntersections
    (
      camera.get(),
      osgUtil::Intersector::WINDOW,
      eventAdapter.getX(),
      eventAdapter.getY(),
      iPath,
      out
      //no mask at this time.
    );
    
    return out;
  };
  
  auto findDragger = [&](const osgUtil::LineSegmentIntersector::Intersections &intersections)
  {
    this->dragger = nullptr;
    
    for (auto i : intersections)
    {
      for (auto n: i.nodePath)
      {
        if
        (
            (!dynamic_cast<osgManipulator::Translate1DDragger *>(n)) &&
            (!dynamic_cast<lbr::RotateCircularDragger *>(n))
        )
            continue;

        this->dragger = dynamic_cast<osgManipulator::Dragger *>(n);
        if (this->dragger)
        {
            pointer.addIntersection(i.nodePath, i.getLocalIntersectPoint());
            pointer.setCamera(camera);
            pointer.setMousePosition(eventAdapter.getX(), eventAdapter.getY());
            
            path = i.nodePath;
        
            return;
        }
      }
    }
  };
  
  auto findIcon = [&](const osgUtil::LineSegmentIntersector::Intersections &intersections)
  {
    for (auto i : intersections)
    {
      for (auto n: i.nodePath)
      {
        if
        (
            (n->getName() == "LinkIcon") ||
            (n->getName() == "UnlinkIcon")
        )
        {
            path = i.nodePath;
            return;
        }
      }
    }
  };
  
  auto findDimension = [&](const osgUtil::LineSegmentIntersector::Intersections &intersections)
  {
    for (auto i : intersections)
    {
      for (auto n: i.nodePath)
      {
        dimension = dynamic_cast<lbr::IPGroup *>(n);
        if(dimension)
        {
            path = i.nodePath;
            return;
        }
      }
    }
  };
  
  auto findCSysOrigin = [&](const osgUtil::LineSegmentIntersector::Intersections &intersections)
  {
    for (auto i : intersections)
    {
      bool foundOrigin = false;
      bool foundDragger = false;
      for (auto n: i.nodePath)
      {
        if (dynamic_cast<lbr::CSysDragger *>(n))
            foundDragger = true;
        if (n->getName() == "origin")
            foundOrigin = true;
        if (foundDragger && foundOrigin)
        {
            path = i.nodePath;
            return;
        }
      }
    }
  };
  
  auto findPLabel = [&](const osgUtil::LineSegmentIntersector::Intersections &intersections)
  {
    for (const auto &i : intersections)
    {
      for (const auto &n : i.nodePath)
      {
        pLabel = dynamic_cast<lbr::PLabel *>(n);
        if (pLabel)
        {
            path = i.nodePath;
            return;
        }
      }
    }
  };
  
  bool out= false;
  bool shouldRedraw = false;
  
  //when we are over, overlay geometry we want to absorb the preselection.
  if (eventAdapter.getEventType() == osgGA::GUIEventAdapter::MOVE)
  {
    assert(!dragger);
    assert(!dimension);
    assert(!pLabel);
    assert(path.empty());
    osgUtil::LineSegmentIntersector::Intersections intersections = doIntersection();
    findDragger(intersections);
    findIcon(intersections);
    findDimension(intersections);
    findCSysOrigin(intersections);
    findPLabel(intersections);
    out = !path.empty();
    
    pointer.reset();
    dragger = nullptr;
    dimension.release();
    pLabel.release();
    path.clear();
  }
  
  if
  (
    (eventAdapter.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON) &&
    (eventAdapter.getEventType() == osgGA::GUIEventAdapter::PUSH)
  )
  {
    assert(!dragger);
    assert(path.empty());
    osgUtil::LineSegmentIntersector::Intersections intersections = doIntersection();
    findDragger(intersections);
    if (dragger)
    {
      if (dragger->handle(pointer, eventAdapter, actionAdapter))
      {
        dragger->setDraggerActive(true);
        out = true;
      }
    }
    else //no dragger
    {
      assert(path.empty());
      findIcon(intersections);
      if (!path.empty())
      {
        std::string nodeName;
        lbr::CSysDragger *csysDragger = nullptr;
        for (auto it = path.rbegin(); it != path.rend(); ++it)
        {
            if (nodeName.empty())
                nodeName = (*it)->getName();
            if (!csysDragger)
                csysDragger = dynamic_cast<lbr::CSysDragger*>(*it);
        }
        
        if (!nodeName.empty() && csysDragger)
        {
            if (nodeName == "LinkIcon")
                csysDragger->setUnlink();
            if (nodeName == "UnlinkIcon")
                csysDragger->setLink();
            shouldRedraw = true;
            path.clear(); //don't need to save path for icon.
        }
      }
    }
  }
  
  //drag is button independent
  if(eventAdapter.getEventType() == osgGA::GUIEventAdapter::DRAG)
  {
    if (dragger)
    {
      pointer.setMousePosition(eventAdapter.getX(), eventAdapter.getY());
      dragger->handle(pointer, eventAdapter, actionAdapter);
      isDrag = true;
      out = true;
    }
  }
  
  if
  (
    (eventAdapter.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON) &&
    (eventAdapter.getEventType() == osgGA::GUIEventAdapter::RELEASE)
  )
  {
    if (dragger)
    {
      //this leaves the dragger highlighted in a non drag(select) scenario.

      pointer.setMousePosition(eventAdapter.getX(), eventAdapter.getY());
      dragger->handle(pointer, eventAdapter, actionAdapter);
      dragger->setDraggerActive(false);
      
      if (!isDrag)
      {
        osgManipulator::Translate1DDragger *tDragger = dynamic_cast<osgManipulator::Translate1DDragger*>(dragger);
        if (tDragger)
        {
            //look for csysdragger.
            ParentMaskVisitor visitor(NodeMaskDef::csys);
            dragger->accept(visitor);
            if (visitor.out)
            {
                lbr::CSysDragger *csysDragger = dynamic_cast<lbr::CSysDragger*>(visitor.out);
                assert(csysDragger); //might not assert, just test and continue if ok.
                
                std::shared_ptr<cmd::CSysEdit> csysEdit(new cmd::CSysEdit());
                csysEdit->csysDragger = csysDragger;
                csysEdit->translateDragger = tDragger;
                csysEdit->type = cmd::CSysEdit::Type::Vector;
                cmd::manager().addCommand(csysEdit);
            }
        }
      }
      
      isDrag = false;
      out = true;
      pointer.reset();
      dragger = nullptr;
    }
    else
    {
      osgUtil::LineSegmentIntersector::Intersections intersections = doIntersection();
      findDimension(intersections);
      if (dimension)
      {
        ParentMaskVisitor visitor(NodeMaskDef::overlaySwitch);
        dimension->accept(visitor);
        assert(visitor.out);
        
        dlg::ParameterDialog *dialog = new dlg::ParameterDialog(dimension->getParameter(), gu::getId(visitor.out));
        dialog->show();
        dimension.release();
      }
      else
      {
        findCSysOrigin(intersections);
        if (!path.empty())
        {
            assert(path.back()->getName() == "origin");
            ParentMaskVisitor visitor(NodeMaskDef::csys);
            path.back()->accept(visitor);
            assert(visitor.out);
            lbr::CSysDragger *lDragger = dynamic_cast<lbr::CSysDragger*>(visitor.out);
            assert(lDragger);
            
            std::shared_ptr<cmd::CSysEdit> csysEdit(new cmd::CSysEdit());
            csysEdit->csysDragger = lDragger;
            csysEdit->type = cmd::CSysEdit::Type::Origin;
            cmd::manager().addCommand(csysEdit);
        }
        else
        {
            findPLabel(intersections);
            if (pLabel)
            {
                ParentMaskVisitor visitor(NodeMaskDef::overlaySwitch);
                pLabel->accept(visitor);
                assert(visitor.out);
                
                dlg::ParameterDialog *dialog = new dlg::ParameterDialog(pLabel->getParameter(), gu::getId(visitor.out));
                dialog->show();
                pLabel.release();
            }
        }
      }
    }
    path.clear();
  }

  if (shouldRedraw)
    actionAdapter.requestRedraw();
  return out;  //default to not handled.
}

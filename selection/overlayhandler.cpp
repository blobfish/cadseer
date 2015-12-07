/*
 * Copyright 2015 <copyright holder> <email>
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 */

#include <iostream>
#include <assert.h>

#include <osgViewer/View>

#include <viewer/overlaycamera.h>
#include <osg/csysdragger.h>
#include "overlayhandler.h"

using namespace slc;

OverlayHandler::OverlayHandler(OverlayCamera* cameraIn) : camera(cameraIn)
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
	  (!dynamic_cast<RotateCircularDragger *>(n))
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
  
  bool out= false;
  bool shouldRedraw = false;
  
  //when we are over, overlay geometry we want to absorb the preselection.
  if (eventAdapter.getEventType() == osgGA::GUIEventAdapter::MOVE)
  {
    assert(!dragger);
    assert(path.empty());
    osgUtil::LineSegmentIntersector::Intersections intersections = doIntersection();
    findDragger(intersections);
    findIcon(intersections);
    out = !path.empty();
    
    pointer.reset();
    dragger = nullptr;
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
	CSysDragger *csysDragger = nullptr;
	for (auto it = path.rbegin(); it != path.rend(); ++it)
	{
	  if (nodeName.empty())
	    nodeName = (*it)->getName();
	  if (!csysDragger)
	    csysDragger = dynamic_cast<CSysDragger*>(*it);
	}
	
	if (!nodeName.empty() && csysDragger)
	{
	  if (nodeName == "LinkIcon")
	    csysDragger->setUnlink();
	  if (nodeName == "UnlinkIcon")
	    csysDragger->setLink();
	  shouldRedraw = true;
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
      if (isDrag)
      {
	pointer.setMousePosition(eventAdapter.getX(), eventAdapter.getY());
	dragger->handle(pointer, eventAdapter, actionAdapter);
	dragger->setDraggerActive(false);
	isDrag = false;
      }
      out = true;
      pointer.reset();
      dragger = nullptr;
    }
    path.clear();
  }

  if (shouldRedraw)
    actionAdapter.requestRedraw();
  return out;  //default to not handled.
}

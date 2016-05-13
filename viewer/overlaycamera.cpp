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
#include <sstream>
#include <assert.h>

#include <osgViewer/GraphicsWindow>

#include <nodemaskdefs.h>
#include <feature/base.h>
#include <message/dispatch.h>
#include <message/observer.h>
#include <viewer/overlaycamera.h>

using namespace vwr;

OverlayCamera::OverlayCamera(osgViewer::GraphicsWindow *windowIn) : osg::Camera()
{
  observer = std::move(std::unique_ptr<msg::Observer>(new msg::Observer()));
  observer->name = "vwr::OverlayCamera";
  
  setNodeMask(NodeMaskDef::overlayCamera);
  setupDispatcher();
  setGraphicsContext(windowIn);
  setClearMask(GL_DEPTH_BUFFER_BIT);
  setRenderOrder(osg::Camera::NESTED_RENDER, 2);
  
  osg::Camera *mainCamera = nullptr;
  const osg::GraphicsContext::Cameras &cameras = windowIn->getCameras();
  for (const auto it : cameras)
  {
    if (it->getName() == "main")
    {
      mainCamera = it;
      break;
    }
  }
  assert(mainCamera); //couldn't find main camera
  setViewport(new osg::Viewport(*mainCamera->getViewport()));
}

void OverlayCamera::setupDispatcher()
{
  msg::Mask mask;

  mask = msg::Response | msg::Post | msg::AddFeature;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&OverlayCamera::featureAddedDispatched, this, _1)));
  
  mask = msg::Response | msg::Pre | msg::RemoveFeature;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&OverlayCamera::featureRemovedDispatched, this, _1)));
  
  mask = msg::Response | msg::Pre | msg::CloseProject;;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&OverlayCamera::closeProjectDispatched, this, _1)));
}

void OverlayCamera::featureAddedDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  prj::Message message = boost::get<prj::Message>(messageIn.payload);
  addChild(message.feature->getOverlaySwitch());
}

void OverlayCamera::featureRemovedDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  prj::Message message = boost::get<prj::Message>(messageIn.payload);
  removeChild(message.feature->getOverlaySwitch());
}

void OverlayCamera::closeProjectDispatched(const msg::Message&)
{
  //this code assumes that the first child is the absolute csys.
  assert(getChild(0)->getNodeMask() == NodeMaskDef::csys);
  removeChildren(1, getNumChildren() - 1);
}

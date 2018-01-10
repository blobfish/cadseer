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

#include <boost/variant.hpp>
#include <boost/filesystem.hpp>

#include <osgViewer/GraphicsWindow>

#include <application/application.h>
#include <project/project.h>
#include <globalutilities.h>
#include <tools/idtools.h>
#include <modelviz/nodemaskdefs.h>
#include <feature/base.h>
#include <message/dispatch.h>
#include <message/observer.h>
#include <viewer/message.h>
#include <selection/visitors.h>
#include <project/serial/xsdcxxoutput/view.h>
#include <viewer/overlay.h>

using namespace vwr;

Overlay::Overlay(osgViewer::GraphicsWindow *windowIn) : osg::Camera()
{
  observer = std::move(std::unique_ptr<msg::Observer>(new msg::Observer()));
  observer->name = "vwr::Overlay";
  
  fleetingGeometry = new osg::Switch();
  this->addChild(fleetingGeometry);
  
  setNodeMask(mdv::overlay);
  setName("overlay");
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
  
  this->getOrCreateStateSet()->setMode(GL_MULTISAMPLE_ARB, osg::StateAttribute::ON);
}

void Overlay::setupDispatcher()
{
  msg::Mask mask;

  mask = msg::Response | msg::Post | msg::Add | msg::Feature;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Overlay::featureAddedDispatched, this, _1)));
  
  mask = msg::Response | msg::Pre | msg::Remove | msg::Feature;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Overlay::featureRemovedDispatched, this, _1)));
  
  mask = msg::Response | msg::Pre | msg::Close | msg::Project;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Overlay::closeProjectDispatched, this, _1)));
  
  mask = msg::Request | msg::Add | msg::Overlay;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Overlay::addOverlayGeometryDispatched, this, _1)));
  
  mask = msg::Request | msg::Clear | msg::Overlay;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Overlay::clearOverlayGeometryDispatched, this, _1)));
  
  mask = msg::Request | msg::View | msg::Show | msg::Overlay;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Overlay::showOverlayDispatched, this, _1)));
  
  mask = msg::Request | msg::View | msg::Hide | msg::Overlay;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Overlay::hideOverlayDispatched, this, _1)));
  
  mask = msg::Request | msg::View | msg::Toggle | msg::Overlay;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Overlay::overlayToggleDispatched, this, _1)));
  
  mask = msg::Response | msg::Post | msg::Open | msg::Project;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Overlay::projectOpenedDispatched, this, _1)));
    
  mask = msg::Response | msg::Post | msg::Project | msg::Update | msg::Model;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Overlay::projectUpdatedDispatched, this, _1)));
}

void Overlay::featureAddedDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  prj::Message message = boost::get<prj::Message>(messageIn.payload);
  addChild(message.feature->getOverlaySwitch());
}

void Overlay::featureRemovedDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  prj::Message message = boost::get<prj::Message>(messageIn.payload);
  removeChild(message.feature->getOverlaySwitch());
}

void Overlay::closeProjectDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
    //this code assumes that the first child is the absolute csys switch.
    //also the fleeting switch.
    removeChildren(2, getNumChildren() - 2);
}

void Overlay::addOverlayGeometryDispatched(const msg::Message &message)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
    vwr::Message vMessage = boost::get<vwr::Message>(message.payload);
    fleetingGeometry->addChild(vMessage.node.get());
}

void Overlay::clearOverlayGeometryDispatched(const msg::Message&)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
    fleetingGeometry->removeChildren(0, fleetingGeometry->getNumChildren());
}

void Overlay::showOverlayDispatched(const msg::Message &msgIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  slc::MainSwitchVisitor v(boost::get<vwr::Message>(msgIn.payload).featureId);
  this->accept(v);
  assert(v.out);
  if (!v.out)
    return;
  
  if (v.out->getNewChildDefaultValue()) //already shown
    return;
  
  v.out->setAllChildrenOn();
  
  msg::Message mOut(msg::Response | msg::View | msg::Show | msg::Overlay);
  mOut.payload = msgIn.payload;
  observer->outBlocked(mOut);
}

void Overlay::hideOverlayDispatched(const msg::Message &msgIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  slc::MainSwitchVisitor v(boost::get<vwr::Message>(msgIn.payload).featureId);
  this->accept(v);
  assert(v.out);
  if (!v.out)
    return;
  
  if (!v.out->getNewChildDefaultValue()) //already hidden
    return;
  
  v.out->setAllChildrenOff();
  
  msg::Message mOut(msg::Response | msg::View | msg::Hide | msg::Overlay);
  mOut.payload = msgIn.payload;
  observer->outBlocked(mOut);
}

void Overlay::overlayToggleDispatched(const msg::Message &msgIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  slc::MainSwitchVisitor v(boost::get<vwr::Message>(msgIn.payload).featureId);
  this->accept(v);
  assert(v.out); //some features won't have overlay, but we want to filter those out before the message call.
  if (!v.out)
    return;
  
  msg::Mask maskOut; //notice we are not responding with Msg::Toggle that got us here.
  if (v.out->getNewChildDefaultValue())
  {
    v.out->setAllChildrenOff();
    maskOut = msg::Response | msg::View | msg::Hide | msg::Overlay;
  }
  else
  {
    v.out->setAllChildrenOn();
    maskOut = msg::Response | msg::View | msg::Show | msg::Overlay;
  }
  
  msg::Message mOut(maskOut);
  mOut.payload = msgIn.payload;
  observer->outBlocked(mOut);
}

void Overlay::projectOpenedDispatched(const msg::Message &)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  serialRead();
}

void Overlay::projectUpdatedDispatched(const msg::Message &)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  serialWrite();
}

//restore states from serialize
class SerialInOverlayVisitor : public osg::NodeVisitor
{
public:
  SerialInOverlayVisitor(const prj::srl::States &statesIn) :
  NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
  states(statesIn)
  {
    observer.name = "vwr::Overlay::SerialIn";
  }
  virtual void apply(osg::Switch &switchIn) override
  {
    std::string userValue;
    if (switchIn.getUserValue(gu::idAttributeTitle, userValue))
    {
      for (const auto &s : states.array())
      {
        if (userValue == s.id())
        {
          msg::Payload payload((vwr::Message(gu::stringToId(userValue))));
          if (s.visible())
          {
            switchIn.setAllChildrenOn();
            observer.outBlocked(msg::Message(msg::Mask(msg::Response | msg::View | msg::Show | msg::Overlay), payload));
          }
          else
          {
            switchIn.setAllChildrenOff();
            observer.outBlocked(msg::Message(msg::Mask(msg::Response | msg::View | msg::Hide | msg::Overlay), payload));
          }
          break;
        }
      }
    }
    
    //only interested in top level children, so don't need to call traverse here.
  }
protected:
  const prj::srl::States &states;
  msg::Observer observer;
};

void Overlay::serialRead()
{
  boost::filesystem::path file = static_cast<app::Application*>(qApp)->getProject()->getSaveDirectory();
  file /= "overlay.xml";
  if (!boost::filesystem::exists(file))
    return;
  
  auto sView = prj::srl::view(file.string(), ::xml_schema::Flags::dont_validate);
  SerialInOverlayVisitor v(sView->states());
  this->accept(v);
}

//get all states to serialize
class SerialOutVisitor : public osg::NodeVisitor
{
public:
  SerialOutVisitor(prj::srl::States &statesIn) : NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), states(statesIn){}
  virtual void apply(osg::Switch &switchIn) override
  {
    std::string userValue;
    if (switchIn.getUserValue(gu::idAttributeTitle, userValue))
      states.array().push_back(prj::srl::State(userValue, switchIn.getNewChildDefaultValue()));
    
    //only interested in top level children, so don't need to call traverse here.
  }
protected:
  prj::srl::States &states;
};

void Overlay::serialWrite()
{
  prj::srl::States states;
  SerialOutVisitor v(states);
  this->accept(v);
  prj::srl::View svOut(states);
  
  boost::filesystem::path file = static_cast<app::Application*>(qApp)->getProject()->getSaveDirectory();
  file /= "overlay.xml";
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(file.string());
  prj::srl::view(stream, svOut, infoMap);
}

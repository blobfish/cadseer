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

#ifndef TEXTCAMERA_H
#define TEXTCAMERA_H

#include <boost/uuid/uuid.hpp>

#include <osg/Camera>
#include <osg/Switch>
#include <osgText/Text>
#include <osg/observer_ptr>
#include <osgGA/GUIEventHandler>

#include <selection/message.h>
#include <message/message.h>

class ResizeEventHandler : public osgGA::GUIEventHandler
{
public:
  ResizeEventHandler(const osg::observer_ptr<osg::Camera> slaveCameraIn);
  void positionSelection();
  
protected:
  virtual bool handle(const osgGA::GUIEventAdapter& eventAdapter,
                      osgGA::GUIActionAdapter& actionAdapter, osg::Object *object,
                      osg::NodeVisitor *nodeVistor);
  
  osg::observer_ptr<osg::Camera> slaveCamera;
};

namespace osgViewer{class GraphicsWindow;}

class TextCamera : public osg::Camera
{
public:
    TextCamera(osgViewer::GraphicsWindow *);
    
    //new messaging system
    void messageInSlot(const msg::Message &);
    typedef boost::signals2::signal<void (const msg::Message &)> MessageOutSignal;
    boost::signals2::connection connectMessageOut(const MessageOutSignal::slot_type &subscriber)
    {
      return messageOutSignal.connect(subscriber);
    }
private:
  MessageOutSignal messageOutSignal; //new message system.
  msg::MessageDispatcher dispatcher;
  void setupDispatcher();
  void preselectionAdditionDispatched(const msg::Message &);
  void preselectionSubtractionDispatched(const msg::Message &);
  void selectionAdditionDispatched(const msg::Message &);
  void selectionSubtractionDispatched(const msg::Message &);
    
  osg::ref_ptr<osg::Switch> infoSwitch;
  //indexes for children
  static const unsigned int SelectionIndex = 0;
  
  osg::ref_ptr<osgText::Text> selectionLabel;
  void updateSelectionLabel();
  std::string preselectionText;
  
  typedef std::pair<boost::uuids::uuid, boost::uuids::uuid> SelectionMapKey;
  typedef std::map<SelectionMapKey, slc::Message> SelectionMap;
  SelectionMap selectionMap;
};

#endif // TEXTCAMERA_H

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

#ifndef OVERLAYHANDLER_H
#define OVERLAYHANDLER_H

#include <osgGA/GUIEventHandler>
#include <osgManipulator/Dragger> //needed for pointerInfo

class OverlayCamera;

namespace slc
{
  class OverlayHandler : public osgGA::GUIEventHandler
  {
  public:
    OverlayHandler(OverlayCamera *cameraIn);
  protected:
    virtual bool handle(const osgGA::GUIEventAdapter& eventAdapter,
		      osgGA::GUIActionAdapter& actionAdapter, osg::Object *object,
		      osg::NodeVisitor *nodeVistor) override;
    osg::ref_ptr<OverlayCamera> camera;
    osgManipulator::Dragger *dragger = nullptr;
    osgManipulator::PointerInfo pointer;
    osg::NodePath path;
    bool isDrag = false;
  };
}

#endif // OVERLAYHANDLER_H

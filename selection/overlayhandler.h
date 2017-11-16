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

#ifndef SLC_OVERLAYHANDLER_H
#define SLC_OVERLAYHANDLER_H

#include <osg/ref_ptr>
#include <osgGA/GUIEventHandler>
#include <osgManipulator/Dragger> //needed for pointerInfo

namespace vwr{class Overlay;}

namespace lbr {class IPGroup; class PLabel;}

namespace slc
{
  class OverlayHandler : public osgGA::GUIEventHandler
  {
  public:
    OverlayHandler(vwr::Overlay *cameraIn);
  protected:
    virtual bool handle(const osgGA::GUIEventAdapter& eventAdapter,
        osgGA::GUIActionAdapter& actionAdapter, osg::Object *object,
        osg::NodeVisitor *nodeVistor) override;
    osg::ref_ptr<vwr::Overlay> camera;
    osgManipulator::Dragger *dragger = nullptr;
    osgManipulator::PointerInfo pointer;
    osg::ref_ptr<lbr::IPGroup> dimension;
    osg::ref_ptr<lbr::PLabel> pLabel;
    
    osg::NodePath path;
    bool isDrag = false;
  };
}

#endif // SLC_OVERLAYHANDLER_H

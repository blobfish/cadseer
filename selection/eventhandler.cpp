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

#include <boost/variant.hpp>

#include <osg/Geometry>
#include <osg/View>
#include <osgViewer/View>
#include <osg/ValueObject>
#include <osg/Point>
#include <osg/Switch>
#include <osg/Depth>
#include <osgUtil/PolytopeIntersector>

#include <selection/eventhandler.h>
#include <modelviz/nodemaskdefs.h>
#include <selection/definitions.h>
#include <globalutilities.h>
#include <application/application.h>
#include <project/project.h>
#include <feature/base.h>
#include <annex/seershape.h>
#include <selection/message.h>
#include <message/dispatch.h>
#include <message/observer.h>
#include <selection/interpreter.h>
#include <selection/intersection.h>

using namespace osg;
using namespace boost::uuids;
using namespace slc;

//build polytope at origin. 8 sided.
static osg::Polytope buildBasePolytope(double radius)
{
  //apparently the order of the addition matters. intersector
  //wants opposites sequenced. This seems to be working.
  osg::Polytope out;
  osg::Matrixd rotation = osg::Matrixd::rotate(osg::DegreesToRadians(45.0), osg::Vec3d(0.0, 0.0, 1.0));
  osg::Vec3d base = osg::Vec3d(1.0, 0.0, 0.0);
  
  out.add(osg::Plane(base, radius));
  out.add(osg::Plane(-base, radius));
  
  base = rotation * base;
  
  out.add(osg::Plane(base, radius));
  out.add(osg::Plane(-base, radius));
  
  base = rotation * base;
  
  out.add(osg::Plane(base, radius));
  out.add(osg::Plane(-base, radius));
  
  base = rotation * base;
  
  out.add(osg::Plane(base, radius));
  out.add(osg::Plane(-base, radius));
  
  out.add(osg::Plane(0.0,0.0,1.0, 0.0)); //last has to be 'cap'
  
  return out;
}

static osg::Polytope buildPolytope(double x, double y, double radius)
{
  static osg::Polytope base = buildBasePolytope(radius);
  osg::Polytope out(base);
  out.transform(osg::Matrixd::translate(x, y, 0.0));
  
  return out;
}

EventHandler::EventHandler(osg::Group *viewerRootIn) : osgGA::GUIEventHandler()
{
    observer = std::move(std::unique_ptr<msg::Observer>(new msg::Observer()));
    observer->name = "slc::EventHandler";
    setupDispatcher();
  
    viewerRoot = viewerRootIn;
    
    preHighlightColor = Vec4(1.0, 1.0, 0.0, 1.0);
    selectionColor = Vec4(1.0, 1.0, 1.0, 1.0);
    nodeMask = ~(mdv::backGroundCamera | mdv::gestureCamera | mdv::csys | mdv::point);
    
    setSelectionMask(slc::AllEnabled);
}

void EventHandler::setSelectionMask(Mask maskIn)
{
    selectionMask = maskIn;

    if
    (
      canSelectWires(selectionMask) |
      canSelectFaces(selectionMask) |
      canSelectShells(selectionMask) |
      canSelectSolids(selectionMask) |
      canSelectFeatures(selectionMask) |
      canSelectObjects(selectionMask) |
      canSelectNearestPoints(selectionMask)
    )
        nodeMask |= mdv::face;
    else
        nodeMask &= ~mdv::face;

    if
    (
      canSelectEdges(selectionMask) |
      canSelectWires(selectionMask) |
      canSelectPoints(selectionMask)
    )
        nodeMask |= mdv::edge;
    else
        nodeMask &= ~mdv::edge;

    //obsolete. we no longer generate vertices
//     if ((Selection::pointsSelectable & selectionMask) == Selection::pointsSelectable)
//         nodeMask |= NodeMaskDef::vertex;
//     else
//         nodeMask &= ~NodeMaskDef::vertex;
}

bool EventHandler::handle(const osgGA::GUIEventAdapter& eventAdapter,
                    osgGA::GUIActionAdapter& actionAdapter, osg::Object*,
                                   osg::NodeVisitor*)
{
    if (eventAdapter.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_LEFT_CTRL)
    {
      clearPrehighlight();
      return false;
    }
  
    if(eventAdapter.getHandled())
    {
      clearPrehighlight();
      return true; //overlay has taken event;
    }
    
    //escape key should dispatch to cancel command.
    if (eventAdapter.getEventType() == osgGA::GUIEventAdapter::KEYUP)
    {
      if (eventAdapter.getKey() == osgGA::GUIEventAdapter::KEY_Escape)
      {
        observer->out(msg::Message(msg::Request | msg::Command | msg::Cancel));
        return true;
      }
    }
  
    if (eventAdapter.getEventType() == osgGA::GUIEventAdapter::MOVE)
    {
        osg::View* view = actionAdapter.asView();
        if (!view)
            return false;
        
        Intersections currentIntersections;
        
        //use poly on just the edges for now.
        if (nodeMask & mdv::edge)
        {
          osg::ref_ptr<osgUtil::PolytopeIntersector> polyPicker = new osgUtil::PolytopeIntersector
          (
            osgUtil::Intersector::WINDOW,
            buildPolytope(eventAdapter.getX(), eventAdapter.getY(), 16.0)
  //           eventAdapter.getX() - 16.0,
  //           eventAdapter.getY() - 16.0,
  //           eventAdapter.getX() + 16.0,
  //           eventAdapter.getY() + 16.0
          );
          osgUtil::IntersectionVisitor polyVisitor(polyPicker.get());
          polyVisitor.setTraversalMask(nodeMask & ~mdv::face);
          view->getCamera()->accept(polyVisitor);
          append(currentIntersections, polyPicker->getIntersections());
        }
        
        //use linesegment on just the faces for now.
        if (nodeMask & mdv::face)
        {
          osg::ref_ptr<osgUtil::LineSegmentIntersector> linePicker = new osgUtil::LineSegmentIntersector
          (
            osgUtil::Intersector::WINDOW,
            eventAdapter.getX(),
            eventAdapter.getY()
          );
          osgUtil::IntersectionVisitor lineVisitor(linePicker.get());
          lineVisitor.setTraversalMask(nodeMask & ~mdv::edge);
  //         lineVisitor.setUseKdTreeWhenAvailable(false); //temp for testing.
          view->getCamera()->accept(lineVisitor);
          append(currentIntersections, linePicker->getIntersections());
        }

        if (!currentIntersections.empty())
        {
            Interpreter interpreter(currentIntersections, selectionMask);

            slc::Container newContainer;
            //loop to get first non selected geometry.
            for (const auto& container : interpreter.containersOut)
            {
                if(alreadySelected(container))
                    continue;
                newContainer = container;
                break;
            }

            if (newContainer == lastPrehighlight)
            {
                //update the point location though.
                lastPrehighlight.pointLocation = newContainer.pointLocation;
                return false;
            }

           clearPrehighlight();

            if (newContainer.selectionType == slc::Type::None)
                return false;
           
            //this is here so we make sure we get through all selection
            //conditions before we construct point geometry.
            if (isPointType(newContainer.selectionType))
            {
                ref_ptr<Geometry> pointGeometry(buildTempPoint(newContainer.pointLocation));
                newContainer.pointGeometry = pointGeometry.get();
                viewerRoot->addChild(pointGeometry.get());
            }
            setPrehighlight(newContainer);
        }
        else
            clearPrehighlight();
    }

    if (eventAdapter.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON &&
            eventAdapter.getEventType() == osgGA::GUIEventAdapter::RELEASE)
    {
        if(lastPrehighlight.selectionType != slc::Type::None)
        {
            //prehighlight gets 'moved' into selections so can't call
            //clear prehighlight, but we still clear the prehighlight
            //selections we need to make observers aware of this 'hidden' change.
            msg::Message clearMessage(msg::Response | msg::Pre | msg::Preselection | msg::Remove);
            clearMessage.payload = containerToMessage(lastPrehighlight);
            observer->out(clearMessage);
            
            if (slc::isPointType(lastPrehighlight.selectionType))
            {
                assert(lastPrehighlight.pointGeometry.valid());
                osg::Vec4Array *colors = dynamic_cast<osg::Vec4Array*>(lastPrehighlight.pointGeometry->getColorArray());
                assert(colors);
                (*colors)[0] = osg::Vec4(1.0, 1.0, 1.0, 1.0);
                colors->dirty();
                lastPrehighlight.pointGeometry->dirtyDisplayList();
            }
            else
                selectionOperation(lastPrehighlight.featureId, lastPrehighlight.selectionIds,
                    HighlightVisitor::Operation::Highlight);
            
            add(selectionContainers, lastPrehighlight);
            
            msg::Message addMessage(msg::Response | msg::Post | msg::Selection | msg::Add);
            addMessage.payload = containerToMessage(lastPrehighlight);
            lastPrehighlight = Container(); //set to null before signal in case we end up in 'this' again.
            observer->out(addMessage);
        }
        //not clearing the selection anymore on a empty pick.
//         else
//             clearSelections();
    }
    
    if (eventAdapter.getButton() == osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON &&
            eventAdapter.getEventType() == osgGA::GUIEventAdapter::RELEASE)
    {
      clearSelections();
    }

    if (eventAdapter.getButton() == osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON &&
            eventAdapter.getEventType() == osgGA::GUIEventAdapter::PUSH)
    {
        clearPrehighlight();
    }

    if (eventAdapter.getEventType() == osgGA::GUIEventAdapter::DRAG)
    {
        //don't get button info here, need to cache.
    }

    return false;
}

void EventHandler::clearSelections()
{
    // clear in reverse order to fix wire issue were edges remained highlighted. edge was remembering already selected color.
    // something else will have to been when we get into delselection. maybe pool of selection and indexes for container?
    Containers::reverse_iterator it;
    for (it = selectionContainers.rbegin(); it != selectionContainers.rend(); ++it)
    {
        msg::Message removeMessage(msg::Response | msg::Pre | msg::Selection | msg::Remove);
        removeMessage.payload = containerToMessage(*it);
        observer->out(removeMessage);
        
        if (slc::isPointType(it->selectionType))
        {
            assert(it->pointGeometry.valid());
            osg::Group *parent = it->pointGeometry->getParent(0)->asGroup();
            parent->removeChild(it->pointGeometry);
        }
        else
            selectionOperation(it->featureId, it->selectionIds, HighlightVisitor::Operation::Restore);
    }
    selectionContainers.clear();
}

bool EventHandler::alreadySelected(const slc::Container &testContainer)
{
    Containers::const_iterator it;
    for (it = selectionContainers.begin(); it != selectionContainers.end(); ++it)
    {
        if ((*it) == testContainer)
            return true;
    }
    return false;
}

void EventHandler::setPrehighlight(slc::Container &selected)
{
    lastPrehighlight = selected;
    if (slc::isPointType(selected.selectionType))
    {
      assert(selected.pointGeometry.valid());
      osg::Vec4Array *colors = dynamic_cast<osg::Vec4Array*>(selected.pointGeometry->getColorArray());
      assert(colors);
      colors->at(0) = osg::Vec4(1.0, 1.0, 0.0, 1.0);
      colors->dirty();
    }
    else
      selectionOperation(selected.featureId, selected.selectionIds, HighlightVisitor::Operation::PreHighlight);
    
    msg::Message addMessage(msg::Response | msg::Post | msg::Preselection | msg::Add);
    addMessage.payload = containerToMessage(lastPrehighlight);
    observer->out(addMessage);
}

void EventHandler::clearPrehighlight()
{
    if (lastPrehighlight.selectionType == Type::None)
      return;
    
    msg::Message removeMessage(msg::Response | msg::Pre | msg::Preselection | msg::Remove);
    removeMessage.payload = containerToMessage(lastPrehighlight);
    observer->out(removeMessage);
    
    if (slc::isPointType(lastPrehighlight.selectionType))
    {
      assert(lastPrehighlight.pointGeometry.valid());
      osg::Group *parent = lastPrehighlight.pointGeometry->getParent(0)->asGroup();
      parent->removeChild(lastPrehighlight.pointGeometry);
    }
    else
    {
      selectionOperation(lastPrehighlight.featureId, lastPrehighlight.selectionIds, HighlightVisitor::Operation::Restore);
      //certain situations, like selecting wires, where prehighlight can over write something already selected. Then
      //when the prehighlight gets cleared and the color restored the selection is lost. for now lets just re select
      //everything and do something different if we have performance problems.
      for (const auto &current : selectionContainers)
        selectionOperation(current.featureId, current.selectionIds, HighlightVisitor::Operation::Highlight);
    }
    
    lastPrehighlight = Container();
}

void EventHandler::setupDispatcher()
{
  msg::Mask mask;
  
  mask = msg::Request | msg::Preselection | msg::Add;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&EventHandler::requestPreselectionAdditionDispatched, this, _1)));
  
  mask = msg::Request | msg::Preselection | msg::Remove;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&EventHandler::requestPreselectionSubtractionDispatched, this, _1)));
  
  mask = msg::Request | msg::Selection | msg::Add;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&EventHandler::requestSelectionAdditionDispatched, this, _1)));
  
  mask = msg::Request | msg::Selection | msg::Remove;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&EventHandler::requestSelectionSubtractionDispatched, this, _1)));

  mask = msg::Request | msg::Selection | msg::Clear;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&EventHandler::requestSelectionClearDispatched, this, _1)));
  
  mask = msg::Response | msg::Selection | msg::SetMask;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&EventHandler::selectionMaskDispatched, this, _1)));
}

void EventHandler::selectionOperation
(
  const uuid &featureIdIn,
  const std::vector<uuid> &idsIn,
  HighlightVisitor::Operation operationIn
)
{
  MainSwitchVisitor sVisitor(featureIdIn);
  viewerRoot->accept(sVisitor);
  if (!sVisitor.out)
    return;
  HighlightVisitor hVisitor(idsIn, operationIn);
  sVisitor.out->accept(hVisitor);
}

void EventHandler::requestPreselectionAdditionDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  slc::Message sMessage = boost::get<slc::Message>(messageIn.payload);
  slc::Container container = messageToContainer(sMessage);
  if
  (
    (container == lastPrehighlight) ||
    (alreadySelected(container))
  )
    return;
  
  //setPrehighlight handles points
  setPrehighlight(container);
}

void EventHandler::requestPreselectionSubtractionDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  slc::Message sMessage = boost::get<slc::Message>(messageIn.payload);
  
  slc::Container container = messageToContainer(sMessage);
  assert(container == lastPrehighlight);
  
  //clearPrehighlight handles points
  clearPrehighlight();
}

void EventHandler::requestSelectionAdditionDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  clearPrehighlight();

  slc::Message sMessage = boost::get<slc::Message>(messageIn.payload);
  slc::Container container = messageToContainer(sMessage);
  if (alreadySelected(container))
    return;
  
  if (slc::isPointType(container.selectionType))
  {
    assert(container.pointGeometry.valid());
    osg::Vec4Array *colors = dynamic_cast<osg::Vec4Array*>(container.pointGeometry->getColorArray());
    assert(colors);
    (*colors)[0] = osg::Vec4(1.0, 1.0, 1.0, 1.0);
    colors->dirty();
    container.pointGeometry->dirtyDisplayList();
    
    viewerRoot->addChild(container.pointGeometry.get());
  }
  else
    selectionOperation(sMessage.featureId, container.selectionIds, HighlightVisitor::Operation::Highlight);
  
  add(selectionContainers, container);
  
  msg::Message messageOut = messageIn;
  messageOut.mask &= ~msg::Request;
  messageOut.mask |= msg::Response | msg::Post;
  observer->out(messageOut);
}

void EventHandler::requestSelectionSubtractionDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  slc::Message sMessage = boost::get<slc::Message>(messageIn.payload);
  slc::Container container = messageToContainer(sMessage);
  
  Containers::iterator containIt = std::find(selectionContainers.begin(), selectionContainers.end(), container);
  assert(containIt != selectionContainers.end());
  if (containIt == selectionContainers.end())
    return;
  
  msg::Message messageOut = messageIn;
  messageOut.mask &= ~msg::Request;
  messageOut.mask |= msg::Response | msg::Pre;
  observer->out(messageOut);
  
  if (slc::isPointType(containIt->selectionType))
  {
    assert(containIt->pointGeometry.valid());
    osg::Group *parent = containIt->pointGeometry->getParent(0)->asGroup();
    parent->removeChild(containIt->pointGeometry);
  }
    selectionOperation(sMessage.featureId, containIt->selectionIds, HighlightVisitor::Operation::Restore);
    
  selectionContainers.erase(containIt);
}

void EventHandler::requestSelectionClearDispatched(const msg::Message &)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  clearPrehighlight();
  clearSelections();
}

void EventHandler::selectionMaskDispatched(const msg::Message &messageIn)
{
  slc::Message sMsg = boost::get<slc::Message>(messageIn.payload);
  setSelectionMask(sMsg.selectionMask);
}

Geometry* EventHandler::buildTempPoint(const Vec3d& pointIn)
{
  osg::Geometry *geometry = new osg::Geometry();
  geometry->setNodeMask(mdv::point);
  geometry->setCullingActive(false);
  geometry->setDataVariance(osg::Object::DYNAMIC);
  osg::Vec3Array *vertices = new osg::Vec3Array();
  vertices->push_back(pointIn);
  geometry->setVertexArray(vertices);
  osg::Vec4Array *colors = new osg::Vec4Array();
  colors->push_back(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
  geometry->setColorArray(colors);
  geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
  geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, 1));
  
  geometry->getOrCreateStateSet()->setAttribute(new osg::Point(10.0));
  geometry->getOrCreateStateSet()->setAttribute(new osg::Depth(osg::Depth::LESS, 0.0, 1.0));
  geometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
  
  return geometry;
}

Message EventHandler::containerToMessage(const Container &containerIn)
{
  slc::Message out;
  out.type = containerIn.selectionType;
  out.featureId = containerIn.featureId;
  out.featureType = containerIn.featureType;
  out.shapeId = containerIn.shapeId;
  out.pointLocation = containerIn.pointLocation;
  
  return out;
}

Container EventHandler::messageToContainer(const Message &messageIn)
{
  //should incoming selection messages ignore selection mask?
  //I am thinking yes right now. Selection masks are only really
  //useful for cursor picking.
  
  assert(!messageIn.featureId.is_nil());
  const ftr::Base *feature = dynamic_cast<app::Application *>(qApp)->getProject()->findFeature(messageIn.featureId);
  assert(feature);
  
  slc::Container container;
  container.selectionType = messageIn.type;
  container.featureId = messageIn.featureId;
  container.featureType = messageIn.featureType;
  container.shapeId = messageIn.shapeId;
  container.pointLocation = messageIn.pointLocation;
  if (container.featureType == ftr::Type::Base)
    container.featureType = feature->getType();
  
  if (feature->hasAnnex(ann::Type::SeerShape) && !feature->getAnnex<ann::SeerShape>(ann::Type::SeerShape).isNull())
  {
    const ann::SeerShape &seerShape = feature->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
    if
    (
      (messageIn.type == slc::Type::Object) ||
      (messageIn.type == slc::Type::Solid) ||
      (messageIn.type == slc::Type::Shell)
    )
    {
      container.selectionIds = seerShape.useGetChildrenOfType(seerShape.getRootOCCTShape(), TopAbs_FACE);
    }
    //skip feature for now.
    else if
    (
      (messageIn.type == slc::Type::Face) ||
      (messageIn.type == slc::Type::Edge)
    )
    {
      container.selectionIds.push_back(container.shapeId);
    }
    else if (messageIn.type == slc::Type::Wire)
    {
      container.selectionIds = seerShape.useGetChildrenOfType(container.shapeId, TopAbs_EDGE);
    }
    else if (isPointType(messageIn.type))
    {
      container.pointGeometry = buildTempPoint(messageIn.pointLocation);
    }
  }
  
  return container;
}


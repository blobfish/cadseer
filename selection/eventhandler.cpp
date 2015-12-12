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

#include <boost/uuid/string_generator.hpp>

#include <osg/Geometry>
#include <osg/View>
#include <osgViewer/View>
#include <osg/ValueObject>
#include <osg/Point>
#include <osg/Switch>
#include <osg/Depth>

#include <selection/eventhandler.h>
#include <nodemaskdefs.h>
#include <selection/definitions.h>
#include <selection/intersector.h>
#include <globalutilities.h>
#include <application/application.h>
#include <project/project.h>
#include <modelviz/connector.h>
#include <modelviz/shapegeometry.h>
#include <selection/message.h>
#include <message/dispatch.h>

using namespace osg;
using namespace boost::uuids;
using namespace slc;

EventHandler::EventHandler(osg::Group *viewerRootIn) : osgGA::GUIEventHandler()
{
    viewerRoot = viewerRootIn;
    
    preHighlightColor = Vec4(1.0, 1.0, 0.0, 1.0);
    selectionColor = Vec4(1.0, 1.0, 1.0, 1.0);
    nodeMask = ~(NodeMaskDef::backGroundCamera | NodeMaskDef::gestureCamera | NodeMaskDef::csys | NodeMaskDef::point);
    setupDispatcher();
}

void EventHandler::setSelectionMask(const unsigned int &maskIn)
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
        nodeMask |= NodeMaskDef::face;
    else
        nodeMask &= ~NodeMaskDef::face;

    if
    (
      canSelectEdges(selectionMask) |
      canSelectWires(selectionMask) |
      canSelectPoints(selectionMask)
    )
        nodeMask |= NodeMaskDef::edge;
    else
        nodeMask &= ~NodeMaskDef::edge;

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
    if(eventAdapter.getHandled())
    {
      clearPrehighlight();
      return true; //overlay has taken event;
    }
  
    currentIntersections.clear();
    if (eventAdapter.getEventType() == osgGA::GUIEventAdapter::MOVE)
    {
        osg::View* view = actionAdapter.asView();
        if (!view)
            return false;

        osg::ref_ptr<Intersector>picker = new Intersector(
                    osgUtil::Intersector::WINDOW, eventAdapter.getX(),
                    eventAdapter.getY());
        picker->setPickRadius(16.0); //32 x 32 cursor

        osgUtil::IntersectionVisitor iv(picker.get());
        iv.setTraversalMask(nodeMask);
        view->getCamera()->accept(iv);
	
        if (picker->containsIntersections())
        {
           currentIntersections = picker->getMyIntersections();
           osgUtil::LineSegmentIntersector::Intersections::const_iterator itIt;

           slc::Container newContainer;
           for (itIt = currentIntersections.begin(); itIt != currentIntersections.end(); ++itIt)
           {
               slc::Container tempContainer;
               if (!buildPreSelection(tempContainer, itIt))
                   continue;

               if (alreadySelected(tempContainer))
		 continue;

               if (lastPrehighlight == tempContainer)
		 return false;

               newContainer = tempContainer;
	       
               break;
           }

           //wires were screwing up edges when changing same edge belonging to 2 different wires in
           //the same function call. so we make sure the current prehighlight is empty.
           if
	   (
	     (lastPrehighlight.selectionType == slc::Type::Wire) ||
	     (lastPrehighlight.selectionType == slc::Type::Edge)
	   )
           {
               clearPrehighlight();
               return false;
           }
           
           clearPrehighlight();
	   
	   if
	   (
	     (newContainer.selectionIds.empty()) &&
	     (!slc::isPointType(newContainer.selectionType))
	   )
	      return false;
           
	   //this is here so we make sure we get through all selection
	   //conditions before we construct point geometry.
           if (isPointType(newContainer.selectionType))
	   {
	     ref_ptr<Geometry> pointGeometry(buildTempPoint(newContainer.pointLocation));
	     newContainer.pointGeometry = pointGeometry.get();
	     
	     ParentMaskVisitor visitor(NodeMaskDef::object);
	     itIt->nodePath.back()->accept(visitor);
	     assert(visitor.out);
	     osg::Switch *tempSwitch = visitor.out->asSwitch();
	     tempSwitch->addChild(pointGeometry.get());
	   }
	   
	   setPrehighlight(newContainer);
        }
        else
            clearPrehighlight();
    }

    if (eventAdapter.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON &&
            eventAdapter.getEventType() == osgGA::GUIEventAdapter::RELEASE)
    {
        if
	(
	  (!lastPrehighlight.selectionIds.empty()) ||
	  (slc::isPointType(lastPrehighlight.selectionType))
	)
        {
	  //prehighlight gets 'moved' into selections so can't call
	  //clear prehighlight, but we still clear the prehighlight
	  //selections we need to make observers aware of this 'hidden' change.
	    msg::Message clearMessage;
	    clearMessage.mask = msg::Response | msg::Pre | msg::Preselection | msg::Subtraction;
	    slc::Message clearSMessage;
	    clearSMessage.type = lastPrehighlight.selectionType;
	    clearSMessage.featureId = lastPrehighlight.featureId;
	    clearSMessage.shapeId = lastPrehighlight.shapeId;
	    clearMessage.payload = clearSMessage;
	    messageOutSignal(clearMessage);
	    
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
	    
            selectionContainers.push_back(lastPrehighlight);
	    
	    msg::Message addMessage;
	    addMessage.mask = msg::Response | msg::Post | msg::Selection | msg::Addition;
	    slc::Message addSMessage;
	    addSMessage.type = lastPrehighlight.selectionType;
	    addSMessage.featureId = lastPrehighlight.featureId;
	    addSMessage.shapeId = lastPrehighlight.shapeId;
	    addSMessage.pointLocation = lastPrehighlight.pointLocation;
	    addMessage.payload = addSMessage;
	    messageOutSignal(addMessage);
	    
            lastPrehighlight = Container();
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

bool EventHandler::buildPreSelection(slc::Container &container,
                                              const osgUtil::LineSegmentIntersector::Intersections::const_iterator &intersection)
{
    auto walkUpToMask = [intersection](unsigned int maskIn) -> osg::Node*
    {
      auto nodePath = (*intersection).nodePath; //vector
      for (auto it = nodePath.rbegin(); it != nodePath.rend(); ++it)
      {
        if ((*it)->getNodeMask() == maskIn)
          return *it;
      }
      return nullptr;
    };
    
    auto processWire = [this, &container, &intersection](const uuid &featureId) -> bool
    {
      const ftr::Base *feature = dynamic_cast<app::Application *>(qApp)->getProject()->findFeature(featureId);
      assert(feature);
      const mdv::Connector &connector = feature->getConnector();
      
      osgUtil::LineSegmentIntersector::Intersections::const_iterator it = intersection;

      uuid faceId = boost::uuids::nil_generator()();
      uuid edgeId = boost::uuids::nil_generator()();
      osgUtil::LineSegmentIntersector::Intersections::const_iterator edgeIt = currentIntersections.end();
      for(; it != currentIntersections.end(); ++it)
      {
          mdv::ShapeGeometry *sGeometry = dynamic_cast<mdv::ShapeGeometry *>(it->nodePath.back());
	  if (!sGeometry)
	    continue;
          int currentNodeMask = it->nodePath.back()->getNodeMask();
          if (currentNodeMask == NodeMaskDef::face && faceId.is_nil())
              faceId = sGeometry->getId(it->primitiveIndex);
          if (currentNodeMask == NodeMaskDef::edge)
          {
              edgeId = sGeometry->getId(it->primitiveIndex);
              edgeIt = it;
          }
          if (faceId.is_nil() || edgeId.is_nil())
            continue;
          if (connector.useIsEdgeOfFace(edgeId, faceId))
            break;
      }
      
      if (it == currentIntersections.end())
        return false;

      uuid wireId = connector.useGetWire(edgeId, faceId);
      if (wireId.is_nil())
        return false;
      assert(edgeIt != currentIntersections.end());

      container.selectionIds = connector.useGetChildrenOfType(wireId, TopAbs_EDGE);
      container.selectionType = Type::Wire;
      container.shapeId = wireId;
      
      return true;
    };
    
    mdv::ShapeGeometry *shapeGeometry = dynamic_cast<mdv::ShapeGeometry *>((*intersection).drawable.get());
    if (!shapeGeometry)
        return false;

    int localNodeMask = (*intersection).nodePath.back()->getNodeMask();
    
    uuid selectedId = shapeGeometry->getId(intersection->primitiveIndex);
    osg::Node *featureRoot = walkUpToMask(NodeMaskDef::object);
    assert(featureRoot);
    uuid featureId = GU::getId(featureRoot);

//    std::cout << std::endl << "buildPreselection: " << std::endl << "   localNodeMask is: " << localNodeMask << std::endl <<
//                 "   selectedId is: " << selectedId << std::endl <<
//                 "   featureId is: " << featureId << std::endl;

    container.featureId = featureId;
    switch (localNodeMask)
    {
    case NodeMaskDef::face:
    {
        bool wireSignal = false;
        if (canSelectWires(selectionMask))
        {
          if (processWire(featureId))
            break;
        }
        if (canSelectFaces(selectionMask) && !wireSignal)
        {
          container.selectionType = Type::Face;
          container.shapeId = selectedId;
	  container.selectionIds.push_back(selectedId);
        }
        else if (canSelectShells(selectionMask))
        {
            const ftr::Base *feature = dynamic_cast<app::Application *>(qApp)->getProject()->findFeature(featureId);
            assert(feature);
            const mdv::Connector &connector = feature->getConnector();
            std::vector<uuid> shells = connector.useGetParentsOfType(selectedId, TopAbs_SHELL);
            //should be only 1 shell
            if (shells.size() == 1)
            {
                container.selectionIds = connector.useGetChildrenOfType(shells.at(0), TopAbs_FACE);
                container.selectionType = Type::Shell;
                container.shapeId = shells.at(0);
            }
        }
        else if (canSelectSolids(selectionMask))
        {
            const ftr::Base *feature = dynamic_cast<app::Application *>(qApp)->getProject()->findFeature(featureId);
            assert(feature);
            const mdv::Connector &connector = feature->getConnector();
            std::vector<uuid> solids = connector.useGetParentsOfType(selectedId, TopAbs_SOLID);
            //should be only 1 solid
            if (solids.size() == 1)
            {
                container.selectionIds = connector.useGetChildrenOfType(solids.at(0), TopAbs_FACE);
                container.selectionType = Type::Solid;
                container.shapeId = solids.at(0);
            }
        }
        else if (canSelectObjects(selectionMask))
        {
          const ftr::Base *feature = dynamic_cast<app::Application *>(qApp)->getProject()->findFeature(featureId);
          assert(feature);
          const mdv::Connector &connector = feature->getConnector();
          uuid object = connector.useGetRoot();
          if (!object.is_nil())
          {
            container.selectionIds = connector.useGetChildrenOfType(object, TopAbs_FACE);
            container.selectionType = Type::Object;
            container.shapeId = nil_generator()();
          }
        }
        break;
    }
    case NodeMaskDef::edge:
	if (canSelectPoints(selectionMask))
	{
	  const ftr::Base *feature = dynamic_cast<app::Application *>(qApp)->getProject()->findFeature(featureId);
          assert(feature);
          const mdv::Connector &connector = feature->getConnector();
	  osg::Vec3d iPoint = intersection->getWorldIntersectPoint();
	  osg::Vec3d snapPoint;
	  double distance = std::numeric_limits<double>::max();
	  slc::Type sType = slc::Type::None;
	  
	  auto updateSnaps = [&](const std::vector<osg::Vec3d> &vecIn) -> bool
	  {
	    bool out = false;
	    for (const auto& point : vecIn)
	    {
	      double tempDistance = (iPoint - point).length2();
	      if (tempDistance < distance)
	      {
		snapPoint = point;
		distance = tempDistance;
		out = true;
	      }
	    }
	    return out;
	  };
	  
	  if (canSelectEndPoints(selectionMask))
	  {
	    std::vector<osg::Vec3d> endPoints = connector.useGetEndPoints(selectedId);
	    if (updateSnaps(endPoints))
	      sType = slc::Type::EndPoint;
	  }
	  
	  if (canSelectMidPoints(selectionMask))
	  {
	    std::vector<osg::Vec3d> midPoints = connector.useGetMidPoint(selectedId);
	    if (updateSnaps(midPoints))
	      sType = slc::Type::MidPoint;
	  }
	  
	  if (canSelectCenterPoints(selectionMask))
	  {
	    std::vector<osg::Vec3d> centerPoints = connector.useGetCenterPoint(selectedId);
	    if (updateSnaps(centerPoints))
	      sType = slc::Type::CenterPoint;
	  }
	  
	  if (canSelectQuadrantPoints(selectionMask))
	  {
	    std::vector<osg::Vec3d> quadrantPoints = connector.useGetQuadrantPoints(selectedId);
	    if (updateSnaps(quadrantPoints))
	      sType = slc::Type::QuadrantPoint;
	  }
	  
	  if (canSelectNearestPoints(selectionMask))
	  {
	    std::vector<osg::Vec3d> nearestPoints = connector.useGetNearestPoint(selectedId, intersection->getWorldIntersectPoint());
	    if (updateSnaps(nearestPoints))
	      sType = slc::Type::NearestPoint;
	  }
	  container.selectionType = sType;
	  container.shapeId = selectedId;
	  container.pointLocation = snapPoint;
	}
        else if (canSelectEdges(selectionMask))
        {
            container.selectionType = Type::Edge;
            container.shapeId = selectedId;
	    container.selectionIds.push_back(selectedId);
        }
        else if (canSelectWires(selectionMask))
          processWire(featureId);
        break;
	//obsolete. we no longer generate vertices.
//     case NodeMaskDef::vertex:
//     {
//         Selected newSelection;
//         newSelection.initialize(geometry);
//         container.selectionType = Type::Vertex;
//         container.shapeId = selectedId;
//         container.selections.push_back(newSelection);
//         break;
//     }
    default:
        assert(0);
        break;
    }

    return true;
}

void EventHandler::clearSelections()
{
    // clear in reverse order to fix wire issue were edges remained highlighted. edge was remembering already selected color.
    // something else will have to been when we get into delselection. maybe pool of selection and indexes for container?
    Containers::reverse_iterator it;
    for (it = selectionContainers.rbegin(); it != selectionContainers.rend(); ++it)
    {
	msg::Message removeMessage;
	removeMessage.mask = msg::Response | msg::Pre | msg::Selection | msg::Subtraction;
	slc::Message removeSMessage;
	removeSMessage.type = it->selectionType;
	removeSMessage.featureId = it->featureId;
	removeSMessage.shapeId = it->shapeId;
	removeMessage.payload = removeSMessage;
	messageOutSignal(removeMessage);
      
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
    
    msg::Message addMessage;
    addMessage.mask = msg::Response | msg::Post | msg::Preselection | msg::Addition;
    slc::Message addSMessage;
    addSMessage.type = lastPrehighlight.selectionType;
    addSMessage.featureId = lastPrehighlight.featureId;
    addSMessage.shapeId = lastPrehighlight.shapeId;
    addSMessage.pointLocation = lastPrehighlight.pointLocation;
    addMessage.payload = addSMessage;
    messageOutSignal(addMessage);
}

void EventHandler::clearPrehighlight()
{
    if (lastPrehighlight.selectionType == Type::None)
      return;
    
    msg::Message removeMessage;
    removeMessage.mask = msg::Response | msg::Pre | msg::Preselection | msg::Subtraction;
    slc::Message removeSMessage;
    removeSMessage.type = lastPrehighlight.selectionType;
    removeSMessage.featureId = lastPrehighlight.featureId;
    removeSMessage.shapeId = lastPrehighlight.shapeId;
    removeMessage.payload = removeSMessage;
    messageOutSignal(removeMessage);
    
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
  
  mask = msg::Request | msg::Preselection | msg::Addition;
  dispatcher.insert(std::make_pair(mask, boost::bind(&EventHandler::requestPreselectionAdditionDispatched, this, _1)));
  
  mask = msg::Request | msg::Preselection | msg::Subtraction;
  dispatcher.insert(std::make_pair(mask, boost::bind(&EventHandler::requestPreselectionSubtractionDispatched, this, _1)));
  
  mask = msg::Request | msg::Selection | msg::Addition;
  dispatcher.insert(std::make_pair(mask, boost::bind(&EventHandler::requestSelectionAdditionDispatched, this, _1)));
  
  mask = msg::Request | msg::Selection | msg::Subtraction;
  dispatcher.insert(std::make_pair(mask, boost::bind(&EventHandler::requestSelectionSubtractionDispatched, this, _1)));

  mask = msg::Request | msg::Selection | msg::Clear;
  dispatcher.insert(std::make_pair(mask, boost::bind(&EventHandler::requestSelectionClearDispatched, this, _1)));
}

void EventHandler::messageInSlot(const msg::Message &messageIn)
{
  msg::MessageDispatcher::iterator it = dispatcher.find(messageIn.mask);
  if (it == dispatcher.end())
    return;
  
  it->second(messageIn);
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
  assert(!sMessage.featureId.is_nil());
  const ftr::Base *feature = dynamic_cast<app::Application *>(qApp)->getProject()->findFeature(sMessage.featureId);
  assert(feature);
  const mdv::Connector &connector = feature->getConnector();
  
  slc::Container container;
  container.selectionType = sMessage.type;
  container.featureId = sMessage.featureId;
  container.shapeId = sMessage.shapeId;
  if
  (
    (container == lastPrehighlight) ||
    (alreadySelected(container))
  )
    return;
  
  if (sMessage.type == slc::Type::Object)
  {
    container.selectionIds = connector.useGetChildrenOfType(connector.useGetRoot(), TopAbs_FACE);
    setPrehighlight(container);
  }
}

void EventHandler::requestPreselectionSubtractionDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  slc::Message sMessage = boost::get<slc::Message>(messageIn.payload);
  assert(!sMessage.featureId.is_nil());
  const ftr::Base *feature = dynamic_cast<app::Application *>(qApp)->getProject()->findFeature(sMessage.featureId);
  assert(feature);
  const mdv::Connector &connector = feature->getConnector();
  
  slc::Container container;
  container.selectionType = sMessage.type;
  container.featureId = sMessage.featureId;
  container.shapeId = sMessage.shapeId;
  assert(container == lastPrehighlight);
  
  if (sMessage.type == slc::Type::Object)
  {
    container.selectionIds = connector.useGetChildrenOfType(connector.useGetRoot(), TopAbs_FACE);
    clearPrehighlight();
  }
}

void EventHandler::requestSelectionAdditionDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  clearPrehighlight();

  slc::Message sMessage = boost::get<slc::Message>(messageIn.payload);
  assert(!sMessage.featureId.is_nil());
  const ftr::Base *feature = dynamic_cast<app::Application *>(qApp)->getProject()->findFeature(sMessage.featureId);
  assert(feature);
  const mdv::Connector &connector = feature->getConnector();
  
  slc::Container container;
  container.selectionType = sMessage.type;
  container.featureId = sMessage.featureId;
  container.shapeId = sMessage.shapeId;
  if (alreadySelected(container))
    return;
  
  if (sMessage.type == slc::Type::Object)
  {
    container.selectionIds = connector.useGetChildrenOfType(connector.useGetRoot(), TopAbs_FACE);
    selectionOperation(sMessage.featureId, container.selectionIds, HighlightVisitor::Operation::Highlight);
    
    selectionContainers.push_back(container);
    msg::Message messageOut = messageIn;
    messageOut.mask &= ~msg::Request;
    messageOut.mask |= msg::Response | msg::Post;
    messageOutSignal(messageOut);
  }
}

void EventHandler::requestSelectionSubtractionDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  slc::Message sMessage = boost::get<slc::Message>(messageIn.payload);
  assert(!sMessage.featureId.is_nil());
  const ftr::Base *feature = dynamic_cast<app::Application *>(qApp)->getProject()->findFeature(sMessage.featureId);
  assert(feature);
  const mdv::Connector &connector = feature->getConnector();
  
  slc::Container container;
  container.selectionType = sMessage.type;
  container.featureId = sMessage.featureId;
  container.shapeId = sMessage.shapeId;
  Containers::iterator containIt = std::find(selectionContainers.begin(), selectionContainers.end(), container);
  assert(containIt != selectionContainers.end());
  if (containIt == selectionContainers.end())
    return;
  
  if (sMessage.type == slc::Type::Object)
  {
    msg::Message messageOut = messageIn;
    messageOut.mask &= ~msg::Request;
    messageOut.mask |= msg::Response | msg::Pre;
    messageOutSignal(messageOut);
    
    std::vector<uuid> faces = connector.useGetChildrenOfType(connector.useGetRoot(), TopAbs_FACE);
    selectionOperation(sMessage.featureId, faces, HighlightVisitor::Operation::Restore);
    selectionContainers.erase(containIt);
  }
}

void EventHandler::requestSelectionClearDispatched(const msg::Message &)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  clearPrehighlight();
  clearSelections();
}

Geometry* EventHandler::buildTempPoint(const Vec3d& pointIn)
{
  osg::Geometry *geometry = new osg::Geometry();
  geometry->setNodeMask(NodeMaskDef::point);
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

MainSwitchVisitor::MainSwitchVisitor(const uuid& idIn):
  NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), id(idIn)
{

}

void MainSwitchVisitor::apply(Switch& switchIn)
{
  std::string userValue;
  if (switchIn.getUserValue(GU::idAttributeTitle, userValue))
  {
    boost::uuids::uuid switchId = boost::uuids::string_generator()(userValue);
    if (switchId == id)
    {
      out = &switchIn;
      return; //no need to continue search.
    }
  }

  traverse(switchIn);
}

ParentMaskVisitor::ParentMaskVisitor(std::size_t maskIn):
  NodeVisitor(osg::NodeVisitor::TRAVERSE_PARENTS), mask(maskIn)
{

}

void ParentMaskVisitor::apply(Node& nodeIn)
{
  if(nodeIn.getNodeMask() == mask)
  {
    out = &nodeIn;
    return; //no need to continue search.
  }
  traverse(nodeIn);
}

HighlightVisitor::HighlightVisitor(const std::vector< uuid >& idsIn, HighlightVisitor::Operation operationIn):
  NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), ids(idsIn), operation(operationIn)
{
  //possible run time speed up and compile time slowdown:
  //setup a std::funciton dispatcher here and eliminate if/then/else tree in apply.
}

void HighlightVisitor::apply(Geometry& geometryIn)
{
  mdv::ShapeGeometry *sGeometry = dynamic_cast<mdv::ShapeGeometry*>(&geometryIn);
  if (!sGeometry)
    return;
  assert (operation != Operation::None);
  if (operation == Operation::PreHighlight)
  {
    for (const auto &id : ids)
      sGeometry->setToPreHighlight(id);
  }
  else if(operation == Operation::Highlight)
  {
    for (const auto &id : ids)
      sGeometry->setToHighlight(id);
  }
  else //has to equal restore.
  {
    for (const auto &id : ids)
      sGeometry->setToColor(id);
  }
  
  //don't need to call traverse because geometry should be 'end of line'
}

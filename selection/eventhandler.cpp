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
#include <application.h>
#include <project/project.h>
#include <modelviz/connector.h>
#include <selection/message.h>
#include <message/dispatch.h>

using namespace osg;
using namespace boost::uuids;
using namespace Selection;

std::ostream& Selection::operator<<(std::ostream& os, const Container& container)
{
  os << 
    "type is: " << getNameOfType(container.selectionType) << 
    "      featureid is: " << GU::idToString(container.featureId) <<
    "      id is: " << GU::idToString(container.shapeId) << std::endl;
  return os;
}

std::ostream& Selection::operator<<(std::ostream& os, const Containers& containers)
{
  for (const auto &current : containers)
    os << current;
  return os;
}

EventHandler::EventHandler() : osgGA::GUIEventHandler()
{
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
                    osgGA::GUIActionAdapter& actionAdapter, osg::Object *object,
                                   osg::NodeVisitor * nodeVistor)
{
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
           currentIntersections = picker->getIntersections();
           osgUtil::LineSegmentIntersector::Intersections::const_iterator itIt;

           Selection::Container newContainer;
           for (itIt = currentIntersections.begin(); itIt != currentIntersections.end(); ++itIt)
           {
               Selection::Container tempContainer;
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
	     (lastPrehighlight.selectionType == Selection::Type::Wire) ||
	     (lastPrehighlight.selectionType == Selection::Type::Edge)
	   )
           {
               clearPrehighlight();
               return false;
           }
           
           clearPrehighlight();
           
           if (isPointType(newContainer.selectionType))
	   {
	     ref_ptr<Geode> pointGeode(buildTempPoint(newContainer.pointLocation));
	     Selected freshSelection;
	     freshSelection.initialize(pointGeode->getDrawable(0)->asGeometry());
	     newContainer.selections.push_back(freshSelection);
	     osg::Switch *tempSwitch = itIt->nodePath[itIt->nodePath.size() - 2]->asSwitch();
	     tempSwitch->addChild(pointGeode);
	   }
	   
           if (newContainer.selections.empty())
               return false;
           
	   setPrehighlight(newContainer);
        }
        else
            clearPrehighlight();
    }

    if (eventAdapter.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON &&
            eventAdapter.getEventType() == osgGA::GUIEventAdapter::RELEASE)
    {
        if (lastPrehighlight.selections.size() > 0)
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
	  
            std::vector<Selected>::iterator it;
            for (it = lastPrehighlight.selections.begin(); it != lastPrehighlight.selections.end(); ++it)
            {
                Selected currentSelected = *it;
                assert(currentSelected.geometry.valid());
                setGeometryColor(currentSelected.geometry.get(), selectionColor);
            }
            selectionContainers.push_back(lastPrehighlight);
	    
	    msg::Message addMessage;
	    addMessage.mask = msg::Response | msg::Post | msg::Selection | msg::Addition;
	    slc::Message addSMessage;
	    addSMessage.type = lastPrehighlight.selectionType;
	    addSMessage.featureId = lastPrehighlight.featureId;
	    addSMessage.shapeId = lastPrehighlight.shapeId;
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

bool EventHandler::buildPreSelection(Selection::Container &container,
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
      const Feature::Base *feature = dynamic_cast<Application *>(qApp)->getProject()->findFeature(featureId);
      assert(feature);
      const ModelViz::Connector &connector = feature->getConnector();
      
      osgUtil::LineSegmentIntersector::Intersections::const_iterator it = intersection;

      uuid faceId = boost::uuids::nil_generator()();
      uuid edgeId = boost::uuids::nil_generator()();
      osgUtil::LineSegmentIntersector::Intersections::const_iterator edgeIt = currentIntersections.end();
      for(; it != currentIntersections.end(); ++it)
      {
          assert((*it).nodePath.size() - 2 >= 0);
          int currentNodeMask = (*it).nodePath[(*it).nodePath.size() - 2]->getNodeMask();
          if (currentNodeMask == NodeMaskDef::face && faceId.is_nil())
              faceId = GU::getId((*it).nodePath.back());
          if (currentNodeMask == NodeMaskDef::edge)
          {
              edgeId = GU::getId((*it).nodePath.back());
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

      std::vector<uuid> edges = connector.useGetChildrenOfType(wireId, TopAbs_EDGE);
      std::vector<osg::Geometry *> edgeGeometry;
      getGeometryFromIds visit(edges, edgeGeometry);
      edgeIt->nodePath[edgeIt->nodePath.size() - 2]->accept(visit);
      
      std::vector<osg::Geometry *>::const_iterator geomIt;
      for (geomIt = edgeGeometry.begin(); geomIt != edgeGeometry.end(); ++geomIt)
      {
          Selected newSelection;
          newSelection.initialize(*geomIt);
          container.selections.push_back(newSelection);
      }
      container.selectionType = Type::Wire;
      container.shapeId = wireId;
      
      return true;
    };
    
    osg::Geometry *geometry = dynamic_cast<Geometry *>((*intersection).drawable.get());
    if (!geometry)
        return false;

    int localNodeMask = (*intersection).nodePath[(*intersection).nodePath.size() - 2]->getNodeMask();
    uuid selectedId = GU::getId((*intersection).nodePath.back());
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
          Selected newSelection;
          newSelection.initialize(geometry);
          container.selections.push_back(newSelection);
          container.selectionType = Type::Face;
          container.shapeId = selectedId;
        }
        else if (canSelectShells(selectionMask))
        {
            const Feature::Base *feature = dynamic_cast<Application *>(qApp)->getProject()->findFeature(featureId);
            assert(feature);
            const ModelViz::Connector &connector = feature->getConnector();
            std::vector<uuid> shells = connector.useGetParentsOfType(selectedId, TopAbs_SHELL);
            //should be only 1 shell
            if (shells.size() == 1)
            {
                std::vector<uuid> faces = connector.useGetChildrenOfType(shells.at(0), TopAbs_FACE);
                std::vector<osg::Geometry *> faceGeometry;
                getGeometryFromIds visit(faces, faceGeometry);
                (*intersection).nodePath[(*intersection).nodePath.size() - 2]->accept(visit);
                std::vector<osg::Geometry *>::const_iterator geomIt;
                for (geomIt = faceGeometry.begin(); geomIt != faceGeometry.end(); ++geomIt)
                {
                    Selected newSelection;
                    newSelection.initialize(*geomIt);
                    container.selections.push_back(newSelection);
                }
                container.selectionType = Type::Shell;
                container.shapeId = shells.at(0);
            }
        }
        else if (canSelectSolids(selectionMask))
        {
            const Feature::Base *feature = dynamic_cast<Application *>(qApp)->getProject()->findFeature(featureId);
            assert(feature);
            const ModelViz::Connector &connector = feature->getConnector();
            std::vector<uuid> solids = connector.useGetParentsOfType(selectedId, TopAbs_SOLID);
            //should be only 1 solid
            if (solids.size() == 1)
            {
                std::vector<uuid> faces = connector.useGetChildrenOfType(solids.at(0), TopAbs_FACE);
                std::vector<osg::Geometry *> faceGeometry;
                getGeometryFromIds visit(faces, faceGeometry);
                (*intersection).nodePath[(*intersection).nodePath.size() - 2]->accept(visit);
                std::vector<osg::Geometry *>::const_iterator geomIt;
                for (geomIt = faceGeometry.begin(); geomIt != faceGeometry.end(); ++geomIt)
                {
                    Selected newSelection;
                    newSelection.initialize(*geomIt);
                    container.selections.push_back(newSelection);
                }
                container.selectionType = Type::Solid;
                container.shapeId = solids.at(0);
            }
        }
        else if (canSelectObjects(selectionMask))
        {
          const Feature::Base *feature = dynamic_cast<Application *>(qApp)->getProject()->findFeature(featureId);
          assert(feature);
          const ModelViz::Connector &connector = feature->getConnector();
          uuid object = connector.useGetRoot();
          if (!object.is_nil())
          {
            std::vector<uuid> faces = connector.useGetChildrenOfType(object, TopAbs_FACE);
            std::vector<osg::Geometry *> faceGeometry;
            getGeometryFromIds visit(faces, faceGeometry);
            (*intersection).nodePath[(*intersection).nodePath.size() - 2]->accept(visit);
            std::vector<osg::Geometry *>::const_iterator geomIt;
            for (geomIt = faceGeometry.begin(); geomIt != faceGeometry.end(); ++geomIt)
            {
                Selected newSelection;
                newSelection.initialize(*geomIt);
                container.selections.push_back(newSelection);
            }
            container.selectionType = Type::Object;
            container.shapeId = nil_generator()();
          }
        }
        break;
    }
    case NodeMaskDef::edge:
	if (canSelectPoints(selectionMask))
	{
	  const Feature::Base *feature = dynamic_cast<Application *>(qApp)->getProject()->findFeature(featureId);
          assert(feature);
          const ModelViz::Connector &connector = feature->getConnector();
	  osg::Vec3d iPoint = intersection->getWorldIntersectPoint();
	  osg::Vec3d snapPoint;
	  double distance = std::numeric_limits<double>::max();
	  Selection::Type sType = Selection::Type::None;
	  
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
	      sType = Selection::Type::EndPoint;
	  }
	  
	  if (canSelectMidPoints(selectionMask))
	  {
	    std::vector<osg::Vec3d> midPoints = connector.useGetMidPoint(selectedId);
	    if (updateSnaps(midPoints))
	      sType = Selection::Type::MidPoint;
	  }
	  
	  if (canSelectCenterPoints(selectionMask))
	  {
	    std::vector<osg::Vec3d> centerPoints = connector.useGetCenterPoint(selectedId);
	    if (updateSnaps(centerPoints))
	      sType = Selection::Type::CenterPoint;
	  }
	  
	  if (canSelectQuadrantPoints(selectionMask))
	  {
	    std::vector<osg::Vec3d> quadrantPoints = connector.useGetQuadrantPoints(selectedId);
	    if (updateSnaps(quadrantPoints))
	      sType = Selection::Type::QuadrantPoint;
	  }
	  
	  if (canSelectNearestPoints(selectionMask))
	  {
	    std::vector<osg::Vec3d> nearestPoints = connector.useGetNearestPoint(selectedId, intersection->getWorldIntersectPoint());
	    if (updateSnaps(nearestPoints))
	      sType = Selection::Type::NearestPoint;
	  }
	  container.selectionType = sType;
	  container.shapeId = selectedId;
	  container.pointLocation = snapPoint;
	}
        else if (canSelectEdges(selectionMask))
        {
            Selected newSelection;
            newSelection.initialize(geometry);
            container.selectionType = Type::Edge;
            container.shapeId = selectedId;
            container.selections.push_back(newSelection);
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
      
        std::vector<Selected>::iterator selectedIt;
        for (selectedIt = it->selections.begin(); selectedIt != it->selections.end(); ++selectedIt)
        {
          if (!selectedIt->geometry.valid())
            continue;
          //should I lock the geometry observer point?
          setGeometryColor(selectedIt->geometry.get(), selectedIt->color);
	  
	  //snap points are temporary objects. so remove the geometry.
	  if (isPointType(it->selectionType))
	  {
	    osg::Geode *geode = selectedIt->geometry.get()->getParent(0)->asGeode();
	    osg::Switch *aSwitch = geode->getParent(0)->asSwitch();
	    aSwitch->removeChild(geode);
	  }
        }
    }
    selectionContainers.clear();
}

bool EventHandler::alreadySelected(const Selection::Container &testContainer)
{
    Containers::const_iterator it;
    for (it = selectionContainers.begin(); it != selectionContainers.end(); ++it)
    {
        if ((*it) == testContainer)
            return true;
    }
    return false;
}

void EventHandler::setPrehighlight(Selection::Container &selected)
{
    lastPrehighlight = selected;
    std::vector<Selected>::const_iterator it;
    for (it = selected.selections.begin(); it != selected.selections.end(); ++it)
        setGeometryColor(it->geometry.get(), preHighlightColor);
    
    msg::Message addMessage;
    addMessage.mask = msg::Response | msg::Post | msg::Preselection | msg::Addition;
    slc::Message addSMessage;
    addSMessage.type = lastPrehighlight.selectionType;
    addSMessage.featureId = lastPrehighlight.featureId;
    addSMessage.shapeId = lastPrehighlight.shapeId;
    addMessage.payload = addSMessage;
    messageOutSignal(addMessage);
}

void EventHandler::clearPrehighlight()
{
    if (lastPrehighlight.selections.size() < 1)
        return;
    
    msg::Message removeMessage;
    removeMessage.mask = msg::Response | msg::Pre | msg::Preselection | msg::Subtraction;
    slc::Message removeSMessage;
    removeSMessage.type = lastPrehighlight.selectionType;
    removeSMessage.featureId = lastPrehighlight.featureId;
    removeSMessage.shapeId = lastPrehighlight.shapeId;
    removeMessage.payload = removeSMessage;
    messageOutSignal(removeMessage);
    
    
    std::vector<Selected>::const_iterator it;
    for (it = lastPrehighlight.selections.begin(); it != lastPrehighlight.selections.end(); ++it)
    {
      if (!it->geometry.valid())
        continue;
      //should I lock the geometry observer point?
      setGeometryColor(it->geometry.get(), it->color);
      
      //snap points are temporary objects. so remove the geometry.
      if (isPointType(lastPrehighlight.selectionType))
      {
	osg::Geode *geode = it->geometry.get()->getParent(0)->asGeode();
	osg::Switch *aSwitch = geode->getParent(0)->asSwitch();
	aSwitch->removeChild(geode);
      }
    }
    
    lastPrehighlight = Container();
}

Container EventHandler::buildContainer(const msg::Message &messageIn)
{
  slc::Message sMessage = boost::get<slc::Message>(messageIn.payload);
  assert(!sMessage.featureId.is_nil());
  
  Selection::Container container;
  const Feature::Base *feature = dynamic_cast<Application *>(qApp)->getProject()->findFeature(sMessage.featureId);
  assert(feature);
  const ModelViz::Connector &connector = feature->getConnector();
  //only object supported at this time.
  if (sMessage.type == Selection::Type::Object)
  {
    uuid object = connector.useGetRoot();
    assert(!object.is_nil());
    std::vector<uuid> faces = connector.useGetChildrenOfType(object, TopAbs_FACE);
    std::vector<osg::Geometry *> faceGeometry;
    getGeometryFromIds visit(faces, faceGeometry);
    feature->getMainSwitch()->accept(visit); //starting higher than I need. FYI.
    std::vector<osg::Geometry *>::const_iterator geomIt;
    for (geomIt = faceGeometry.begin(); geomIt != faceGeometry.end(); ++geomIt)
    {
	Selected newSelection;
	newSelection.initialize(*geomIt);
	container.selections.push_back(newSelection);
    }
    container.selectionType = Type::Object;
    container.featureId = sMessage.featureId;
    container.shapeId = nil_generator()();
  }
  return container;
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

void EventHandler::requestPreselectionAdditionDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  Selection::Container container = buildContainer(messageIn);
  if (alreadySelected(container))
    return;
  if (!container.selections.empty())
    setPrehighlight(container);
}

void EventHandler::requestPreselectionSubtractionDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  Selection::Container container = buildContainer(messageIn);
  if (alreadySelected(container))
    return;
  clearPrehighlight();
}

void EventHandler::requestSelectionAdditionDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  //there is a good chance that what we are selecting has already been preselected.
  //because buildContainer pulls the current color from objects it will
  //pull the prehighlight color from the object if we don't clear the prehighlight.
  clearPrehighlight();
  
  Selection::Container container = buildContainer(messageIn);
  if (!alreadySelected(container))
  {
    std::vector<Selected>::iterator it;
    for (it = container.selections.begin(); it != container.selections.end(); ++it)
    {
	Selected currentSelected = *it;
	assert(currentSelected.geometry.valid());
	setGeometryColor(currentSelected.geometry.get(), selectionColor);
    }
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
  
  Selection::Container container = buildContainer(messageIn);
  Containers::iterator containIt = std::find(selectionContainers.begin(), selectionContainers.end(), container);
  assert(containIt != selectionContainers.end());
  
  msg::Message messageOut = messageIn;
  messageOut.mask &= ~msg::Request;
  messageOut.mask |= msg::Response | msg::Pre;
  messageOutSignal(messageOut);
  
  std::vector<Selected>::iterator it;
  for (it = containIt->selections.begin(); it != containIt->selections.end(); ++it)
  {
    assert(it->geometry.valid());
    setGeometryColor(it->geometry.get(), it->color);
  }
  
  selectionContainers.erase(containIt);
}

void EventHandler::requestSelectionClearDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  clearPrehighlight();
  clearSelections();
}

void EventHandler::setGeometryColor(osg::Geometry *geometryIn, const osg::Vec4 &colorIn)
{
    Vec4Array *colors = dynamic_cast<Vec4Array*>(geometryIn->getColorArray());
    assert(colors);
    assert(colors->size() > 0);

    (*colors)[0] = colorIn;
    colors->dirty();
    geometryIn->dirtyDisplayList();
}

Geode* EventHandler::buildTempPoint(const Vec3d& pointIn)
{
  osg::Geode *geode = new osg::Geode();
  geode->setCullingActive(false);
  geode->setNodeMask(NodeMaskDef::point);
  
  osg::Geometry *geometry = new osg::Geometry();
  osg::Vec3Array *vertices = new osg::Vec3Array();
  vertices->push_back(pointIn);
  geometry->setVertexArray(vertices);
  osg::Vec4Array *colors = new osg::Vec4Array();
  colors->push_back(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
  geometry->setColorArray(colors);
  geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
  geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, 1));
  geode->addDrawable(geometry);
  
  osg::Point *point = new osg::Point();
  point->setSize(10.0);
  geode->getOrCreateStateSet()->setAttribute(point);
  
  osg::Depth *depth = new osg::Depth();
  depth->setRange(0.0, 1.0);
  geode->getOrCreateStateSet()->setAttribute(depth);
  
  geode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
  
  return geode;
}

void Selected::initialize(osg::Geometry *geometryIn)
{
    this->geometry = geometryIn;
    Vec4Array *colors = dynamic_cast<Vec4Array*>(geometryIn->getColorArray());
    assert(colors);
    assert(colors->size() > 0);
    this->color = (*colors)[0];
}

getGeometryFromIds::getGeometryFromIds(const std::vector<boost::uuids::uuid> &idsIn, std::vector<osg::Geometry *> &geometryIn) :
    osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), ids(idsIn), geometry(geometryIn)
{

}

void getGeometryFromIds::apply(osg::Geode &aGeode)
{
    std::string selectedIdString;
    if (!aGeode.getUserValue(GU::idAttributeTitle, selectedIdString))
        assert(0);
    uuid selectedId = GU::stringToId(selectedIdString);
    //consider switching connector and this visitor to a std::set instead of a vector for speed here.
    if (std::find(ids.begin(), ids.end(), selectedId) == ids.end())
    {
        traverse(aGeode);
        return;
    }
    osg::Geometry *geom = dynamic_cast<osg::Geometry *>(aGeode.getDrawable(0));
    assert(geom);
    geometry.push_back(geom);
}

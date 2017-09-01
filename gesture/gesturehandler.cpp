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

#include <QImage>
#include <QGLWidget>
#include <QMetaObject>
#include <QCoreApplication>

#include <osgViewer/View>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgUtil/LineSegmentIntersector>
#include <osg/ValueObject>
#include <osg/Depth>

#include <viewer/message.h>
#include <gesture/gesturenode.h>
#include <modelviz/nodemaskdefs.h>
#include <message/dispatch.h>
#include <message/observer.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <gesture/animations.h>
#include <viewer/spaceballosgevent.h>
#include <gesture/gesturehandler.h>

static const std::string attributeMask = "CommandAttributeTitle";
static const std::string attributeStatus = "CommandAttributeStatus";

GestureHandler::GestureHandler(osg::Camera *cameraIn) : osgGA::GUIEventHandler(), rightButtonDown(false),
    currentNodeLeft(false), iconRadius(32.0), includedAngle(90.0)
{
    observer = std::move(std::unique_ptr<msg::Observer>(new msg::Observer()));
    observer->name = "GestureHandler";
    setupDispatcher();
  
    gestureCamera = cameraIn;
    if (!gestureCamera.valid())
        return;
    gestureSwitch = dynamic_cast<osg::Switch *>(gestureCamera->getChild(0));
    if (!gestureSwitch.valid())
        return;
    
    updateVariables();
    constructMenu();
}

bool GestureHandler::handle(const osgGA::GUIEventAdapter& eventAdapter,
                            osgGA::GUIActionAdapter&, osg::Object *,
                            osg::NodeVisitor *)
{
  if (eventAdapter.getEventType() == osgGA::GUIEventAdapter::USER)
  {
    const spb::SpaceballOSGEvent *event = 
      dynamic_cast<const spb::SpaceballOSGEvent *>(eventAdapter.getUserData());
    assert (event);
    
    if (event->theType == spb::SpaceballOSGEvent::Button)
    {
      int currentButton = event->buttonNumber;
      spb::SpaceballOSGEvent::ButtonState currentState = event->theButtonState;
      if (currentState == spb::SpaceballOSGEvent::Pressed)
      {
        spaceballButton = currentButton;
        if (!rightButtonDown)
        {
          std::string maskString = prf::manager().getSpaceballButton(spaceballButton);
          if (!maskString.empty())
          {
            msg::Mask msgMask(maskString);
            msg::Message messageOut;
            messageOut.mask = msgMask;
            observer->out(messageOut);
          }
        }
        else
        {
          std::ostringstream stream;
          stream << QObject::tr("Link to spaceball button ").toStdString() << spaceballButton;
          observer->out(msg::buildStatusMessage(stream.str()));
        }
      }
      else
      {
        assert(currentState == spb::SpaceballOSGEvent::Released);
        spaceballButton = -1;
        observer->out(msg::buildStatusMessage(""));
      }
      return true;
    }
  }
  
    //lambda to clear status.
    auto clearStatus = [&]()
    {
      //clear any status message
      observer->out(msg::buildStatusMessage(""));
    };
    
    if (eventAdapter.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_LEFT_CTRL)
    {
        if (dragStarted)
        {
        clearStatus();
        GestureAllSwitchesOffVisitor visitor;
        gestureCamera->accept(visitor);
        dragStarted = false;
        gestureSwitch->setAllChildrenOff();
        }
        return false;
    }
  
    if (!gestureSwitch.valid())
        return false;

    if (eventAdapter.getButton() == osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON)
    {
        if (eventAdapter.getEventType() == osgGA::GUIEventAdapter::PUSH)
        {
            GestureAllSwitchesOffVisitor visitor;
            gestureCamera->accept(visitor);

            rightButtonDown = true;
        }

        if (eventAdapter.getEventType() == osgGA::GUIEventAdapter::RELEASE)
        {
            clearStatus();
            rightButtonDown = false;
            if (!dragStarted)
                return false;
            dragStarted = false;
            gestureSwitch->setAllChildrenOff();
            if (currentNode->getNodeMask() & mdv::gestureCommand)
            {
                std::string msgMaskString;
                if (currentNode->getUserValue(attributeMask, msgMaskString))
                {
                    if (spaceballButton != -1)
                    {
                      prf::manager().setSpaceballButton(spaceballButton, msgMaskString);
                      prf::manager().saveConfig();
                    }
                    
                    msg::Mask msgMask(msgMaskString);
                    msg::Message messageOut;
                    messageOut.mask = msgMask;
//                     observer->out(messageOut);
                    QMetaObject::invokeMethod(qApp, "messageSlot", Qt::QueuedConnection, Q_ARG(msg::Message, messageOut));
                }
                else
                    assert(0); //gesture node doesn't have msgMask attribute;
            }
        }
    }

    if (eventAdapter.getEventType() == osgGA::GUIEventAdapter::DRAG)
    {
      if (!rightButtonDown) //only right button drag
          return false;
      if (!dragStarted)
      {
          dragStarted = true;
          startDrag(eventAdapter);
      }

      osg::Matrixd projection = gestureCamera->getProjectionMatrix();
      osg::Matrixd window = gestureCamera->getViewport()->computeWindowMatrix();
      osg::Matrixd transformation = projection * window;
      transformation = osg::Matrixd::inverse(transformation);

      osg::Vec3 temp(eventAdapter.getX(), eventAdapter.getY(), 0.0);
      temp = transformation * temp;

      osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector = new osgUtil::LineSegmentIntersector
              (osgUtil::Intersector::MODEL, temp.x(), temp.y());
      osgUtil::IntersectionVisitor iv(intersector);
      iv.setTraversalMask(~mdv::gestureCamera);
      gestureSwitch->accept(iv);
      
      //look for icon intersection, but send message when intersecting lines for user feedback.
      osg::ref_ptr<osg::Drawable> drawable;
      osg::ref_ptr<osg::MatrixTransform> node;
      osg::Vec3 hPoint;
      for (auto &intersection : intersector->getIntersections())
      {
        osg::ref_ptr<osg::Drawable> tempDrawable = intersection.drawable;
        assert(tempDrawable.valid());
        osg::ref_ptr<osg::MatrixTransform> tempNode = dynamic_cast<osg::MatrixTransform*>
          (tempDrawable->getParent(0)->getParent(0)->getParent(0));
        assert(temp.valid());
        
        std::string statusString;
        if (tempNode->getUserValue(attributeStatus, statusString))
          observer->out(msg::buildStatusMessage(statusString));
        
        if (tempDrawable->getName() != "Line")
        {
          drawable = tempDrawable;
          node = tempNode;
          hPoint = intersection.getLocalIntersectPoint();
          break;
        }
      }
      
      if (!drawable.valid()) //no icon intersection found.
      {
        if (currentNodeLeft == false)
        {
          currentNodeLeft = true;
          if (currentNode->getNodeMask() & mdv::gestureMenu)
            spraySubNodes(temp);
        }
        
        return false;
      }
      
      lastHitPoint = hPoint;
      if (node == currentNode)
      {
        if (currentNodeLeft == true)
        {
          currentNodeLeft = false;
          if (currentNode->getNodeMask() & mdv::gestureMenu)
              contractSubNodes();
        }
      }
      else
      {
        osg::MatrixTransform *parentNode = currentNode;
        currentNode = node;

        osg::Switch *geometrySwitch = dynamic_cast<osg::Switch*>(parentNode->getChild(parentNode->getNumChildren() - 1));
        assert(geometrySwitch);
        geometrySwitch->setAllChildrenOff();

        unsigned int childIndex = parentNode->getChildIndex(currentNode);
        for (unsigned int index = 0; index < parentNode->getNumChildren() - 2; ++index)
        {
          osg::MatrixTransform *childNode = dynamic_cast<osg::MatrixTransform*>(parentNode->getChild(index));
          assert(childNode);

          if (index != childIndex)
          {
            osg::Switch *childGeometrySwitch = dynamic_cast<osg::Switch*>(childNode->getChild(childNode->getNumChildren() - 1));
            assert(childGeometrySwitch);
            childGeometrySwitch->setAllChildrenOff();
          }

          osg::Switch *childLineSwitch = dynamic_cast<osg::Switch*>(childNode->getChild(childNode->getNumChildren() - 2));
          assert(childLineSwitch);
          childLineSwitch->setAllChildrenOff();
        }

        currentNodeLeft = false;
        aggregateMatrix = currentNode->getMatrix() * aggregateMatrix;
      }
    }
    return false;
}

void GestureHandler::spraySubNodes(osg::Vec3 cursorLocation)
{
    cursorLocation = cursorLocation * osg::Matrixd::inverse(aggregateMatrix);
    osg::Vec3 direction = cursorLocation - lastHitPoint;

    int childCount = currentNode->getNumChildren();
    assert(childCount > 2);//line, icon and sub items.
    if (childCount < 3)
      return;
    std::vector<osg::Vec3> locations = buildNodeLocations(direction, childCount - 2);
    for (unsigned int index = 0; index < locations.size(); ++index)
    {
        osg::MatrixTransform *tempLocation = dynamic_cast<osg::MatrixTransform *>
                (currentNode->getChild(index));
        assert(tempLocation);
        
        gsn::NodeExpand *childAnimation = new gsn::NodeExpand
          (osg::Vec3d(0.0, 0.0, 0.0), locations.at(index), time());
        tempLocation->setUpdateCallback(childAnimation);

        osg::Switch *tempSwitch = dynamic_cast<osg::Switch *>
                (tempLocation->getChild(tempLocation->getNumChildren() - 1));
        assert(tempSwitch);
        tempSwitch->setAllChildrenOn();
        
        tempSwitch = dynamic_cast<osg::Switch *>
                (tempLocation->getChild(tempLocation->getNumChildren() - 2));
        assert(tempSwitch);
        tempSwitch->setAllChildrenOn();

        osg::Geometry *geometry = dynamic_cast<osg::Geometry*>(tempSwitch->getChild(0)->asGeode()->getDrawable(0));
        assert(geometry);
        osg::Vec3Array *pointArray = dynamic_cast<osg::Vec3Array *>(geometry->getVertexArray());
        assert(pointArray);
        (*pointArray)[1] = locations.at(index) * -1.0;
        geometry->dirtyDisplayList();
        geometry->dirtyBound();
        
        gsn::GeometryExpand *lineAnimate = new gsn::GeometryExpand
          (osg::Vec3d(0.0, 0.0, 0.0), -locations.at(index), time());
        geometry->setUpdateCallback(lineAnimate);
    }
}

void GestureHandler::contractSubNodes()
{
    int childCount = currentNode->getNumChildren();
    assert(childCount > 2);//line, icon and sub items.
    for (int index = 0; index < childCount - 2; ++index)
    {
        osg::MatrixTransform *tempLocation = dynamic_cast<osg::MatrixTransform *>
                (currentNode->getChild(index));
        assert(tempLocation);
        
        gsn::NodeCollapse *childAnimation = new gsn::NodeCollapse
          (tempLocation->getMatrix().getTrans(), osg::Vec3d(0.0, 0.0, 0.0), time());
        tempLocation->setUpdateCallback(childAnimation);
        
        osg::Geometry *geometry = dynamic_cast<osg::Geometry*>
          (tempLocation->getChild(tempLocation->getNumChildren() - 2)->
          asSwitch()->getChild(0)->asGeode()->getDrawable(0));
        assert(geometry);
        
        gsn::GeometryCollapse *lineAnimate = new gsn::GeometryCollapse
          (osg::Vec3d(0.0, 0.0, 0.0), -(tempLocation->getMatrix().getTrans()), time());
          
        geometry->setUpdateCallback(lineAnimate);
    }
}

float GestureHandler::time()
{
  return static_cast<float>(prf::manager().rootPtr->gesture().animationSeconds());
}

void GestureHandler::constructMenu()
{
    //should only be 2. the start object and the transparent quad
    gestureSwitch->removeChildren(0, gestureSwitch->getNumChildren() - 1);
  
    startNode = gsn::buildMenuNode(":/resources/images/start.svg", iconRadius);
    startNode->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    startNode->getOrCreateStateSet()->setAttributeAndModes(new osg::Depth(osg::Depth::LEQUAL, 0.0, 0.0));
    gestureSwitch->insertChild(0, startNode);

    osg::Matrixd dummy;

    //view base
    osg::MatrixTransform *viewBase;
    viewBase = gsn::buildMenuNode(":/resources/images/viewBase.svg", iconRadius);
    viewBase->setMatrix(dummy);
    viewBase->setUserValue(attributeStatus, QObject::tr("View Menu").toStdString());
    startNode->insertChild(startNode->getNumChildren() - 2, viewBase);

    osg::MatrixTransform *viewStandard;
    viewStandard = gsn::buildMenuNode(":/resources/images/viewStandard.svg", iconRadius);
    viewStandard->setMatrix(dummy);
    viewStandard->setUserValue(attributeStatus, QObject::tr("Standard Views Menu").toStdString());
    viewBase->insertChild(viewBase->getNumChildren() - 2, viewStandard);

    osg::MatrixTransform *viewTop = gsn::buildCommandNode(":/resources/images/viewTop.svg", iconRadius);
    viewTop->setMatrix(dummy);
    viewTop->setUserValue(attributeMask, (msg::Request | msg::ViewTop).to_string());
    viewTop->setUserValue(attributeStatus, QObject::tr("Top View Command").toStdString());
    viewStandard->insertChild(viewStandard->getNumChildren() - 2, viewTop);

    osg::MatrixTransform *viewFront = gsn::buildCommandNode(":/resources/images/viewFront.svg", iconRadius);
    viewFront->setMatrix(dummy);
    viewFront->setUserValue(attributeMask, (msg::Request | msg::ViewFront).to_string());
    viewFront->setUserValue(attributeStatus, QObject::tr("View Front Command").toStdString());
    viewStandard->insertChild(viewStandard->getNumChildren() - 2, viewFront);

    osg::MatrixTransform *viewRight = gsn::buildCommandNode(":/resources/images/viewRight.svg", iconRadius);
    viewRight->setMatrix(dummy);
    viewRight->setUserValue(attributeMask, (msg::Request | msg::ViewRight).to_string());
    viewRight->setUserValue(attributeStatus, QObject::tr("View Right Command").toStdString());
    viewStandard->insertChild(viewStandard->getNumChildren() - 2, viewRight);

    osg::MatrixTransform *viewIso = gsn::buildCommandNode(":/resources/images/viewIso.svg", iconRadius);
    viewIso->setMatrix(dummy);
    viewIso->setUserValue(attributeMask, (msg::Request | msg::ViewIso).to_string());
    viewIso->setUserValue(attributeStatus, QObject::tr("View Iso Command").toStdString());
    viewStandard->insertChild(viewStandard->getNumChildren() - 2, viewIso);

    osg::MatrixTransform *viewFit = gsn::buildCommandNode(":/resources/images/viewFit.svg", iconRadius);
    viewFit->setMatrix(dummy);
    viewFit->setUserValue(attributeMask, (msg::Request | msg::ViewFit).to_string());
    viewFit->setUserValue(attributeStatus, QObject::tr("View Fit Command").toStdString());
    viewBase->insertChild(viewBase->getNumChildren() - 2, viewFit);
    
    osg::MatrixTransform *viewFill = gsn::buildCommandNode(":/resources/images/viewFill.svg", iconRadius);
    viewFill->setMatrix(dummy);
    viewFill->setUserValue(attributeMask, (msg::Request | msg::ViewFill).to_string());
    viewFill->setUserValue(attributeStatus, QObject::tr("View Fill Command").toStdString());
    viewBase->insertChild(viewBase->getNumChildren() - 2, viewFill);
    
    osg::MatrixTransform *viewLine = gsn::buildCommandNode(":/resources/images/viewLine.svg", iconRadius);
    viewLine->setMatrix(dummy);
    viewLine->setUserValue(attributeMask, (msg::Request | msg::ViewLine).to_string());
    viewLine->setUserValue(attributeStatus, QObject::tr("View Lines Command").toStdString());
    viewBase->insertChild(viewBase->getNumChildren() - 2, viewLine);
    
    osg::MatrixTransform *viewToggleHiddenLines = gsn::buildCommandNode(":/resources/images/viewHiddenLines.svg", iconRadius);
    viewToggleHiddenLines->setMatrix(dummy);
    viewToggleHiddenLines->setUserValue(attributeMask, (msg::Request | msg::ViewToggleHiddenLine).to_string());
    viewToggleHiddenLines->setUserValue(attributeStatus, QObject::tr("Toggle hidden lines Command").toStdString());
    viewBase->insertChild(viewBase->getNumChildren() - 2, viewToggleHiddenLines);
    
    osg::MatrixTransform *viewIsolate = gsn::buildCommandNode(":/resources/images/viewIsolate.svg", iconRadius);
    viewIsolate->setMatrix(dummy);
    viewIsolate->setUserValue(attributeMask, (msg::Request | msg::ViewIsolate).to_string());
    viewIsolate->setUserValue(attributeStatus, QObject::tr("View Only Selected").toStdString());
    viewBase->insertChild(viewBase->getNumChildren() - 2, viewIsolate);
    
    //construction base
    osg::MatrixTransform *constructionBase;
    constructionBase = gsn::buildMenuNode(":/resources/images/constructionBase.svg", iconRadius);
    constructionBase->setMatrix(dummy);
    constructionBase->setUserValue(attributeStatus, QObject::tr("Construction Menu").toStdString());
    startNode->insertChild(startNode->getNumChildren() - 2, constructionBase);

    osg::MatrixTransform *constructionPrimitives;
    constructionPrimitives = gsn::buildMenuNode(":/resources/images/constructionPrimitives.svg", iconRadius);
    constructionPrimitives->setMatrix(dummy);
    constructionPrimitives->setUserValue(attributeStatus, QObject::tr("Primitives Menu").toStdString());
    constructionBase->insertChild(constructionBase->getNumChildren() - 2, constructionPrimitives);

    osg::MatrixTransform *constructionBox = gsn::buildCommandNode(":/resources/images/constructionBox.svg", iconRadius);
    constructionBox->setMatrix(dummy);
    constructionBox->setUserValue(attributeMask, (msg::Request | msg::Construct | msg::Box).to_string());
    constructionBox->setUserValue(attributeStatus, QObject::tr("Box Command").toStdString());
    constructionPrimitives->insertChild(constructionPrimitives->getNumChildren() - 2, constructionBox);

    osg::MatrixTransform *constructionSphere = gsn::buildCommandNode(":/resources/images/constructionSphere.svg", iconRadius);
    constructionSphere->setMatrix(dummy);
    constructionSphere->setUserValue(attributeMask, (msg::Request | msg::Construct | msg::Sphere).to_string());
    constructionSphere->setUserValue(attributeStatus, QObject::tr("Sphere Command").toStdString());
    constructionPrimitives->insertChild(constructionPrimitives->getNumChildren() - 2, constructionSphere);

    osg::MatrixTransform *constructionCone = gsn::buildCommandNode(":/resources/images/constructionCone.svg", iconRadius);
    constructionCone->setMatrix(dummy);
    constructionCone->setUserValue(attributeMask, (msg::Request | msg::Construct | msg::Cone).to_string());
    constructionCone->setUserValue(attributeStatus, QObject::tr("Cone Command").toStdString());
    constructionPrimitives->insertChild(constructionPrimitives->getNumChildren() - 2, constructionCone);

    osg::MatrixTransform *constructionCylinder = gsn::buildCommandNode(":/resources/images/constructionCylinder.svg", iconRadius);
    constructionCylinder->setMatrix(dummy);
    constructionCylinder->setUserValue(attributeMask, (msg::Request | msg::Construct | msg::Cylinder).to_string());
    constructionCylinder->setUserValue(attributeStatus, QObject::tr("Cylinder Command").toStdString());
    constructionPrimitives->insertChild(constructionPrimitives->getNumChildren() - 2, constructionCylinder);
    
    osg::MatrixTransform *constructionOblong = gsn::buildCommandNode(":/resources/images/constructionOblong.svg", iconRadius);
    constructionOblong->setMatrix(dummy);
    constructionOblong->setUserValue(attributeMask, (msg::Request | msg::Construct | msg::Oblong).to_string());
    constructionOblong->setUserValue(attributeStatus, QObject::tr("Oblong Command").toStdString());
    constructionPrimitives->insertChild(constructionPrimitives->getNumChildren() - 2, constructionOblong);
    
    //construction finishing base
    osg::MatrixTransform *constructionFinishing;
    constructionFinishing = gsn::buildMenuNode(":/resources/images/constructionFinishing.svg", iconRadius);
    constructionFinishing->setMatrix(dummy);
    constructionFinishing->setUserValue(attributeStatus, QObject::tr("Finishing Menu").toStdString());
    constructionBase->insertChild(constructionBase->getNumChildren() - 2, constructionFinishing);
    
    osg::MatrixTransform *constructionBlend = gsn::buildCommandNode(":/resources/images/constructionBlend.svg", iconRadius);
    constructionBlend->setMatrix(dummy);
    constructionBlend->setUserValue(attributeMask, (msg::Request | msg::Construct | msg::Blend).to_string());
    constructionBlend->setUserValue(attributeStatus, QObject::tr("Blend Command").toStdString());
    constructionFinishing->insertChild(constructionFinishing->getNumChildren() - 2, constructionBlend);
    
    osg::MatrixTransform *constructionChamfer = gsn::buildCommandNode(":/resources/images/constructionChamfer.svg", iconRadius);
    constructionChamfer->setMatrix(dummy);
    constructionChamfer->setUserValue(attributeMask, (msg::Request | msg::Construct | msg::Chamfer).to_string());
    constructionChamfer->setUserValue(attributeStatus, QObject::tr("Chamfer Command").toStdString());
    constructionFinishing->insertChild(constructionFinishing->getNumChildren() - 2, constructionChamfer);
    
    osg::MatrixTransform *constructionDraft = gsn::buildCommandNode(":/resources/images/constructionDraft.svg", iconRadius);
    constructionDraft->setMatrix(dummy);
    constructionDraft->setUserValue(attributeMask, (msg::Request | msg::Construct | msg::Draft).to_string());
    constructionDraft->setUserValue(attributeStatus, QObject::tr("Draft Command").toStdString());
    constructionFinishing->insertChild(constructionFinishing->getNumChildren() - 2, constructionDraft);
    
    osg::MatrixTransform *constructionHollow = gsn::buildCommandNode(":/resources/images/constructionHollow.svg", iconRadius);
    constructionHollow->setMatrix(dummy);
    constructionHollow->setUserValue(attributeMask, (msg::Request | msg::Construct | msg::Hollow).to_string());
    constructionHollow->setUserValue(attributeStatus, QObject::tr("Hollow Command").toStdString());
    constructionFinishing->insertChild(constructionFinishing->getNumChildren() - 2, constructionHollow);
    
    osg::MatrixTransform *constructionExtract = gsn::buildCommandNode(":/resources/images/constructionExtract.svg", iconRadius);
    constructionExtract->setMatrix(dummy);
    constructionExtract->setUserValue(attributeMask, (msg::Request | msg::Construct | msg::Extract).to_string());
    constructionExtract->setUserValue(attributeStatus, QObject::tr("Extract Command").toStdString());
    constructionFinishing->insertChild(constructionFinishing->getNumChildren() - 2, constructionExtract);
    
    //booleans
    osg::MatrixTransform *constructionBoolean;
    constructionBoolean = gsn::buildMenuNode(":/resources/images/constructionBoolean.svg", iconRadius);
    constructionBoolean->setMatrix(dummy);
    constructionBoolean->setUserValue(attributeStatus, QObject::tr("Boolean Menu").toStdString());
    constructionBase->insertChild(constructionBase->getNumChildren() - 2, constructionBoolean);
    
    osg::MatrixTransform *constructionUnion = gsn::buildCommandNode(":/resources/images/constructionUnion.svg", iconRadius);
    constructionUnion->setMatrix(dummy);
    constructionUnion->setUserValue(attributeMask, (msg::Request | msg::Construct | msg::Union).to_string());
    constructionUnion->setUserValue(attributeStatus, QObject::tr("Union Command").toStdString());
    constructionBoolean->insertChild(constructionBoolean->getNumChildren() - 2, constructionUnion);
    
    osg::MatrixTransform *constructionSubtract = gsn::buildCommandNode(":/resources/images/constructionSubtract.svg", iconRadius);
    constructionSubtract->setMatrix(dummy);
    constructionSubtract->setUserValue(attributeMask, (msg::Request | msg::Construct | msg::Subtract).to_string());
    constructionSubtract->setUserValue(attributeStatus, QObject::tr("Subtract Command").toStdString());
    constructionBoolean->insertChild(constructionBoolean->getNumChildren() - 2, constructionSubtract);
    
    osg::MatrixTransform *constructionIntersect = gsn::buildCommandNode(":/resources/images/constructionIntersect.svg", iconRadius);
    constructionIntersect->setMatrix(dummy);
    constructionIntersect->setUserValue(attributeMask, (msg::Request | msg::Construct | msg::Intersect).to_string());
    constructionIntersect->setUserValue(attributeStatus, QObject::tr("Intersection Command").toStdString());
    constructionBoolean->insertChild(constructionBoolean->getNumChildren() - 2, constructionIntersect);
    
    //TODO build datum menu node and move datum plane to it.
    osg::MatrixTransform *constructDatumPlane = gsn::buildCommandNode(":/resources/images/constructionDatumPlane.svg", iconRadius);
    constructDatumPlane->setMatrix(dummy);
    constructDatumPlane->setUserValue(attributeMask, (msg::Request | msg::Construct | msg::DatumPlane).to_string());
    constructDatumPlane->setUserValue(attributeStatus, QObject::tr("Datum Plane Command").toStdString());
    constructionBase->insertChild(constructionBase->getNumChildren() - 2, constructDatumPlane);
    
    //edit base
    osg::MatrixTransform *editBase;
    editBase = gsn::buildMenuNode(":/resources/images/editBase.svg", iconRadius);
    editBase->setMatrix(dummy);
    editBase->setUserValue(attributeStatus, QObject::tr("Edit Menu").toStdString());
    startNode->insertChild(startNode->getNumChildren() - 2, editBase);
    
    osg::MatrixTransform *editFeature = gsn::buildCommandNode(":/resources/images/editFeature.svg", iconRadius);
    editFeature->setMatrix(dummy);
    editFeature->setUserValue(attributeMask, (msg::Request | msg::Edit | msg::Feature).to_string());
    editFeature->setUserValue(attributeStatus, QObject::tr("Edit Feature Command").toStdString());
    editBase->insertChild(editBase->getNumChildren() - 2, editFeature);
    
    osg::MatrixTransform *remove = gsn::buildCommandNode(":/resources/images/editRemove.svg", iconRadius);
    remove->setMatrix(dummy);
    remove->setUserValue(attributeMask, (msg::Request | msg::Remove).to_string());
    remove->setUserValue(attributeStatus, QObject::tr("Remove Command").toStdString());
    editBase->insertChild(editBase->getNumChildren() - 2, remove);
    
    osg::MatrixTransform *editColor = gsn::buildCommandNode(":/resources/images/editColor.svg", iconRadius);
    editColor->setMatrix(dummy);
    editColor->setUserValue(attributeMask, (msg::Request | msg::Edit | msg::Feature | msg::Color).to_string());
    editColor->setUserValue(attributeStatus, QObject::tr("Edit Color Command").toStdString());
    editBase->insertChild(editBase->getNumChildren() - 2, editColor);
    
    osg::MatrixTransform *editRename = gsn::buildCommandNode(":/resources/images/editRename.svg", iconRadius);
    editRename->setMatrix(dummy);
    editRename->setUserValue(attributeMask, (msg::Request | msg::Edit | msg::Feature | msg::Name).to_string());
    editRename->setUserValue(attributeStatus, QObject::tr("Edit Name Command").toStdString());
    editBase->insertChild(editBase->getNumChildren() - 2, editRename);
    
    osg::MatrixTransform *editUpdate = gsn::buildCommandNode(":/resources/images/editUpdate.svg", iconRadius);
    editUpdate->setMatrix(dummy);
    editUpdate->setUserValue(attributeMask, (msg::Request | msg::Update).to_string());
    editUpdate->setUserValue(attributeStatus, QObject::tr("Update Command").toStdString());
    editBase->insertChild(editBase->getNumChildren() - 2, editUpdate);
    
    osg::MatrixTransform *editForceUpdate = gsn::buildCommandNode(":/resources/images/editForceUpdate.svg", iconRadius);
    editForceUpdate->setMatrix(dummy);
    editForceUpdate->setUserValue(attributeMask, (msg::Request | msg::ForceUpdate).to_string());
    editForceUpdate->setUserValue(attributeStatus, QObject::tr("Force Update Command").toStdString());
    editBase->insertChild(editBase->getNumChildren() - 2, editForceUpdate);
    
    osg::MatrixTransform *editSystemBase;
    editSystemBase = gsn::buildMenuNode(":/resources/images/systemBase.svg", iconRadius);
    editSystemBase->setMatrix(dummy);
    editSystemBase->setUserValue(attributeStatus, QObject::tr("Coordinate System Menu").toStdString());
    editBase->insertChild(editBase->getNumChildren() - 2, editSystemBase);
    
    osg::MatrixTransform *featureToSystem = gsn::buildCommandNode(":/resources/images/featureToSystem.svg", iconRadius);
    featureToSystem->setMatrix(dummy);
    featureToSystem->setUserValue(attributeMask, (msg::Request | msg::FeatureToSystem).to_string());
    featureToSystem->setUserValue(attributeStatus, QObject::tr("Feature To Coordinate System Command").toStdString());
    editSystemBase->insertChild(editSystemBase->getNumChildren() - 2, featureToSystem);
    
    osg::MatrixTransform *editFeatureToDragger = gsn::buildCommandNode(":/resources/images/editFeatureToDragger.svg", iconRadius);
    editFeatureToDragger->setMatrix(dummy);
    editFeatureToDragger->setUserValue(attributeMask, (msg::Request | msg::FeatureToDragger).to_string());
    editFeatureToDragger->setUserValue(attributeStatus, QObject::tr("Feature To Dragger Command").toStdString());
    editSystemBase->insertChild(editSystemBase->getNumChildren() - 2, editFeatureToDragger);
    
    osg::MatrixTransform *editDraggerToFeature = gsn::buildCommandNode(":/resources/images/editDraggerToFeature.svg", iconRadius);
    editDraggerToFeature->setMatrix(dummy);
    editDraggerToFeature->setUserValue(attributeMask, (msg::Request | msg::DraggerToFeature).to_string());
    editDraggerToFeature->setUserValue(attributeStatus, QObject::tr("Dragger To Feature Command").toStdString());
    editSystemBase->insertChild(editSystemBase->getNumChildren() - 2, editDraggerToFeature);
    
    osg::MatrixTransform *preferences = gsn::buildCommandNode(":/resources/images/preferences.svg", iconRadius);
    preferences->setMatrix(dummy);
    preferences->setUserValue(attributeMask, (msg::Request | msg::Preferences).to_string());
    preferences->setUserValue(attributeStatus, QObject::tr("Preferences Command").toStdString());
    editBase->insertChild(editBase->getNumChildren() - 2, preferences);
    
    //file base
    osg::MatrixTransform *fileBase;
    fileBase = gsn::buildMenuNode(":/resources/images/fileBase.svg", iconRadius);
    fileBase->setMatrix(dummy);
    fileBase->setUserValue(attributeStatus, QObject::tr("File Menu").toStdString());
    startNode->insertChild(startNode->getNumChildren() - 2, fileBase);
    
    osg::MatrixTransform *fileImport;
    fileImport = gsn::buildMenuNode(":/resources/images/fileImport.svg", iconRadius);
    fileImport->setMatrix(dummy);
    fileImport->setUserValue(attributeStatus, QObject::tr("Import Menu").toStdString());
    fileBase->insertChild(fileBase->getNumChildren() - 2, fileImport);
    
    osg::MatrixTransform *fileImportOCC = gsn::buildCommandNode(":/resources/images/fileOCC.svg", iconRadius);
    fileImportOCC->setMatrix(dummy);
    fileImportOCC->setUserValue(attributeMask, (msg::Request | msg::ImportOCC).to_string());
    fileImportOCC->setUserValue(attributeStatus, QObject::tr("Import OCC Brep Command").toStdString());
    fileImport->insertChild(fileImport->getNumChildren() - 2, fileImportOCC);
    
    osg::MatrixTransform *fileExport;
    fileExport = gsn::buildMenuNode(":/resources/images/fileExport.svg", iconRadius);
    fileExport->setMatrix(dummy);
    fileExport->setUserValue(attributeStatus, QObject::tr("Export Menu").toStdString());
    fileBase->insertChild(fileBase->getNumChildren() - 2, fileExport);
    
    osg::MatrixTransform *fileExportOSG = gsn::buildCommandNode(":/resources/images/fileOSG.svg", iconRadius);
    fileExportOSG->setMatrix(dummy);
    fileExportOSG->setUserValue(attributeMask, (msg::Request | msg::ExportOSG).to_string());
    fileExportOSG->setUserValue(attributeStatus, QObject::tr("Export Open Scene Graph Command").toStdString());
    fileExport->insertChild(fileExport->getNumChildren() - 2, fileExportOSG);
    
    osg::MatrixTransform *fileExportOCC = gsn::buildCommandNode(":/resources/images/fileOCC.svg", iconRadius);
    fileExportOCC->setMatrix(dummy);
    fileExportOCC->setUserValue(attributeMask, (msg::Request | msg::ExportOCC).to_string());
    fileExportOCC->setUserValue(attributeStatus, QObject::tr("Export OCC Brep Command").toStdString());
    fileExport->insertChild(fileExport->getNumChildren() - 2, fileExportOCC);
    
    osg::MatrixTransform *fileExportStep = gsn::buildCommandNode(":/resources/images/fileStep.svg", iconRadius);
    fileExportStep->setMatrix(dummy);
    fileExportStep->setUserValue(attributeMask, (msg::Request | msg::ExportStep).to_string());
    fileExportStep->setUserValue(attributeStatus, QObject::tr("Export Step Command").toStdString());
    fileExport->insertChild(fileExport->getNumChildren() - 2, fileExportStep);
    
    osg::MatrixTransform *fileOpen = gsn::buildCommandNode(":/resources/images/fileOpen.svg", iconRadius);
    fileOpen->setMatrix(dummy);
    fileOpen->setUserValue(attributeMask, (msg::Request | msg::ProjectDialog).to_string());
    fileOpen->setUserValue(attributeStatus, QObject::tr("Open Project Command").toStdString());
    fileBase->insertChild(fileBase->getNumChildren() - 2, fileOpen);
    
    osg::MatrixTransform *fileSave = gsn::buildCommandNode(":/resources/images/fileSave.svg", iconRadius);
    fileSave->setMatrix(dummy);
    fileSave->setUserValue(attributeMask, (msg::Request | msg::SaveProject).to_string());
    fileSave->setUserValue(attributeStatus, QObject::tr("Save Project Command").toStdString());
    fileBase->insertChild(fileBase->getNumChildren() - 2, fileSave);
    
    //system base
    osg::MatrixTransform *systemBase;
    systemBase = gsn::buildMenuNode(":/resources/images/systemBase.svg", iconRadius);
    systemBase->setMatrix(dummy);
    systemBase->setUserValue(attributeStatus, QObject::tr("Coordinate System Menu").toStdString());
    startNode->insertChild(startNode->getNumChildren() - 2, systemBase);
    
    osg::MatrixTransform *systemReset = gsn::buildCommandNode(":/resources/images/systemReset.svg", iconRadius);
    systemReset->setMatrix(dummy);
    systemReset->setUserValue(attributeMask, (msg::Request | msg::SystemReset).to_string());
    systemReset->setUserValue(attributeStatus, QObject::tr("Coordinate System Reset Command").toStdString());
    systemBase->insertChild(systemBase->getNumChildren() - 2, systemReset);
    
    osg::MatrixTransform *systemToggle = gsn::buildCommandNode(":/resources/images/systemToggle.svg", iconRadius);
    systemToggle->setMatrix(dummy);
    systemToggle->setUserValue(attributeMask, (msg::Request | msg::SystemToggle).to_string());
    systemToggle->setUserValue(attributeStatus, QObject::tr("Toggle Coordinate System Visibility Command").toStdString());
    systemBase->insertChild(systemBase->getNumChildren() - 2, systemToggle);
    
    osg::MatrixTransform *systemToFeature = gsn::buildCommandNode(":/resources/images/systemToFeature.svg", iconRadius);
    systemToFeature->setMatrix(dummy);
    systemToFeature->setUserValue(attributeMask, (msg::Request | msg::SystemToFeature).to_string());
    systemToFeature->setUserValue(attributeStatus, QObject::tr("Coordinate System To Feature Command").toStdString());
    systemBase->insertChild(systemBase->getNumChildren() - 2, systemToFeature);
    
    //inpect base
    osg::MatrixTransform *inspectBase;
    inspectBase = gsn::buildMenuNode(":/resources/images/inspectBase.svg", iconRadius);
    inspectBase->setMatrix(dummy);
    inspectBase->setUserValue(attributeStatus, QObject::tr("Inspect Menu").toStdString());
    startNode->insertChild(startNode->getNumChildren() - 2, inspectBase);
    
    osg::MatrixTransform *inpsectInfo = gsn::buildCommandNode(":/resources/images/inspectInfo.svg", iconRadius);
    inpsectInfo->setMatrix(dummy);
    inpsectInfo->setUserValue(attributeMask, (msg::Request | msg::ViewInfo).to_string());
    inpsectInfo->setUserValue(attributeStatus, QObject::tr("View Info Command").toStdString());
    inspectBase->insertChild(inspectBase->getNumChildren() - 2, inpsectInfo);
    
    osg::MatrixTransform *inspectCheckGeometry = gsn::buildCommandNode(":/resources/images/inspectCheckGeometry.svg", iconRadius);
    inspectCheckGeometry->setMatrix(dummy);
    inspectCheckGeometry->setUserValue(attributeMask, (msg::Request | msg::CheckGeometry).to_string());
    inspectCheckGeometry->setUserValue(attributeStatus, QObject::tr("Check Geometry For Errors").toStdString());
    inspectBase->insertChild(inspectBase->getNumChildren() - 2, inspectCheckGeometry);
    
    //inspect measure base
    osg::MatrixTransform *inspectMeasureBase;
    inspectMeasureBase = gsn::buildMenuNode(":/resources/images/inspectMeasureBase.svg", iconRadius);
    inspectMeasureBase->setMatrix(dummy);
    inspectMeasureBase->setUserValue(attributeStatus, QObject::tr("Measure Menu").toStdString());
    inspectBase->insertChild(inspectBase->getNumChildren() - 2, inspectMeasureBase);
    
    osg::MatrixTransform *inspectMeasureClear = gsn::buildCommandNode(":/resources/images/inspectMeasureClear.svg", iconRadius);
    inspectMeasureClear->setMatrix(dummy);
    inspectMeasureClear->setUserValue(attributeMask, (msg::Request | msg::Clear | msg::Overlay).to_string());
    inspectMeasureClear->setUserValue(attributeStatus, QObject::tr("Clear Measure Command").toStdString());
    inspectMeasureBase->insertChild(inspectMeasureBase->getNumChildren() - 2, inspectMeasureClear);
    
    osg::MatrixTransform *inspectLinearMeasure = gsn::buildCommandNode(":/resources/images/inspectLinearMeasure.svg", iconRadius);
    inspectLinearMeasure->setMatrix(dummy);
    inspectLinearMeasure->setUserValue(attributeMask, (msg::Request | msg::LinearMeasure).to_string());
    inspectLinearMeasure->setUserValue(attributeStatus, QObject::tr("LinearMeasure Command").toStdString());
    inspectMeasureBase->insertChild(inspectMeasureBase->getNumChildren() - 2, inspectLinearMeasure);
    
    //debug base
    osg::MatrixTransform *debugBase;
    debugBase = gsn::buildMenuNode(":/resources/images/debugBase.svg", iconRadius);
    debugBase->setMatrix(dummy);
    debugBase->setUserValue(attributeStatus, QObject::tr("Debug Menu").toStdString());
    inspectBase->insertChild(inspectBase->getNumChildren() - 2, debugBase);
    
    osg::MatrixTransform *checkShapeIds = gsn::buildCommandNode(":/resources/images/debugCheckShapeIds.svg", iconRadius);
    checkShapeIds->setMatrix(dummy);
    checkShapeIds->setUserValue(attributeMask, (msg::Request | msg::CheckShapeIds).to_string());
    checkShapeIds->setUserValue(attributeStatus, QObject::tr("Check Shaped Ids").toStdString());
    debugBase->insertChild(debugBase->getNumChildren() - 2, checkShapeIds);
    
    osg::MatrixTransform *debugDump = gsn::buildCommandNode(":/resources/images/debugDump.svg", iconRadius);
    debugDump->setMatrix(dummy);
    debugDump->setUserValue(attributeMask, (msg::Request | msg::DebugDump).to_string());
    debugDump->setUserValue(attributeStatus, QObject::tr("Debug Feature Dump").toStdString());
    debugBase->insertChild(debugBase->getNumChildren() - 2, debugDump);
    
    osg::MatrixTransform *debugShapeTrackUp = gsn::buildCommandNode(":/resources/images/debugShapeTrackUp.svg", iconRadius);
    debugShapeTrackUp->setMatrix(dummy);
    debugShapeTrackUp->setUserValue(attributeMask, (msg::Request | msg::DebugShapeTrackUp).to_string());
    debugShapeTrackUp->setUserValue(attributeStatus, QObject::tr("Track Shape Up").toStdString());
    debugBase->insertChild(debugBase->getNumChildren() - 2, debugShapeTrackUp);
    
    osg::MatrixTransform *debugShapeTrackDown = gsn::buildCommandNode(":/resources/images/debugShapeTrackDown.svg", iconRadius);
    debugShapeTrackDown->setMatrix(dummy);
    debugShapeTrackDown->setUserValue(attributeMask, (msg::Request | msg::DebugShapeTrackDown).to_string());
    debugShapeTrackDown->setUserValue(attributeStatus, QObject::tr("Track Shape Down").toStdString());
    debugBase->insertChild(debugBase->getNumChildren() - 2, debugShapeTrackDown);
    
    osg::MatrixTransform *debugShapeGraph = gsn::buildCommandNode(":/resources/images/debugShapeGraph.svg", iconRadius);
    debugShapeGraph->setMatrix(dummy);
    debugShapeGraph->setUserValue(attributeMask, (msg::Request | msg::DebugShapeGraph).to_string());
    debugShapeGraph->setUserValue(attributeStatus, QObject::tr("Write shape graph to application directory").toStdString());
    debugBase->insertChild(debugBase->getNumChildren() - 2, debugShapeGraph);
    
    osg::MatrixTransform *debugDumpProjectGraph = gsn::buildCommandNode(":/resources/images/debugDumpProjectGraph.svg", iconRadius);
    debugDumpProjectGraph->setMatrix(dummy);
    debugDumpProjectGraph->setUserValue(attributeMask, (msg::Request | msg::DebugDumpProjectGraph).to_string());
    debugDumpProjectGraph->setUserValue(attributeStatus, QObject::tr("Write project graph to application directory").toStdString());
    debugBase->insertChild(debugBase->getNumChildren() - 2, debugDumpProjectGraph);
    
    osg::MatrixTransform *debugDumpDAGViewGraph = gsn::buildCommandNode(":/resources/images/debugDumpDAGViewGraph.svg", iconRadius);
    debugDumpDAGViewGraph->setMatrix(dummy);
    debugDumpDAGViewGraph->setUserValue(attributeMask, (msg::Request | msg::DebugDumpDAGViewGraph).to_string());
    debugDumpDAGViewGraph->setUserValue(attributeStatus, QObject::tr("Write DAGView graph to application directory").toStdString());
    debugBase->insertChild(debugBase->getNumChildren() - 2, debugDumpDAGViewGraph);
    
    osg::MatrixTransform *debugInquiry = gsn::buildCommandNode(":/resources/images/debugInquiry.svg", iconRadius);
    debugInquiry->setMatrix(dummy);
    debugInquiry->setUserValue(attributeMask, (msg::Request | msg::DebugInquiry).to_string());
    debugInquiry->setUserValue(attributeStatus, QObject::tr("Inquiry. A testing facility").toStdString());
    debugBase->insertChild(debugBase->getNumChildren() - 2, debugInquiry);
}

std::vector<osg::Vec3> GestureHandler::buildNodeLocations(osg::Vec3 direction, int nodeCount)
{
    double sprayRadius = calculateSprayRadius(nodeCount);
    
    osg::Vec3 point = direction;
    point.normalize();
    point *= sprayRadius;

    double localIncludedAngle = includedAngle;
    if (sprayRadius == minimumSprayRadius)
    {
        //now we limit the angle to get node separation.
        double singleAngle = osg::RadiansToDegrees(2 * asin(nodeSpread / (sprayRadius * 2)));
        localIncludedAngle = singleAngle * (nodeCount -1);
    }

    double incrementAngle = localIncludedAngle / (nodeCount - 1);
    double startAngle = localIncludedAngle / 2.0;
    if (nodeCount < 2)
        startAngle = 0;
    //I am missing something why are the following 2 vectors are opposite of what I would expect?
    osg::Matrixd startRotation = osg::Matrixd::rotate(osg::DegreesToRadians(startAngle), osg::Vec3d(0.0, 0.0, -1.0));
    osg::Matrixd incrementRotation = osg::Matrixd::rotate(osg::DegreesToRadians(incrementAngle), osg::Vec3d(0.0, 0.0, 1.0));
    point = (startRotation * point);
    std::vector<osg::Vec3> pointArray;
    pointArray.push_back(point);
    for (int index = 0; index < nodeCount - 1; index++)
    {
        point = (incrementRotation * point);
        pointArray.push_back(point);
    }
    return pointArray;
}

double GestureHandler::calculateSprayRadius(int nodeCount)
{
    double segmentCount = nodeCount - 1;
    if (segmentCount < 1)
        return minimumSprayRadius;
    
    double angle = 0.0;
    double includedAngleRadians = osg::DegreesToRadians(includedAngle);
    
    //try to use minimum spray radius and check against include angle.
    angle = std::asin(nodeSpread / 2.0 / minimumSprayRadius) * 2.0;
    if ((angle * segmentCount) < includedAngleRadians)
      return minimumSprayRadius;
    
    //that didn't work so calculate angle and use to determin spray radius for node spread.
    double halfAngle = includedAngleRadians / segmentCount / 2.0;
    double hypt = nodeSpread / 2.0 / std::sin(halfAngle);
    return hypt;
}

void GestureHandler::startDrag(const osgGA::GUIEventAdapter& eventAdapter)
{
    //send status
    observer->out(msg::buildStatusMessage(QObject::tr("Start Menu").toStdString()));
  
    gestureSwitch->setAllChildrenOn();
    osg::Switch *startSwitch = dynamic_cast<osg::Switch *>(startNode->getChild(startNode->getNumChildren() - 1));
    assert(startSwitch);
    startSwitch->setAllChildrenOn();
    currentNode = startNode;
    currentNodeLeft = false;

    osg::Matrixd projection = gestureCamera->getProjectionMatrix();
    osg::Matrixd window = gestureCamera->getViewport()->computeWindowMatrix();
    osg::Matrixd transformation = projection * window;
    transformation = osg::Matrixd::inverse(transformation);

    osg::Vec3 position(osg::Vec3d(eventAdapter.getX(), eventAdapter.getY(), 0.0));
    position = transformation * position;

    osg::Matrixd locationMatrix = osg::Matrixd::translate(position);
    osg::Matrixd scaleMatrix = osg::Matrixd::scale(transformation.getScale());
    startNode->setMatrix(scaleMatrix * locationMatrix);

    aggregateMatrix = startNode->getMatrix();
}

void GestureHandler::setupDispatcher()
{
  msg::Mask mask;
  
  mask = msg::Response | msg::Preferences;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&GestureHandler::preferencesChangedDispatched, this, _1)));
}

void GestureHandler::updateVariables()
{
  iconRadius = prf::manager().rootPtr->gesture().iconRadius();
  iconRadius = std::max(iconRadius, 16.0);
  iconRadius = std::min(iconRadius, 128.0);
  
  includedAngle = static_cast<double>(prf::manager().rootPtr->gesture().includeAngle());
  includedAngle = std::max(includedAngle, 15.0);
  includedAngle = std::min(includedAngle, 360.0);
  
  double iconDiameter = iconRadius * 2.0;
  nodeSpread = iconDiameter + iconDiameter * prf::manager().rootPtr->gesture().spreadFactor();
  minimumSprayRadius = iconDiameter + iconDiameter * prf::manager().rootPtr->gesture().sprayFactor();
}

void GestureHandler::preferencesChangedDispatched(const msg::Message&)
{
  
  if (iconRadius != prf::manager().rootPtr->gesture().iconRadius())
  {
    updateVariables();
    constructMenu();
  }
  else
    updateVariables();
}

GestureAllSwitchesOffVisitor::GestureAllSwitchesOffVisitor() :
    osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
{
}

void GestureAllSwitchesOffVisitor::apply(osg::Switch &aSwitch)
{
    traverse(aSwitch);
    aSwitch.setAllChildrenOff();
}

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
    dragStarted(false), currentNodeLeft(false), iconRadius(32.0), includedAngle(90.0)
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
    const vwr::SpaceballOSGEvent *event = 
      dynamic_cast<const vwr::SpaceballOSGEvent *>(eventAdapter.getUserData());
    assert (event);
    
    if (event->theType == vwr::SpaceballOSGEvent::Button)
    {
      int currentButton = event->buttonNumber;
      vwr::SpaceballOSGEvent::ButtonState currentState = event->theButtonState;
      if (currentState == vwr::SpaceballOSGEvent::Pressed)
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
        assert(currentState == vwr::SpaceballOSGEvent::Released);
        spaceballButton = -1;
      }
      return true;
    }
  }
  
  if (eventAdapter.getEventType() == osgGA::GUIEventAdapter::KEYDOWN)
  {
    if (!rightButtonDown)
      return false;
    hotKey = eventAdapter.getKey();
    if (dragStarted)
    {
      std::ostringstream stream;
      stream << QObject::tr("Link to key ").toStdString() << hotKey;
      observer->out(msg::buildStatusMessage(stream.str()));
    }
    else
    {
      std::string maskString = prf::manager().getHotKey(hotKey);
      if (!maskString.empty())
      {
        msg::Mask msgMask(maskString);
        msg::Message messageOut;
        messageOut.mask = msgMask;
        observer->out(messageOut);
      }
    }
    return true;
  }
  if (eventAdapter.getEventType() == osgGA::GUIEventAdapter::KEYUP)
  {
    if (!rightButtonDown)
      return false;
    hotKey = -1;
    observer->out(msg::buildStatusMessage(""));
    return true;
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
    {
      std::cout << "gestureSwitch is invalid in GestureHandler::handle" << std::endl;
      return false;
    }

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
            if (currentNode && (currentNode->getNodeMask() & mdv::gestureCommand))
            {
                std::string msgMaskString;
                if (currentNode->getUserValue(attributeMask, msgMaskString))
                {
                    if (spaceballButton != -1)
                    {
                      prf::manager().setSpaceballButton(spaceballButton, msgMaskString);
                      prf::manager().saveConfig();
                    }
                    if (hotKey != -1)
                    {
                      prf::manager().setHotKey(hotKey, msgMaskString);
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
          if (currentNode && (currentNode->getNodeMask() & mdv::gestureMenu))
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
  startNode->getOrCreateStateSet()->setAttributeAndModes(new osg::Depth(osg::Depth::LEQUAL, 0.0, 0.0)); //????
  gestureSwitch->insertChild(0, startNode);

  osg::Matrixd dummy = osg::Matrixd::identity();
  
  auto constructMenuNode = [&]
  (
    const char *resource,
    const std::string &statusText,
    osg::MatrixTransform *parent
  ) -> osg::MatrixTransform*
  {
    assert(!statusText.empty());
    assert(parent);
    
    osg::MatrixTransform *out;
    out = gsn::buildMenuNode(resource, iconRadius);
    out->setMatrix(osg::Matrixd::identity());
    out->setUserValue(attributeStatus, statusText);
    parent->insertChild(parent->getNumChildren() - 2, out);
    
    return out;
  };
  
  auto constructCommandNode = [&]
  (
    const char *resource,
    const std::string &statusText,
    const std::string &mask,
    osg::MatrixTransform *parent
  ) -> osg::MatrixTransform*
  {
    assert(!statusText.empty());
    assert(!mask.empty());
    assert(parent);
    
    osg::MatrixTransform *out;
    out = gsn::buildCommandNode(resource, iconRadius);
    out->setMatrix(osg::Matrixd::identity());
    out->setUserValue(attributeStatus, statusText);
    out->setUserValue(attributeMask, mask);
    parent->insertChild(parent->getNumChildren() - 2, out);
    
    return out;
  };
  
  //using braces and indents to show hierarchy of menu

  osg::MatrixTransform *viewBase = constructMenuNode
  (
    ":/resources/images/viewBase.svg",
    QObject::tr("View Menu").toStdString(),
    startNode.get()
  );
  {
    osg::MatrixTransform *viewStandard = constructMenuNode
    (
      ":/resources/images/viewStandard.svg",
      QObject::tr("Standard Views Menu").toStdString(),
      viewBase
    );
    {
      constructCommandNode
      (
        ":/resources/images/viewTop.svg",
        QObject::tr("Top View Command").toStdString(),
        (msg::Request | msg::View | msg::Top).to_string(),
        viewStandard
      );
      constructCommandNode
      (
        ":/resources/images/viewFront.svg",
        QObject::tr("View Front Command").toStdString(),
        (msg::Request | msg::View | msg::Front).to_string(),
        viewStandard
      );
      constructCommandNode
      (
        ":/resources/images/viewRight.svg",
        QObject::tr("View Right Command").toStdString(),
        (msg::Request | msg::View | msg::Right).to_string(),
        viewStandard
      );
      constructCommandNode
      (
        ":/resources/images/viewIso.svg",
        QObject::tr("View Iso Command").toStdString(),
        (msg::Request | msg::View | msg::Iso).to_string(),
        viewStandard
      );
    }
    osg::MatrixTransform *viewStandardCurrent = constructMenuNode
    (
      ":/resources/images/viewStandardCurrent.svg",
      QObject::tr("Standard Views Menu Relative To Current System").toStdString(),
      viewBase
    );
    {
      constructCommandNode
      (
        ":/resources/images/viewTopCurrent.svg",
        QObject::tr("Top View Current Command").toStdString(),
        (msg::Request | msg::View | msg::Top | msg::Current).to_string(),
        viewStandardCurrent
      );
      constructCommandNode
      (
        ":/resources/images/viewFrontCurrent.svg",
        QObject::tr("View Front Current Command").toStdString(),
        (msg::Request | msg::View | msg::Front | msg::Current).to_string(),
        viewStandardCurrent
      );
      constructCommandNode
      (
        ":/resources/images/viewRightCurrent.svg",
        QObject::tr("View Right Current Command").toStdString(),
        (msg::Request | msg::View | msg::Right | msg::Current).to_string(),
        viewStandardCurrent
      );
    }
    //back to viewbase menu
    constructCommandNode
    (
      ":/resources/images/viewFit.svg",
      QObject::tr("View Fit Command").toStdString(),
      (msg::Request | msg::View | msg::Fit).to_string(),
      viewBase
    );
    constructCommandNode
    (
      ":/resources/images/viewFill.svg",
      QObject::tr("View Fill Command").toStdString(),
      (msg::Request | msg::View | msg::Fill).to_string(),
      viewBase
    );
    constructCommandNode
    (
      ":/resources/images/viewTriangulation.svg",
      QObject::tr("View Triangulation Command").toStdString(),
      (msg::Request | msg::View | msg::Triangulation).to_string(),
      viewBase
    );
    constructCommandNode
    (
      ":/resources/images/renderStyleToggle.svg",
      QObject::tr("Toggle Render Style Command").toStdString(),
      (msg::Request | msg::View | msg::RenderStyle | msg::Toggle).to_string(),
      viewBase
    );
    constructCommandNode
    (
      ":/resources/images/viewHiddenLines.svg",
      QObject::tr("Toggle hidden lines Command").toStdString(),
      (msg::Request | msg::View | msg::Toggle | msg::HiddenLine).to_string(),
      viewBase
    );
    constructCommandNode
    (
      ":/resources/images/viewIsolate.svg",
      QObject::tr("View Only Selected").toStdString(),
      (msg::Request | msg::View | msg::ThreeD | msg::Overlay | msg::Isolate).to_string(),
      viewBase
    );
  }
  osg::MatrixTransform *constructionBase = constructMenuNode
  (
    ":/resources/images/constructionBase.svg",
    QObject::tr("Construction Menu").toStdString(),
    startNode.get()
  );
  {
    osg::MatrixTransform *constructionPrimitives = constructMenuNode
    (
      ":/resources/images/constructionPrimitives.svg",
      QObject::tr("Primitives Menu").toStdString(),
      constructionBase
    );
    {
      constructCommandNode
      (
        ":/resources/images/constructionBox.svg",
        QObject::tr("Box Command").toStdString(),
        (msg::Request | msg::Construct | msg::Box).to_string(),
        constructionPrimitives
      );
      constructCommandNode
      (
        ":/resources/images/constructionSphere.svg",
        QObject::tr("Sphere Command").toStdString(),
        (msg::Request | msg::Construct | msg::Sphere).to_string(),
        constructionPrimitives
      );
      constructCommandNode
      (
        ":/resources/images/constructionCone.svg",
        QObject::tr("Cone Command").toStdString(),
        (msg::Request | msg::Construct | msg::Cone).to_string(),
        constructionPrimitives
      );
      constructCommandNode
      (
        ":/resources/images/constructionCylinder.svg",
        QObject::tr("Cylinder Command").toStdString(),
        (msg::Request | msg::Construct | msg::Cylinder).to_string(),
        constructionPrimitives
      );
      constructCommandNode
      (
        ":/resources/images/constructionOblong.svg",
        QObject::tr("Oblong Command").toStdString(),
        (msg::Request | msg::Construct | msg::Oblong).to_string(),
        constructionPrimitives
      );
    }
    osg::MatrixTransform *constructionFinishing = constructMenuNode
    (
      ":/resources/images/constructionFinishing.svg",
      QObject::tr("Finishing Menu").toStdString(),
      constructionBase
    );
    {
      constructCommandNode
      (
        ":/resources/images/constructionBlend.svg",
        QObject::tr("Blend Command").toStdString(),
        (msg::Request | msg::Construct | msg::Blend).to_string(),
        constructionFinishing
      );
      constructCommandNode
      (
        ":/resources/images/constructionChamfer.svg",
        QObject::tr("Chamfer Command").toStdString(),
        (msg::Request | msg::Construct | msg::Chamfer).to_string(),
        constructionFinishing
      );
      constructCommandNode
      (
        ":/resources/images/constructionDraft.svg",
        QObject::tr("Draft Command").toStdString(),
        (msg::Request | msg::Construct | msg::Draft).to_string(),
        constructionFinishing
      );
      constructCommandNode
      (
        ":/resources/images/constructionHollow.svg",
        QObject::tr("Hollow Command").toStdString(),
        (msg::Request | msg::Construct | msg::Hollow).to_string(),
        constructionFinishing
      );
      constructCommandNode
      (
        ":/resources/images/constructionExtract.svg",
        QObject::tr("Extract Command").toStdString(),
        (msg::Request | msg::Construct | msg::Extract).to_string(),
        constructionFinishing
      );
      constructCommandNode
      (
        ":/resources/images/constructionRefine.svg",
        QObject::tr("Refine Command").toStdString(),
        (msg::Request | msg::Construct | msg::Refine).to_string(),
        constructionFinishing
      );
    }
    osg::MatrixTransform *constructionBoolean = constructMenuNode
    (
      ":/resources/images/constructionBoolean.svg",
      QObject::tr("Boolean Menu").toStdString(),
      constructionBase
    );
    {
      constructCommandNode
      (
        ":/resources/images/constructionUnion.svg",
        QObject::tr("Union Command").toStdString(),
        (msg::Request | msg::Construct | msg::Union).to_string(),
        constructionBoolean
      );
      constructCommandNode
      (
        ":/resources/images/constructionSubtract.svg",
        QObject::tr("Subtract Command").toStdString(),
        (msg::Request | msg::Construct | msg::Subtract).to_string(),
        constructionBoolean
      );
      constructCommandNode
      (
        ":/resources/images/constructionIntersect.svg",
        QObject::tr("Intersection Command").toStdString(),
        (msg::Request | msg::Construct | msg::Intersect).to_string(),
        constructionBoolean
      );
    }
    osg::MatrixTransform *constructionDie = constructMenuNode
    (
      ":/resources/images/constructionDie.svg",
      QObject::tr("Die Menu").toStdString(),
      constructionBase
    );
    {
      constructCommandNode
      (
        ":/resources/images/constructionSquash.svg",
        QObject::tr("Squash Command").toStdString(),
        (msg::Request | msg::Construct | msg::Squash).to_string(),
        constructionDie
      );
      constructCommandNode
      (
        ":/resources/images/constructionStrip.svg",
        QObject::tr("Strip Command").toStdString(),
        (msg::Request | msg::Construct | msg::Strip).to_string(),
        constructionDie
      );
      constructCommandNode
      (
        ":/resources/images/constructionNest.svg",
        QObject::tr("Nest Command").toStdString(),
        (msg::Request | msg::Construct | msg::Nest).to_string(),
        constructionDie
      );
      constructCommandNode
      (
        ":/resources/images/constructionDieSet.svg",
        QObject::tr("DieSet Command").toStdString(),
        (msg::Request | msg::Construct | msg::DieSet).to_string(),
        constructionDie
      );
      constructCommandNode
      (
        ":/resources/images/constructionQuote.svg",
        QObject::tr("Quote Command").toStdString(),
        (msg::Request | msg::Construct | msg::Quote).to_string(),
        constructionDie
      );
    }
    constructCommandNode //TODO build datum menu node and move datum plane to it.
    (
      ":/resources/images/constructionDatumPlane.svg",
      QObject::tr("Datum Plane Command").toStdString(),
      (msg::Request | msg::Construct | msg::DatumPlane).to_string(),
      constructionBase
    );
    osg::MatrixTransform *constructionInstance = constructMenuNode
    (
      ":/resources/images/constructionInstance.svg",
      QObject::tr("Instance Menu").toStdString(),
      constructionBase
    );
    {
      constructCommandNode
      (
        ":/resources/images/constructionInstanceLinear.svg",
        QObject::tr("Instance Linear Command").toStdString(),
        (msg::Request | msg::Construct | msg::InstanceLinear).to_string(),
        constructionInstance
      );
    }
  }
  osg::MatrixTransform *editBase = constructMenuNode
  (
    ":/resources/images/editBase.svg",
    QObject::tr("Edit Menu").toStdString(),
    startNode.get()
  );
  {
    constructCommandNode
    (
      ":/resources/images/editCommandCancel.svg",
      QObject::tr("Cancel Command").toStdString(),
      (msg::Request | msg::Command | msg::Cancel).to_string(),
      editBase
    );
    constructCommandNode
    (
      ":/resources/images/editFeature.svg",
      QObject::tr("Edit Feature Command").toStdString(),
      (msg::Request | msg::Edit | msg::Feature).to_string(),
      editBase
    );
    constructCommandNode
    (
      ":/resources/images/editRemove.svg",
      QObject::tr("Remove Command").toStdString(),
      (msg::Request | msg::Remove).to_string(),
      editBase
    );
    constructCommandNode
    (
      ":/resources/images/editColor.svg",
      QObject::tr("Edit Color Command").toStdString(),
      (msg::Request | msg::Edit | msg::Feature | msg::Color).to_string(),
      editBase
    );
    constructCommandNode
    (
      ":/resources/images/editRename.svg",
      QObject::tr("Edit Name Command").toStdString(),
      (msg::Request | msg::Edit | msg::Feature | msg::Name).to_string(),
      editBase
    );
    constructCommandNode
    (
      ":/resources/images/editUpdate.svg",
      QObject::tr("Update Command").toStdString(),
      (msg::Request | msg::Project | msg::Update).to_string(),
      editBase
    );
    constructCommandNode
    (
      ":/resources/images/editForceUpdate.svg",
      QObject::tr("Force Update Command").toStdString(),
      (msg::Request | msg::Force | msg::Update).to_string(),
      editBase
    );
    osg::MatrixTransform *editSystemBase = constructMenuNode
    (
      ":/resources/images/systemBase.svg",
      QObject::tr("Coordinate System Menu").toStdString(),
      editBase
    );
    {
      constructCommandNode
      (
        ":/resources/images/editDraggerToFeature.svg",
        QObject::tr("Dragger To Feature Command").toStdString(),
        (msg::Request | msg::DraggerToFeature).to_string(),
        editSystemBase
      );
      constructCommandNode
      (
        ":/resources/images/editFeatureToDragger.svg",
        QObject::tr("Feature To Dragger Command").toStdString(),
        (msg::Request | msg::FeatureToDragger).to_string(),
        editSystemBase
      );
      constructCommandNode
      (
        ":/resources/images/featureToSystem.svg",
        QObject::tr("Feature To Current System Command").toStdString(),
        (msg::Request | msg::FeatureToSystem).to_string(),
        editSystemBase
      );
      constructCommandNode
      (
        ":/resources/images/featureReposition.svg",
        QObject::tr("Dragger To Current System Command").toStdString(),
        (msg::Request | msg::FeatureReposition).to_string(),
        editSystemBase
      );
    }
    constructCommandNode
    (
      ":/resources/images/preferences.svg",
      QObject::tr("Preferences Command").toStdString(),
      (msg::Request | msg::Preferences).to_string(),
      editBase
    );
  }
  osg::MatrixTransform *fileBase = constructMenuNode
  (
    ":/resources/images/fileBase.svg",
    QObject::tr("File Menu").toStdString(),
    startNode.get()
  );
  {
    osg::MatrixTransform *fileImport = constructMenuNode
    (
      ":/resources/images/fileImport.svg",
      QObject::tr("Import Menu").toStdString(),
      fileBase
    );
    {
      constructCommandNode
      (
        ":/resources/images/fileOCC.svg",
        QObject::tr("Import OCC Brep Command").toStdString(),
        (msg::Request | msg::Import | msg::OCC).to_string(),
        fileImport
      );
      constructCommandNode
      (
        ":/resources/images/fileStep.svg",
        QObject::tr("Import Step Command").toStdString(),
        (msg::Request | msg::Import | msg::Step).to_string(),
        fileImport
      );
    }
    osg::MatrixTransform *fileExport = constructMenuNode
    (
      ":/resources/images/fileExport.svg",
      QObject::tr("Export Menu").toStdString(),
      fileBase
    );
    {
      constructCommandNode
      (
        ":/resources/images/fileOSG.svg",
        QObject::tr("Export Open Scene Graph Command").toStdString(),
        (msg::Request | msg::Export | msg::OSG).to_string(),
        fileExport
      );
      constructCommandNode
      (
        ":/resources/images/fileOCC.svg",
        QObject::tr("Export OCC Brep Command").toStdString(),
        (msg::Request | msg::Export | msg::OCC).to_string(),
        fileExport
      );
      constructCommandNode
      (
        ":/resources/images/fileStep.svg",
        QObject::tr("Export Step Command").toStdString(),
        (msg::Request | msg::Export | msg::Step).to_string(),
        fileExport
      );
    }
    constructCommandNode
    (
      ":/resources/images/fileOpen.svg",
      QObject::tr("Open Project Command").toStdString(),
      (msg::Request | msg::Project | msg::Dialog).to_string(),
      fileBase
    );
    constructCommandNode
    (
      ":/resources/images/fileSave.svg",
      QObject::tr("Save Project Command").toStdString(),
      (msg::Request | msg::Save | msg::Project).to_string(),
      fileBase
    );
  }
  osg::MatrixTransform *systemBase = constructMenuNode
  (
    ":/resources/images/systemBase.svg",
    QObject::tr("Coordinate System Menu").toStdString(),
    startNode.get()
  );
  {
    constructCommandNode
    (
      ":/resources/images/systemReset.svg",
      QObject::tr("Coordinate System Reset Command").toStdString(),
      (msg::Request | msg::SystemReset).to_string(),
      systemBase
    );
    constructCommandNode
    (
      ":/resources/images/systemToggle.svg",
      QObject::tr("Toggle Coordinate System Visibility Command").toStdString(),
      (msg::Request | msg::SystemToggle).to_string(),
      systemBase
    );
    constructCommandNode
    (
      ":/resources/images/systemToFeature.svg",
      QObject::tr("Coordinate System To Feature Command").toStdString(),
      (msg::Request | msg::SystemToFeature).to_string(),
      systemBase
    );
  }
  osg::MatrixTransform *inspectBase = constructMenuNode
  (
    ":/resources/images/inspectBase.svg",
    QObject::tr("Inspect Menu").toStdString(),
    startNode.get()
  );
  {
    constructCommandNode
    (
      ":/resources/images/inspectInfo.svg",
      QObject::tr("View Info Command").toStdString(),
      (msg::Request | msg::Info).to_string(),
      inspectBase
    );
    constructCommandNode
    (
      ":/resources/images/inspectCheckGeometry.svg",
      QObject::tr("Check Geometry For Errors").toStdString(),
      (msg::Request | msg::CheckGeometry).to_string(),
      inspectBase
    );
    osg::MatrixTransform *inspectMeasureBase = constructMenuNode
    (
      ":/resources/images/inspectMeasureBase.svg",
      QObject::tr("Measure Menu").toStdString(),
      inspectBase
    );
    {
      constructCommandNode
      (
        ":/resources/images/inspectMeasureClear.svg",
        QObject::tr("Clear Measure Command").toStdString(),
        (msg::Request | msg::Clear | msg::Overlay).to_string(),
        inspectMeasureBase
      );
      constructCommandNode
      (
        ":/resources/images/inspectLinearMeasure.svg",
        QObject::tr("LinearMeasure Command").toStdString(),
        (msg::Request | msg::LinearMeasure).to_string(),
        inspectMeasureBase
      );
    }
    osg::MatrixTransform *debugBase = constructMenuNode
    (
      ":/resources/images/debugBase.svg",
      QObject::tr("Debug Menu").toStdString(),
      inspectBase
    );
    {
      constructCommandNode
      (
        ":/resources/images/debugCheckShapeIds.svg",
        QObject::tr("Check Shaped Ids").toStdString(),
        (msg::Request | msg::CheckShapeIds).to_string(),
        debugBase
      );
      constructCommandNode
      (
        ":/resources/images/debugDump.svg",
        QObject::tr("Debug Feature Dump").toStdString(),
        (msg::Request | msg::DebugDump).to_string(),
        debugBase
      );
      constructCommandNode
      (
        ":/resources/images/debugShapeTrackUp.svg",
        QObject::tr("Track Shape Up").toStdString(),
        (msg::Request | msg::DebugShapeTrackUp).to_string(),
        debugBase
      );
      constructCommandNode
      (
        ":/resources/images/debugShapeTrackDown.svg",
        QObject::tr("Track Shape Down").toStdString(),
        (msg::Request | msg::DebugShapeTrackDown).to_string(),
        debugBase
      );
      constructCommandNode
      (
        ":/resources/images/debugShapeGraph.svg",
        QObject::tr("Write shape graph to application directory").toStdString(),
        (msg::Request | msg::DebugShapeGraph).to_string(),
        debugBase
      );
      constructCommandNode
      (
        ":/resources/images/debugDumpProjectGraph.svg",
        QObject::tr("Write project graph to application directory").toStdString(),
        (msg::Request | msg::DebugDumpProjectGraph).to_string(),
        debugBase
      );
      constructCommandNode
      (
        ":/resources/images/debugDumpDAGViewGraph.svg",
        QObject::tr("Write DAGView graph to application directory").toStdString(),
        (msg::Request | msg::DebugDumpDAGViewGraph).to_string(),
        debugBase
      );
      constructCommandNode
      (
        ":/resources/images/debugInquiry.svg",
        QObject::tr("Inquiry. A testing facility").toStdString(),
        (msg::Request | msg::DebugInquiry).to_string(),
        debugBase
      );
      constructCommandNode
      (
        ":/resources/images/dagViewPending.svg",
        QObject::tr("Dirty selected features").toStdString(),
        (msg::Request | msg::Feature | msg::Model | msg::Dirty).to_string(),
        debugBase
      );
    }
  }
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

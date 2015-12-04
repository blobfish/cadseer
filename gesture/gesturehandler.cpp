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

#include <osgViewer/View>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgUtil/LineSegmentIntersector>
#include <osg/ValueObject>

#include <gesture/gesturenode.h>
#include <gesture/gesturehandler.h>
#include <nodemaskdefs.h>
#include <message/dispatch.h>

static const std::string attributeTitle = "CommandAttributeTitle";

GestureHandler::GestureHandler(osg::Camera *cameraIn) : osgGA::GUIEventHandler(), rightButtonDown(false),
    currentNodeLeft(false), iconRadius(32.0), includedAngle(90.0)
{
    gestureCamera = cameraIn;
    if (!gestureCamera.valid())
        return;
    gestureSwitch = dynamic_cast<osg::Switch *>(gestureCamera->getChild(0));
    if (!gestureSwitch.valid())
        return;

    mininumSprayRadius = iconRadius * 7.0;
    nodeSpread = iconRadius * 3.0;
    constructMenu();
    
    setupDispatcher();
    this->connectMessageOut(boost::bind(&msg::Dispatch::messageInSlot, &msg::dispatch(), _1));
    msg::dispatch().connectMessageOut(boost::bind(&GestureHandler::messageInSlot, this, _1));
}

bool GestureHandler::handle(const osgGA::GUIEventAdapter& eventAdapter,
                            osgGA::GUIActionAdapter& actionAdapter, osg::Object *object,
                            osg::NodeVisitor *nodeVistor)
{
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
            rightButtonDown = false;
            if (!dragStarted)
                return false;
            dragStarted = false;
            gestureSwitch->setAllChildrenOff();
            if (currentNode->getNodeMask() & NodeMaskDef::gestureCommand)
            {
	      std::string msgMaskString;
	      if (currentNode->getUserValue(attributeTitle, msgMaskString))
	      {
		msg::Mask msgMask(msgMaskString);
		msg::Message messageOut;
		messageOut.mask = msgMask;
		messageOutSignal(messageOut);
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
        iv.setTraversalMask(~NodeMaskDef::gestureCamera);
        gestureSwitch->accept(iv);

        if(intersector->containsIntersections())
        {
            osg::ref_ptr<osg::MatrixTransform> node = dynamic_cast<osg::MatrixTransform*>
                    (intersector->getFirstIntersection().drawable->getParent(0)->getParent(0)->getParent(0));
            assert(node.valid());
            lastHitPoint = intersector->getFirstIntersection().getLocalIntersectPoint();
            if (node == currentNode)
            {
                if (currentNodeLeft == true)
                {
                    currentNodeLeft = false;
                    if (currentNode->getNodeMask() & NodeMaskDef::gestureMenu)
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
        else
        {
            if (currentNodeLeft == false)
            {
                currentNodeLeft = true;
                if (currentNode->getNodeMask() & NodeMaskDef::gestureMenu)
                    spraySubNodes(temp);
            }
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
    std::vector<osg::Vec3> locations = buildNodeLocations(direction, childCount - 2);
    for (unsigned int index = 0; index < locations.size(); ++index)
    {
        osg::MatrixTransform *tempLocation = dynamic_cast<osg::MatrixTransform *>
                (currentNode->getChild(index));
        assert(tempLocation);
        tempLocation->setMatrix(osg::Matrixd::translate(locations.at(index)));

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
        osg::Switch *tempSwitch = dynamic_cast<osg::Switch *>
                (tempLocation->getChild(tempLocation->getNumChildren() - 1));
        assert(tempSwitch);
        tempSwitch->setAllChildrenOff();

        tempSwitch = dynamic_cast<osg::Switch *>
                (tempLocation->getChild(tempLocation->getNumChildren() - 2));
        assert(tempSwitch);
        tempSwitch->setAllChildrenOff();
    }
}

void GestureHandler::constructMenu()
{
    startNode = gsn::buildMenuNode(":/resources/images/start.svg");
    gestureSwitch->addChild(startNode);

    osg::Matrixd dummy;

    //view base
    osg::MatrixTransform *viewBase;
    viewBase = gsn::buildMenuNode(":/resources/images/viewBase.svg");
    viewBase->setMatrix(dummy);
    startNode->insertChild(startNode->getNumChildren() - 2, viewBase);

    osg::MatrixTransform *viewStandard;
    viewStandard = gsn::buildMenuNode(":/resources/images/viewStandard.svg");
    viewStandard->setMatrix(dummy);
    viewBase->insertChild(viewBase->getNumChildren() - 2, viewStandard);

    osg::MatrixTransform *viewTop = gsn::buildCommandNode(":/resources/images/viewTop.svg");
    viewTop->setMatrix(dummy);
    viewTop->setUserValue(attributeTitle, (msg::Request | msg::ViewTop).to_string());
    viewStandard->insertChild(viewStandard->getNumChildren() - 2, viewTop);

    osg::MatrixTransform *viewFront = gsn::buildCommandNode(":/resources/images/viewFront.svg");
    viewFront->setMatrix(dummy);
    viewFront->setUserValue(attributeTitle, (msg::Request | msg::ViewFront).to_string());
    viewStandard->insertChild(viewStandard->getNumChildren() - 2, viewFront);

    osg::MatrixTransform *viewRight = gsn::buildCommandNode(":/resources/images/viewRight.svg");
    viewRight->setMatrix(dummy);
    viewRight->setUserValue(attributeTitle, (msg::Request | msg::ViewRight).to_string());
    viewStandard->insertChild(viewStandard->getNumChildren() - 2, viewRight);

    osg::MatrixTransform *viewFit = gsn::buildCommandNode(":/resources/images/viewFit.svg");
    viewFit->setMatrix(dummy);
    viewFit->setUserValue(attributeTitle, (msg::Request | msg::ViewFit).to_string());
    viewBase->insertChild(viewBase->getNumChildren() - 2, viewFit);
    
    osg::MatrixTransform *viewFill = gsn::buildCommandNode(":/resources/images/viewFill.svg");
    viewFill->setMatrix(dummy);
    viewFill->setUserValue(attributeTitle, (msg::Request | msg::ViewFill).to_string());
    viewBase->insertChild(viewBase->getNumChildren() - 2, viewFill);
    
    osg::MatrixTransform *viewLine = gsn::buildCommandNode(":/resources/images/viewLine.svg");
    viewLine->setMatrix(dummy);
    viewLine->setUserValue(attributeTitle, (msg::Request | msg::ViewLine).to_string());
    viewBase->insertChild(viewBase->getNumChildren() - 2, viewLine);

    //construction base
    osg::MatrixTransform *constructionBase;
    constructionBase = gsn::buildMenuNode(":/resources/images/constructionBase.svg");
    constructionBase->setMatrix(dummy);
    startNode->insertChild(startNode->getNumChildren() - 2, constructionBase);

    osg::MatrixTransform *constructionPrimitives;
    constructionPrimitives = gsn::buildMenuNode(":/resources/images/constructionPrimitives.svg");
    constructionPrimitives->setMatrix(dummy);
    constructionBase->insertChild(constructionBase->getNumChildren() - 2, constructionPrimitives);

    osg::MatrixTransform *constructionBox = gsn::buildCommandNode(":/resources/images/constructionBox.svg");
    constructionBox->setMatrix(dummy);
    constructionBox->setUserValue(attributeTitle, (msg::Request | msg::Construct | msg::Box).to_string());
    constructionPrimitives->insertChild(constructionPrimitives->getNumChildren() - 2, constructionBox);

    osg::MatrixTransform *constructionSphere = gsn::buildCommandNode(":/resources/images/constructionSphere.svg");
    constructionSphere->setMatrix(dummy);
    constructionSphere->setUserValue(attributeTitle, (msg::Request | msg::Construct | msg::Sphere).to_string());
    constructionPrimitives->insertChild(constructionPrimitives->getNumChildren() - 2, constructionSphere);

    osg::MatrixTransform *constructionCone = gsn::buildCommandNode(":/resources/images/constructionCone.svg");
    constructionCone->setMatrix(dummy);
    constructionCone->setUserValue(attributeTitle, (msg::Request | msg::Construct | msg::Cone).to_string());
    constructionPrimitives->insertChild(constructionPrimitives->getNumChildren() - 2, constructionCone);

    osg::MatrixTransform *constructionCylinder = gsn::buildCommandNode(":/resources/images/constructionCylinder.svg");
    constructionCylinder->setMatrix(dummy);
    constructionCylinder->setUserValue(attributeTitle, (msg::Request | msg::Construct | msg::Cylinder).to_string());
    constructionPrimitives->insertChild(constructionPrimitives->getNumChildren() - 2, constructionCylinder);
    
    //construction finishing base
    osg::MatrixTransform *constructionFinishing;
    constructionFinishing = gsn::buildMenuNode(":/resources/images/constructionFinishing.svg");
    constructionFinishing->setMatrix(dummy);
    constructionBase->insertChild(constructionBase->getNumChildren() - 2, constructionFinishing);
    
    osg::MatrixTransform *constructionBlend = gsn::buildCommandNode(":/resources/images/constructionBlend.svg");
    constructionBlend->setMatrix(dummy);
    constructionBlend->setUserValue(attributeTitle, (msg::Request | msg::Construct | msg::Blend).to_string());
    constructionFinishing->insertChild(constructionFinishing->getNumChildren() - 2, constructionBlend);
    
    //booleans
    osg::MatrixTransform *constructionBoolean;
    constructionBoolean = gsn::buildMenuNode(":/resources/images/constructionBoolean.svg");
    constructionBoolean->setMatrix(dummy);
    constructionBase->insertChild(constructionBase->getNumChildren() - 2, constructionBoolean);
    
    osg::MatrixTransform *constructionUnion = gsn::buildCommandNode(":/resources/images/constructionUnion.svg");
    constructionUnion->setMatrix(dummy);
    constructionUnion->setUserValue(attributeTitle, (msg::Request | msg::Construct | msg::Union).to_string());
    constructionBoolean->insertChild(constructionBoolean->getNumChildren() - 2, constructionUnion);
    
    osg::MatrixTransform *constructionSubtract = gsn::buildCommandNode(":/resources/images/constructionSubtract.svg");
    constructionSubtract->setMatrix(dummy);
    constructionSubtract->setUserValue(attributeTitle, (msg::Request | msg::Construct | msg::Subtract).to_string());
    constructionBoolean->insertChild(constructionBoolean->getNumChildren() - 2, constructionSubtract);
    
    osg::MatrixTransform *constructionIntersect = gsn::buildCommandNode(":/resources/images/constructionIntersect.svg");
    constructionIntersect->setMatrix(dummy);
    constructionIntersect->setUserValue(attributeTitle, (msg::Request | msg::Construct | msg::Intersect).to_string());
    constructionBoolean->insertChild(constructionBoolean->getNumChildren() - 2, constructionIntersect);
    
    //edit base
    osg::MatrixTransform *editBase;
    editBase = gsn::buildMenuNode(":/resources/images/editBase.svg");
    editBase->setMatrix(dummy);
    startNode->insertChild(startNode->getNumChildren() - 2, editBase);
    
    osg::MatrixTransform *remove = gsn::buildCommandNode(":/resources/images/editRemove.svg");
    remove->setMatrix(dummy);
    remove->setUserValue(attributeTitle, (msg::Request | msg::Remove).to_string());
    editBase->insertChild(editBase->getNumChildren() - 2, remove);
    
    osg::MatrixTransform *editUpdate = gsn::buildCommandNode(":/resources/images/editUpdate.svg");
    editUpdate->setMatrix(dummy);
    editUpdate->setUserValue(attributeTitle, (msg::Request | msg::Update).to_string());
    editBase->insertChild(editBase->getNumChildren() - 2, editUpdate);
    
    osg::MatrixTransform *editForceUpdate = gsn::buildCommandNode(":/resources/images/editForceUpdate.svg");
    editForceUpdate->setMatrix(dummy);
    editForceUpdate->setUserValue(attributeTitle, (msg::Request | msg::ForceUpdate).to_string());
    editBase->insertChild(editBase->getNumChildren() - 2, editForceUpdate);
    
    //file base
    osg::MatrixTransform *fileBase;
    fileBase = gsn::buildMenuNode(":/resources/images/fileBase.svg");
    fileBase->setMatrix(dummy);
    startNode->insertChild(startNode->getNumChildren() - 2, fileBase);
    
    osg::MatrixTransform *fileImport;
    fileImport = gsn::buildMenuNode(":/resources/images/fileImport.svg");
    fileImport->setMatrix(dummy);
    fileBase->insertChild(fileBase->getNumChildren() - 2, fileImport);
    
    osg::MatrixTransform *fileImportOCC = gsn::buildCommandNode(":/resources/images/fileOCC.svg");
    fileImportOCC->setMatrix(dummy);
    fileImportOCC->setUserValue(attributeTitle, (msg::Request | msg::ImportOCC).to_string());
    fileImport->insertChild(fileImport->getNumChildren() - 2, fileImportOCC);
    
    osg::MatrixTransform *fileExport;
    fileExport = gsn::buildMenuNode(":/resources/images/fileExport.svg");
    fileExport->setMatrix(dummy);
    fileBase->insertChild(fileBase->getNumChildren() - 2, fileExport);
    
    osg::MatrixTransform *fileExportOSG = gsn::buildCommandNode(":/resources/images/fileOSG.svg");
    fileExportOSG->setMatrix(dummy);
    fileExportOSG->setUserValue(attributeTitle, (msg::Request | msg::ExportOSG).to_string());
    fileExport->insertChild(fileExport->getNumChildren() - 2, fileExportOSG);
    
    osg::MatrixTransform *fileExportOCC = gsn::buildCommandNode(":/resources/images/fileOCC.svg");
    fileExportOCC->setMatrix(dummy);
    fileExportOCC->setUserValue(attributeTitle, (msg::Request | msg::ExportOCC).to_string());
    fileExport->insertChild(fileExport->getNumChildren() - 2, fileExportOCC);
    
    //probably won't stay under file node. good enough for now.
    osg::MatrixTransform *preferences = gsn::buildCommandNode(":/resources/images/preferences.svg");
    preferences->setMatrix(dummy);
    preferences->setUserValue(attributeTitle, (msg::Request | msg::Preferences).to_string());
    fileBase->insertChild(fileBase->getNumChildren() - 2, preferences);
}

std::vector<osg::Vec3> GestureHandler::buildNodeLocations(osg::Vec3 direction, int nodeCount)
{
    double sprayRadius = calculateSprayRadius(nodeCount);
    osg::Vec3 point = direction;
    point.normalize();
    point *= sprayRadius;

    double localIncludedAngle = includedAngle;
    if (sprayRadius == mininumSprayRadius)
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
        return mininumSprayRadius;
    double sprayRadius;
    sprayRadius = (nodeSpread * segmentCount / 2.0) / sin (includedAngle / 2.0);
    if (mininumSprayRadius > sprayRadius)
        return mininumSprayRadius;
    return sprayRadius;
}

void GestureHandler::startDrag(const osgGA::GUIEventAdapter& eventAdapter)
{
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

void GestureHandler::messageInSlot(const msg::Message &messageIn)
{
  //not using yet
  
//   msg::MessageDispatcher::iterator it = dispatcher.find(messageIn.mask);
//   if (it == dispatcher.end())
//     return;
//   
//   it->second(messageIn);
}

void GestureHandler::setupDispatcher()
{
  //not using yet
  
//   msg::Mask mask;
//   
//   mask = msg::Response | msg::Post | msg::NewProject;
//   dispatcher.insert(std::make_pair(mask, boost::bind(&Factory::newProjectDispatched, this, _1)));
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

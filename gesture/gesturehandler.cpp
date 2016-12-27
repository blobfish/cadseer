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
#include <osg/Depth>

#include <viewer/message.h>
#include <gesture/gesturenode.h>
#include <modelviz/nodemaskdefs.h>
#include <message/dispatch.h>
#include <message/observer.h>
#include <gesture/gesturehandler.h>

static const std::string attributeMask = "CommandAttributeTitle";
static const std::string attributeStatus = "CommandAttributeStatus";

GestureHandler::GestureHandler(osg::Camera *cameraIn) : osgGA::GUIEventHandler(), rightButtonDown(false),
    currentNodeLeft(false), iconRadius(32.0), includedAngle(90.0)
{
    observer = std::move(std::unique_ptr<msg::Observer>(new msg::Observer()));
    observer->name = "GestureHandler";
  
    gestureCamera = cameraIn;
    if (!gestureCamera.valid())
        return;
    gestureSwitch = dynamic_cast<osg::Switch *>(gestureCamera->getChild(0));
    if (!gestureSwitch.valid())
        return;

    mininumSprayRadius = iconRadius * 7.0;
    nodeSpread = iconRadius * 3.0;
    constructMenu();
}

bool GestureHandler::handle(const osgGA::GUIEventAdapter& eventAdapter,
                            osgGA::GUIActionAdapter&, osg::Object *,
                            osg::NodeVisitor *)
{
    //lambda to clear status.
    auto clearStatus = [&]()
    {
      //clear any status message
      observer->messageOutSignal(msg::buildStatusMessage(""));
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
            if (currentNode->getNodeMask() & NodeMaskDef::gestureCommand)
            {
                std::string msgMaskString;
                if (currentNode->getUserValue(attributeMask, msgMaskString))
                {
                    msg::Mask msgMask(msgMaskString);
                    msg::Message messageOut;
                    messageOut.mask = msgMask;
                    observer->messageOutSignal(messageOut);
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

                std::string statusString;
                if (currentNode->getUserValue(attributeStatus, statusString))
                {
                  observer->messageOutSignal(msg::buildStatusMessage(statusString));
                }

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
    if (childCount < 3)
      return;
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
        geometry->dirtyBound();
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
    startNode->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    startNode->getOrCreateStateSet()->setAttributeAndModes(new osg::Depth(osg::Depth::LEQUAL, 0.0, 0.0));
    gestureSwitch->insertChild(0, startNode);

    osg::Matrixd dummy;

    //view base
    osg::MatrixTransform *viewBase;
    viewBase = gsn::buildMenuNode(":/resources/images/viewBase.svg");
    viewBase->setMatrix(dummy);
    viewBase->setUserValue(attributeStatus, QObject::tr("View Menu").toStdString());
    startNode->insertChild(startNode->getNumChildren() - 2, viewBase);

    osg::MatrixTransform *viewStandard;
    viewStandard = gsn::buildMenuNode(":/resources/images/viewStandard.svg");
    viewStandard->setMatrix(dummy);
    viewStandard->setUserValue(attributeStatus, QObject::tr("Standard Views Menu").toStdString());
    viewBase->insertChild(viewBase->getNumChildren() - 2, viewStandard);

    osg::MatrixTransform *viewTop = gsn::buildCommandNode(":/resources/images/viewTop.svg");
    viewTop->setMatrix(dummy);
    viewTop->setUserValue(attributeMask, (msg::Request | msg::ViewTop).to_string());
    viewTop->setUserValue(attributeStatus, QObject::tr("Top View Command").toStdString());
    viewStandard->insertChild(viewStandard->getNumChildren() - 2, viewTop);

    osg::MatrixTransform *viewFront = gsn::buildCommandNode(":/resources/images/viewFront.svg");
    viewFront->setMatrix(dummy);
    viewFront->setUserValue(attributeMask, (msg::Request | msg::ViewFront).to_string());
    viewFront->setUserValue(attributeStatus, QObject::tr("View Front Command").toStdString());
    viewStandard->insertChild(viewStandard->getNumChildren() - 2, viewFront);

    osg::MatrixTransform *viewRight = gsn::buildCommandNode(":/resources/images/viewRight.svg");
    viewRight->setMatrix(dummy);
    viewRight->setUserValue(attributeMask, (msg::Request | msg::ViewRight).to_string());
    viewRight->setUserValue(attributeStatus, QObject::tr("View Right Command").toStdString());
    viewStandard->insertChild(viewStandard->getNumChildren() - 2, viewRight);

    osg::MatrixTransform *viewFit = gsn::buildCommandNode(":/resources/images/viewFit.svg");
    viewFit->setMatrix(dummy);
    viewFit->setUserValue(attributeMask, (msg::Request | msg::ViewFit).to_string());
    viewFit->setUserValue(attributeStatus, QObject::tr("View Fit Command").toStdString());
    viewBase->insertChild(viewBase->getNumChildren() - 2, viewFit);
    
    osg::MatrixTransform *viewFill = gsn::buildCommandNode(":/resources/images/viewFill.svg");
    viewFill->setMatrix(dummy);
    viewFill->setUserValue(attributeMask, (msg::Request | msg::ViewFill).to_string());
    viewFill->setUserValue(attributeStatus, QObject::tr("View Fill Command").toStdString());
    viewBase->insertChild(viewBase->getNumChildren() - 2, viewFill);
    
    osg::MatrixTransform *viewLine = gsn::buildCommandNode(":/resources/images/viewLine.svg");
    viewLine->setMatrix(dummy);
    viewLine->setUserValue(attributeMask, (msg::Request | msg::ViewLine).to_string());
    viewLine->setUserValue(attributeStatus, QObject::tr("View Lines Command").toStdString());
    viewBase->insertChild(viewBase->getNumChildren() - 2, viewLine);
    
    //construction base
    osg::MatrixTransform *constructionBase;
    constructionBase = gsn::buildMenuNode(":/resources/images/constructionBase.svg");
    constructionBase->setMatrix(dummy);
    constructionBase->setUserValue(attributeStatus, QObject::tr("Construction Menu").toStdString());
    startNode->insertChild(startNode->getNumChildren() - 2, constructionBase);

    osg::MatrixTransform *constructionPrimitives;
    constructionPrimitives = gsn::buildMenuNode(":/resources/images/constructionPrimitives.svg");
    constructionPrimitives->setMatrix(dummy);
    constructionPrimitives->setUserValue(attributeStatus, QObject::tr("Primitives Menu").toStdString());
    constructionBase->insertChild(constructionBase->getNumChildren() - 2, constructionPrimitives);

    osg::MatrixTransform *constructionBox = gsn::buildCommandNode(":/resources/images/constructionBox.svg");
    constructionBox->setMatrix(dummy);
    constructionBox->setUserValue(attributeMask, (msg::Request | msg::Construct | msg::Box).to_string());
    constructionBox->setUserValue(attributeStatus, QObject::tr("Box Command").toStdString());
    constructionPrimitives->insertChild(constructionPrimitives->getNumChildren() - 2, constructionBox);

    osg::MatrixTransform *constructionSphere = gsn::buildCommandNode(":/resources/images/constructionSphere.svg");
    constructionSphere->setMatrix(dummy);
    constructionSphere->setUserValue(attributeMask, (msg::Request | msg::Construct | msg::Sphere).to_string());
    constructionSphere->setUserValue(attributeStatus, QObject::tr("Sphere Command").toStdString());
    constructionPrimitives->insertChild(constructionPrimitives->getNumChildren() - 2, constructionSphere);

    osg::MatrixTransform *constructionCone = gsn::buildCommandNode(":/resources/images/constructionCone.svg");
    constructionCone->setMatrix(dummy);
    constructionCone->setUserValue(attributeMask, (msg::Request | msg::Construct | msg::Cone).to_string());
    constructionCone->setUserValue(attributeStatus, QObject::tr("Cone Command").toStdString());
    constructionPrimitives->insertChild(constructionPrimitives->getNumChildren() - 2, constructionCone);

    osg::MatrixTransform *constructionCylinder = gsn::buildCommandNode(":/resources/images/constructionCylinder.svg");
    constructionCylinder->setMatrix(dummy);
    constructionCylinder->setUserValue(attributeMask, (msg::Request | msg::Construct | msg::Cylinder).to_string());
    constructionCylinder->setUserValue(attributeStatus, QObject::tr("Cylinder Command").toStdString());
    constructionPrimitives->insertChild(constructionPrimitives->getNumChildren() - 2, constructionCylinder);
    
    //construction finishing base
    osg::MatrixTransform *constructionFinishing;
    constructionFinishing = gsn::buildMenuNode(":/resources/images/constructionFinishing.svg");
    constructionFinishing->setMatrix(dummy);
    constructionFinishing->setUserValue(attributeStatus, QObject::tr("Finishing Menu").toStdString());
    constructionBase->insertChild(constructionBase->getNumChildren() - 2, constructionFinishing);
    
    osg::MatrixTransform *constructionBlend = gsn::buildCommandNode(":/resources/images/constructionBlend.svg");
    constructionBlend->setMatrix(dummy);
    constructionBlend->setUserValue(attributeMask, (msg::Request | msg::Construct | msg::Blend).to_string());
    constructionBlend->setUserValue(attributeStatus, QObject::tr("Blend Command").toStdString());
    constructionFinishing->insertChild(constructionFinishing->getNumChildren() - 2, constructionBlend);
    
    osg::MatrixTransform *constructionChamfer = gsn::buildCommandNode(":/resources/images/constructionChamfer.svg");
    constructionChamfer->setMatrix(dummy);
    constructionChamfer->setUserValue(attributeMask, (msg::Request | msg::Construct | msg::Chamfer).to_string());
    constructionChamfer->setUserValue(attributeStatus, QObject::tr("Chamfer Command").toStdString());
    constructionFinishing->insertChild(constructionFinishing->getNumChildren() - 2, constructionChamfer);
    
    osg::MatrixTransform *constructionDraft = gsn::buildCommandNode(":/resources/images/constructionDraft.svg");
    constructionDraft->setMatrix(dummy);
    constructionDraft->setUserValue(attributeMask, (msg::Request | msg::Construct | msg::Draft).to_string());
    constructionDraft->setUserValue(attributeStatus, QObject::tr("Draft Command").toStdString());
    constructionFinishing->insertChild(constructionFinishing->getNumChildren() - 2, constructionDraft);
    
    //booleans
    osg::MatrixTransform *constructionBoolean;
    constructionBoolean = gsn::buildMenuNode(":/resources/images/constructionBoolean.svg");
    constructionBoolean->setMatrix(dummy);
    constructionBoolean->setUserValue(attributeStatus, QObject::tr("Boolean Menu").toStdString());
    constructionBase->insertChild(constructionBase->getNumChildren() - 2, constructionBoolean);
    
    osg::MatrixTransform *constructionUnion = gsn::buildCommandNode(":/resources/images/constructionUnion.svg");
    constructionUnion->setMatrix(dummy);
    constructionUnion->setUserValue(attributeMask, (msg::Request | msg::Construct | msg::Union).to_string());
    constructionUnion->setUserValue(attributeStatus, QObject::tr("Union Command").toStdString());
    constructionBoolean->insertChild(constructionBoolean->getNumChildren() - 2, constructionUnion);
    
    osg::MatrixTransform *constructionSubtract = gsn::buildCommandNode(":/resources/images/constructionSubtract.svg");
    constructionSubtract->setMatrix(dummy);
    constructionSubtract->setUserValue(attributeMask, (msg::Request | msg::Construct | msg::Subtract).to_string());
    constructionSubtract->setUserValue(attributeStatus, QObject::tr("Subtract Command").toStdString());
    constructionBoolean->insertChild(constructionBoolean->getNumChildren() - 2, constructionSubtract);
    
    osg::MatrixTransform *constructionIntersect = gsn::buildCommandNode(":/resources/images/constructionIntersect.svg");
    constructionIntersect->setMatrix(dummy);
    constructionIntersect->setUserValue(attributeMask, (msg::Request | msg::Construct | msg::Intersect).to_string());
    constructionIntersect->setUserValue(attributeStatus, QObject::tr("Intersection Command").toStdString());
    constructionBoolean->insertChild(constructionBoolean->getNumChildren() - 2, constructionIntersect);
    
    //TODO build datum menu node and move datum plane to it.
    osg::MatrixTransform *constructDatumPlane = gsn::buildCommandNode(":/resources/images/constructionDatumPlane.svg");
    constructDatumPlane->setMatrix(dummy);
    constructDatumPlane->setUserValue(attributeMask, (msg::Request | msg::Construct | msg::DatumPlane).to_string());
    constructDatumPlane->setUserValue(attributeStatus, QObject::tr("Datum Plane Command").toStdString());
    constructionBase->insertChild(constructionBase->getNumChildren() - 2, constructDatumPlane);
    
    //edit base
    osg::MatrixTransform *editBase;
    editBase = gsn::buildMenuNode(":/resources/images/editBase.svg");
    editBase->setMatrix(dummy);
    editBase->setUserValue(attributeStatus, QObject::tr("Edit Menu").toStdString());
    startNode->insertChild(startNode->getNumChildren() - 2, editBase);
    
    osg::MatrixTransform *remove = gsn::buildCommandNode(":/resources/images/editRemove.svg");
    remove->setMatrix(dummy);
    remove->setUserValue(attributeMask, (msg::Request | msg::Remove).to_string());
    remove->setUserValue(attributeStatus, QObject::tr("Remove Command").toStdString());
    editBase->insertChild(editBase->getNumChildren() - 2, remove);
    
    osg::MatrixTransform *editUpdate = gsn::buildCommandNode(":/resources/images/editUpdate.svg");
    editUpdate->setMatrix(dummy);
    editUpdate->setUserValue(attributeMask, (msg::Request | msg::Update).to_string());
    editUpdate->setUserValue(attributeStatus, QObject::tr("Update Command").toStdString());
    editBase->insertChild(editBase->getNumChildren() - 2, editUpdate);
    
    osg::MatrixTransform *editForceUpdate = gsn::buildCommandNode(":/resources/images/editForceUpdate.svg");
    editForceUpdate->setMatrix(dummy);
    editForceUpdate->setUserValue(attributeMask, (msg::Request | msg::ForceUpdate).to_string());
    editForceUpdate->setUserValue(attributeStatus, QObject::tr("Force Update Command").toStdString());
    editBase->insertChild(editBase->getNumChildren() - 2, editForceUpdate);
    
    osg::MatrixTransform *editSystemBase;
    editSystemBase = gsn::buildMenuNode(":/resources/images/systemBase.svg");
    editSystemBase->setMatrix(dummy);
    editSystemBase->setUserValue(attributeStatus, QObject::tr("Coordinate System Menu").toStdString());
    editBase->insertChild(editBase->getNumChildren() - 2, editSystemBase);
    
    osg::MatrixTransform *featureToSystem = gsn::buildCommandNode(":/resources/images/featureToSystem.svg");
    featureToSystem->setMatrix(dummy);
    featureToSystem->setUserValue(attributeMask, (msg::Request | msg::FeatureToSystem).to_string());
    featureToSystem->setUserValue(attributeStatus, QObject::tr("Feature To Coordinate System Command").toStdString());
    editSystemBase->insertChild(editSystemBase->getNumChildren() - 2, featureToSystem);
    
    osg::MatrixTransform *editFeatureToDragger = gsn::buildCommandNode(":/resources/images/editFeatureToDragger.svg");
    editFeatureToDragger->setMatrix(dummy);
    editFeatureToDragger->setUserValue(attributeMask, (msg::Request | msg::FeatureToDragger).to_string());
    editFeatureToDragger->setUserValue(attributeStatus, QObject::tr("Feature To Dragger Command").toStdString());
    editSystemBase->insertChild(editSystemBase->getNumChildren() - 2, editFeatureToDragger);
    
    osg::MatrixTransform *editDraggerToFeature = gsn::buildCommandNode(":/resources/images/editDraggerToFeature.svg");
    editDraggerToFeature->setMatrix(dummy);
    editDraggerToFeature->setUserValue(attributeMask, (msg::Request | msg::DraggerToFeature).to_string());
    editDraggerToFeature->setUserValue(attributeStatus, QObject::tr("Dragger To Feature Command").toStdString());
    editSystemBase->insertChild(editSystemBase->getNumChildren() - 2, editDraggerToFeature);
    
    osg::MatrixTransform *preferences = gsn::buildCommandNode(":/resources/images/preferences.svg");
    preferences->setMatrix(dummy);
    preferences->setUserValue(attributeMask, (msg::Request | msg::Preferences).to_string());
    preferences->setUserValue(attributeStatus, QObject::tr("Preferences Command").toStdString());
    editBase->insertChild(editBase->getNumChildren() - 2, preferences);
    
    //file base
    osg::MatrixTransform *fileBase;
    fileBase = gsn::buildMenuNode(":/resources/images/fileBase.svg");
    fileBase->setMatrix(dummy);
    fileBase->setUserValue(attributeStatus, QObject::tr("File Menu").toStdString());
    startNode->insertChild(startNode->getNumChildren() - 2, fileBase);
    
    osg::MatrixTransform *fileImport;
    fileImport = gsn::buildMenuNode(":/resources/images/fileImport.svg");
    fileImport->setMatrix(dummy);
    fileImport->setUserValue(attributeStatus, QObject::tr("Import Menu").toStdString());
    fileBase->insertChild(fileBase->getNumChildren() - 2, fileImport);
    
    osg::MatrixTransform *fileImportOCC = gsn::buildCommandNode(":/resources/images/fileOCC.svg");
    fileImportOCC->setMatrix(dummy);
    fileImportOCC->setUserValue(attributeMask, (msg::Request | msg::ImportOCC).to_string());
    fileImportOCC->setUserValue(attributeStatus, QObject::tr("Import OCC Brep Command").toStdString());
    fileImport->insertChild(fileImport->getNumChildren() - 2, fileImportOCC);
    
    osg::MatrixTransform *fileExport;
    fileExport = gsn::buildMenuNode(":/resources/images/fileExport.svg");
    fileExport->setMatrix(dummy);
    fileExport->setUserValue(attributeStatus, QObject::tr("Export Menu").toStdString());
    fileBase->insertChild(fileBase->getNumChildren() - 2, fileExport);
    
    osg::MatrixTransform *fileExportOSG = gsn::buildCommandNode(":/resources/images/fileOSG.svg");
    fileExportOSG->setMatrix(dummy);
    fileExportOSG->setUserValue(attributeMask, (msg::Request | msg::ExportOSG).to_string());
    fileExportOSG->setUserValue(attributeStatus, QObject::tr("Export Open Scene Graph Command").toStdString());
    fileExport->insertChild(fileExport->getNumChildren() - 2, fileExportOSG);
    
    osg::MatrixTransform *fileExportOCC = gsn::buildCommandNode(":/resources/images/fileOCC.svg");
    fileExportOCC->setMatrix(dummy);
    fileExportOCC->setUserValue(attributeMask, (msg::Request | msg::ExportOCC).to_string());
    fileExportOCC->setUserValue(attributeStatus, QObject::tr("Export OCC Brep Command").toStdString());
    fileExport->insertChild(fileExport->getNumChildren() - 2, fileExportOCC);
    
    osg::MatrixTransform *fileOpen = gsn::buildCommandNode(":/resources/images/fileOpen.svg");
    fileOpen->setMatrix(dummy);
    fileOpen->setUserValue(attributeMask, (msg::Request | msg::ProjectDialog).to_string());
    fileOpen->setUserValue(attributeStatus, QObject::tr("Open Project Command").toStdString());
    fileBase->insertChild(fileBase->getNumChildren() - 2, fileOpen);
    
    osg::MatrixTransform *fileSave = gsn::buildCommandNode(":/resources/images/fileSave.svg");
    fileSave->setMatrix(dummy);
    fileSave->setUserValue(attributeMask, (msg::Request | msg::SaveProject).to_string());
    fileSave->setUserValue(attributeStatus, QObject::tr("Save Project Command").toStdString());
    fileBase->insertChild(fileBase->getNumChildren() - 2, fileSave);
    
    //system base
    osg::MatrixTransform *systemBase;
    systemBase = gsn::buildMenuNode(":/resources/images/systemBase.svg");
    systemBase->setMatrix(dummy);
    systemBase->setUserValue(attributeStatus, QObject::tr("Coordinate System Menu").toStdString());
    startNode->insertChild(startNode->getNumChildren() - 2, systemBase);
    
    osg::MatrixTransform *systemReset = gsn::buildCommandNode(":/resources/images/systemReset.svg");
    systemReset->setMatrix(dummy);
    systemReset->setUserValue(attributeMask, (msg::Request | msg::SystemReset).to_string());
    systemReset->setUserValue(attributeStatus, QObject::tr("Coordinate System Reset Command").toStdString());
    systemBase->insertChild(systemBase->getNumChildren() - 2, systemReset);
    
    osg::MatrixTransform *systemToggle = gsn::buildCommandNode(":/resources/images/systemToggle.svg");
    systemToggle->setMatrix(dummy);
    systemToggle->setUserValue(attributeMask, (msg::Request | msg::SystemToggle).to_string());
    systemToggle->setUserValue(attributeStatus, QObject::tr("Toggle Coordinate System Visibility Command").toStdString());
    systemBase->insertChild(systemBase->getNumChildren() - 2, systemToggle);
    
    osg::MatrixTransform *systemToFeature = gsn::buildCommandNode(":/resources/images/systemToFeature.svg");
    systemToFeature->setMatrix(dummy);
    systemToFeature->setUserValue(attributeMask, (msg::Request | msg::SystemToFeature).to_string());
    systemToFeature->setUserValue(attributeStatus, QObject::tr("Coordinate System To Feature Command").toStdString());
    systemBase->insertChild(systemBase->getNumChildren() - 2, systemToFeature);
    
    //inpect base
    osg::MatrixTransform *inspectBase;
    inspectBase = gsn::buildMenuNode(":/resources/images/inspectBase.svg");
    inspectBase->setMatrix(dummy);
    inspectBase->setUserValue(attributeStatus, QObject::tr("Inspect Menu").toStdString());
    startNode->insertChild(startNode->getNumChildren() - 2, inspectBase);
    
    osg::MatrixTransform *inpsectInfo = gsn::buildCommandNode(":/resources/images/inspectInfo.svg");
    inpsectInfo->setMatrix(dummy);
    inpsectInfo->setUserValue(attributeMask, (msg::Request | msg::ViewInfo).to_string());
    inpsectInfo->setUserValue(attributeStatus, QObject::tr("View Info Command").toStdString());
    inspectBase->insertChild(inspectBase->getNumChildren() - 2, inpsectInfo);
    
    osg::MatrixTransform *inspectCheckGeometry = gsn::buildCommandNode(":/resources/images/inspectCheckGeometry.svg");
    inspectCheckGeometry->setMatrix(dummy);
    inspectCheckGeometry->setUserValue(attributeMask, (msg::Request | msg::CheckGeometry).to_string());
    inspectCheckGeometry->setUserValue(attributeStatus, QObject::tr("Check Geometry For Errors").toStdString());
    inspectBase->insertChild(inspectBase->getNumChildren() - 2, inspectCheckGeometry);
    
    //inspect measure base
    osg::MatrixTransform *inspectMeasureBase;
    inspectMeasureBase = gsn::buildMenuNode(":/resources/images/inspectMeasureBase.svg");
    inspectMeasureBase->setMatrix(dummy);
    inspectMeasureBase->setUserValue(attributeStatus, QObject::tr("Measure Menu").toStdString());
    inspectBase->insertChild(inspectBase->getNumChildren() - 2, inspectMeasureBase);
    
    osg::MatrixTransform *inspectMeasureClear = gsn::buildCommandNode(":/resources/images/constructionMeasureClear.svg");
    inspectMeasureClear->setMatrix(dummy);
    inspectMeasureClear->setUserValue(attributeMask, (msg::Request | msg::Clear | msg::OverlayGeometry).to_string());
    inspectMeasureClear->setUserValue(attributeStatus, QObject::tr("Clear Measure Command").toStdString());
    inspectMeasureBase->insertChild(inspectMeasureBase->getNumChildren() - 2, inspectMeasureClear);
    
    osg::MatrixTransform *inspectLinearMeasure = gsn::buildCommandNode(":/resources/images/constructionLinearMeasure.svg");
    inspectLinearMeasure->setMatrix(dummy);
    inspectLinearMeasure->setUserValue(attributeMask, (msg::Request | msg::LinearMeasure).to_string());
    inspectLinearMeasure->setUserValue(attributeStatus, QObject::tr("LinearMeasure Command").toStdString());
    inspectMeasureBase->insertChild(inspectMeasureBase->getNumChildren() - 2, inspectLinearMeasure);
    
    //debug base
    osg::MatrixTransform *debugBase;
    debugBase = gsn::buildMenuNode(":/resources/images/debugBase.svg");
    debugBase->setMatrix(dummy);
    debugBase->setUserValue(attributeStatus, QObject::tr("Debug Menu").toStdString());
    inspectBase->insertChild(inspectBase->getNumChildren() - 2, debugBase);
    
    osg::MatrixTransform *checkShapeIds = gsn::buildCommandNode(":/resources/images/debugCheckShapeIds.svg");
    checkShapeIds->setMatrix(dummy);
    checkShapeIds->setUserValue(attributeMask, (msg::Request | msg::CheckShapeIds).to_string());
    checkShapeIds->setUserValue(attributeStatus, QObject::tr("Check Shaped Ids").toStdString());
    debugBase->insertChild(debugBase->getNumChildren() - 2, checkShapeIds);
    
    osg::MatrixTransform *debugDump = gsn::buildCommandNode(":/resources/images/debugDump.svg");
    debugDump->setMatrix(dummy);
    debugDump->setUserValue(attributeMask, (msg::Request | msg::DebugDump).to_string());
    debugDump->setUserValue(attributeStatus, QObject::tr("Debug Feature Dump").toStdString());
    debugBase->insertChild(debugBase->getNumChildren() - 2, debugDump);
    
    osg::MatrixTransform *debugShapeTrackUp = gsn::buildCommandNode(":/resources/images/debugShapeTrackUp.svg");
    debugShapeTrackUp->setMatrix(dummy);
    debugShapeTrackUp->setUserValue(attributeMask, (msg::Request | msg::DebugShapeTrackUp).to_string());
    debugShapeTrackUp->setUserValue(attributeStatus, QObject::tr("Track Shape Up").toStdString());
    debugBase->insertChild(debugBase->getNumChildren() - 2, debugShapeTrackUp);
    
    osg::MatrixTransform *debugShapeTrackDown = gsn::buildCommandNode(":/resources/images/debugShapeTrackDown.svg");
    debugShapeTrackDown->setMatrix(dummy);
    debugShapeTrackDown->setUserValue(attributeMask, (msg::Request | msg::DebugShapeTrackDown).to_string());
    debugShapeTrackDown->setUserValue(attributeStatus, QObject::tr("Track Shape Down").toStdString());
    debugBase->insertChild(debugBase->getNumChildren() - 2, debugShapeTrackDown);
    
    osg::MatrixTransform *debugShapeGraph = gsn::buildCommandNode(":/resources/images/debugShapeGraph.svg");
    debugShapeGraph->setMatrix(dummy);
    debugShapeGraph->setUserValue(attributeMask, (msg::Request | msg::DebugShapeGraph).to_string());
    debugShapeGraph->setUserValue(attributeStatus, QObject::tr("Write shape graph to application directory").toStdString());
    debugBase->insertChild(debugBase->getNumChildren() - 2, debugShapeGraph);
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
    //send status
    observer->messageOutSignal(msg::buildStatusMessage(QObject::tr("Start Menu").toStdString()));
  
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

GestureAllSwitchesOffVisitor::GestureAllSwitchesOffVisitor() :
    osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
{
}

void GestureAllSwitchesOffVisitor::apply(osg::Switch &aSwitch)
{
    traverse(aSwitch);
    aSwitch.setAllChildrenOff();
}

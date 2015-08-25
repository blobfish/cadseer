#include <iostream>
#include <assert.h>

#include <QImage>
#include <QGLWidget>

#include <osgViewer/View>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgUtil/LineSegmentIntersector>
#include <osg/ValueObject>

#include "gesturenode.h"
#include "gesturehandler.h"
#include "../nodemaskdefs.h"
#include "../command/commandmanager.h"

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
            if (currentNode->getNodeMask() & NodeMask::gestureCommand)
            {
                int temp;
                if (!currentNode->getUserValue(CommandConstants::attributeTitle, temp))
                {
                    std::cout << "couldn't get commandid" << std::endl;
                    return false;
                }
                CommandConstants::Constants commandId = static_cast<CommandConstants::Constants>(temp);

                CommandManager::getManager().trigger(commandId);
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
        iv.setTraversalMask(~NodeMask::noSelect);
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
                    if (currentNode->getNodeMask() & NodeMask::gestureMenu)
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
                if (currentNode->getNodeMask() & NodeMask::gestureMenu)
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
    startNode = GestureNode::buildMenuNode(":/resources/images/start.svg");
    gestureSwitch->addChild(startNode);

    osg::Matrixd dummy;

    //view base
    osg::MatrixTransform *viewBase;
    viewBase = GestureNode::buildMenuNode(":/resources/images/viewBase.svg");
    viewBase->setMatrix(dummy);
    startNode->insertChild(startNode->getNumChildren() - 2, viewBase);

    osg::MatrixTransform *viewStandard;
    viewStandard = GestureNode::buildMenuNode(":/resources/images/viewStandard.svg");
    viewStandard->setMatrix(dummy);
    viewBase->insertChild(viewBase->getNumChildren() - 2, viewStandard);

    osg::MatrixTransform *viewTop = GestureNode::buildCommandNode(":/resources/images/viewTop.svg");
    viewTop->setMatrix(dummy);
    viewTop->setUserValue(CommandConstants::attributeTitle, static_cast<int>(CommandConstants::StandardViewTop));
    viewStandard->insertChild(viewStandard->getNumChildren() - 2, viewTop);

    osg::MatrixTransform *viewFront = GestureNode::buildCommandNode(":/resources/images/viewFront.svg");
    viewFront->setMatrix(dummy);
    viewFront->setUserValue(CommandConstants::attributeTitle, static_cast<int>(CommandConstants::StandardViewFront));
    viewStandard->insertChild(viewStandard->getNumChildren() - 2, viewFront);

    osg::MatrixTransform *viewRight = GestureNode::buildCommandNode(":/resources/images/viewRight.svg");
    viewRight->setMatrix(dummy);
    viewRight->setUserValue(CommandConstants::attributeTitle, static_cast<int>(CommandConstants::StandardViewRight));
    viewStandard->insertChild(viewStandard->getNumChildren() - 2, viewRight);

    osg::MatrixTransform *viewFit = GestureNode::buildCommandNode(":/resources/images/viewFit.svg");
    viewFit->setMatrix(dummy);
    viewFit->setUserValue(CommandConstants::attributeTitle, static_cast<int>(CommandConstants::ViewFit));
    viewBase->insertChild(viewBase->getNumChildren() - 2, viewFit);

    //construction base
    osg::MatrixTransform *constructionBase;
    constructionBase = GestureNode::buildMenuNode(":/resources/images/constructionBase.svg");
    constructionBase->setMatrix(dummy);
    startNode->insertChild(startNode->getNumChildren() - 2, constructionBase);

    osg::MatrixTransform *constructionPrimitives;
    constructionPrimitives = GestureNode::buildMenuNode(":/resources/images/constructionPrimitives.svg");
    constructionPrimitives->setMatrix(dummy);
    constructionBase->insertChild(constructionBase->getNumChildren() - 2, constructionPrimitives);

    osg::MatrixTransform *constructionBox = GestureNode::buildCommandNode(":/resources/images/constructionBox.svg");
    constructionBox->setMatrix(dummy);
    constructionBox->setUserValue(CommandConstants::attributeTitle, static_cast<int>(CommandConstants::ConstructionBox));
    constructionPrimitives->insertChild(constructionPrimitives->getNumChildren() - 2, constructionBox);

    osg::MatrixTransform *constructionSphere = GestureNode::buildCommandNode(":/resources/images/constructionSphere.svg");
    constructionSphere->setMatrix(dummy);
    constructionSphere->setUserValue(CommandConstants::attributeTitle, static_cast<int>(CommandConstants::ConstructionSphere));
    constructionPrimitives->insertChild(constructionPrimitives->getNumChildren() - 2, constructionSphere);

    osg::MatrixTransform *constructionCone = GestureNode::buildCommandNode(":/resources/images/constructionCone.svg");
    constructionCone->setMatrix(dummy);
    constructionCone->setUserValue(CommandConstants::attributeTitle, static_cast<int>(CommandConstants::ConstructionCone));
    constructionPrimitives->insertChild(constructionPrimitives->getNumChildren() - 2, constructionCone);

    osg::MatrixTransform *constructionCylinder = GestureNode::buildCommandNode(":/resources/images/constructionCylinder.svg");
    constructionCylinder->setMatrix(dummy);
    constructionCylinder->setUserValue(CommandConstants::attributeTitle, static_cast<int>(CommandConstants::ConstructionCylinder));
    constructionPrimitives->insertChild(constructionPrimitives->getNumChildren() - 2, constructionCylinder);
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


GestureAllSwitchesOffVisitor::GestureAllSwitchesOffVisitor() :
    osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
{
}

void GestureAllSwitchesOffVisitor::apply(osg::Switch &aSwitch)
{
    traverse(aSwitch);
    aSwitch.setAllChildrenOff();
}

#include <iostream>

#include <osg/ComputeBoundsVisitor>

#include "nodemaskdefs.h"
#include "spaceballosgevent.h"
#include "spaceballmanipulator.h"

using namespace osgGA;

SpaceballManipulator::SpaceballManipulator(osg::Camera *camIn) : inherited(), boundingSphere(),
    cam(camIn), spaceEye(0.0, 0.0, 1.0), spaceCenter(0.0, 0.0, 0.0), spaceUp(0.0, 1.0, 0.0)
{
}

SpaceballManipulator::SpaceballManipulator(const SpaceballManipulator& manipIn,
                                           const osg::CopyOp& copyOp) : inherited(),
                                           cam(manipIn.cam)
{
}

void SpaceballManipulator::init(const GUIEventAdapter &ea, GUIActionAdapter &us)
{
    us.requestContinuousUpdate(false);
}

void SpaceballManipulator::setByMatrix(const osg::Matrixd& matrix)
{
//    matrix.getLookAt(spaceEye, spaceCenter, spaceUp);
}

void SpaceballManipulator::setByInverseMatrix(const osg::Matrixd& matrix)
{
//    setByMatrix(osg::Matrixd::inverse(matrix));
}

osg::Matrixd SpaceballManipulator::getMatrix() const
{
    osg::Matrixd out;
    out.makeIdentity();

    return out;
}

osg::Matrixd SpaceballManipulator::getInverseMatrix() const
{
    osg::Matrixd out;
    out.makeLookAt(spaceEye, spaceCenter, spaceUp);
    return out;
}

void SpaceballManipulator::dump()
{
    std::cout << std::endl << "stats: " << std::endl <<
                 "   spaceEye: " << spaceEye.x() << "   " << spaceEye.y() << "   " << spaceEye.z() << std::endl <<
                 "   spaceCenter: " << spaceCenter.x() << "   " << spaceCenter.y() << "   " << spaceCenter.z() << std::endl <<
                 "   spaceUp: " << spaceUp.x() << "   " << spaceUp.y() << "   " << spaceUp.z() << std::endl;
}

void SpaceballManipulator::setNode(osg::Node *nodeIn)
{
    node = nodeIn;
}

void SpaceballManipulator::computeHomePosition(const osg::Camera *camera, bool useBoundingBox)
{
    if (useBoundingBox)
    {
      // (bounding box computes model center more precisely than bounding sphere)
      osg::ComputeBoundsVisitor cbVisitor;
      cbVisitor.setTraversalMask(~(NodeMaskDef::csys));
      node->accept(cbVisitor);
      osg::BoundingBox &boundingBox = cbVisitor.getBoundingBox();
      if (boundingBox.valid())
        boundingSphere = osg::BoundingSphere(boundingBox);
      else
        boundingSphere = node->getBound();
    }
    else
      boundingSphere = node->getBound();
    
    //when there is nothing in scene getBound returns -1 and the view is screwy.
    if (boundingSphere.radius() < 0)
      boundingSphere.radius() = 1.0;
    camSphere = osg::BoundingSphere(boundingSphere.center(), boundingSphere.radius() * 2.0d);
    getProjectionData();//this is needed to test camera type ortho vs perspective.
    getViewData();//this is needed to viewport width and height.
    if (projectionData.isCamOrtho)
    {
      if (((spaceCenter - boundingSphere.center()).length2()) > (0.01 * boundingSphere.radius()))
      {
        osg::Vec3d diffVector = boundingSphere.center() - spaceCenter;
        //don't change up vector.
        spaceCenter += diffVector;
        spaceEye += diffVector;
        spaceEye = projectToBound(spaceEye, spaceCenter);
      }

        //possibly
        //camera->setComputeNearFar(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    }
}

void SpaceballManipulator::scaleView(double scaleFactor)
{
    projectionData.left += (projectionData.left * scaleFactor);
    projectionData.right += (projectionData.right * scaleFactor);
    projectionData.bottom += (projectionData.bottom * scaleFactor);
    projectionData.top += (projectionData.top * scaleFactor);

    cam->setProjectionMatrixAsOrtho(projectionData.left, projectionData.right, projectionData.bottom,
                                    projectionData.top, projectionData.near, projectionData.far);
}

void SpaceballManipulator::home(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us)
{
    if (getAutoComputeHomePosition())
        computeHomePosition(cam, true);

    scaleFit();

    us.requestRedraw();
    us.requestContinuousUpdate(false);
}

void SpaceballManipulator::home(double)
{
    if (getAutoComputeHomePosition())
        computeHomePosition(cam, true);
    
    scaleFit();
}

bool SpaceballManipulator::handle(const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &us)
{
    if (ea.getEventType() == osgGA::GUIEventAdapter::USER)
    {
        const SpaceballOSGEvent *event = static_cast<const SpaceballOSGEvent *>(ea.getUserData());
        if (!event || !cam || !node.get())
        {
            std::cout << "error in spaceball event" << std::endl;
            return true;
        }
        getProjectionData();
        getViewData();
        if (projectionData.isCamOrtho)
            goOrtho(event);
        else
            goPerspective(event);
        us.requestRedraw();
        return true;
    }
    return inherited::handle(ea, us);
}

void SpaceballManipulator::getProjectionData()
{
    double fovy, aspectRatio, left, right, top, bottom, near, far;
    if (cam->getProjectionMatrixAsPerspective(fovy, aspectRatio, near, far))
    {
        projectionData.fovy = fovy;
        projectionData.aspectRatio = aspectRatio;
        projectionData.near = near;
        projectionData.far = far;
        projectionData.isCamOrtho = false;
    }
    else
    {
        cam->getProjectionMatrixAsOrtho(left, right, bottom, top, near, far);
        projectionData.left = left;
        projectionData.right = right;
        projectionData.bottom = bottom;
        projectionData.top = top;
        projectionData.near = near;
        projectionData.far = far;
        projectionData.isCamOrtho = true;
    }
}

void SpaceballManipulator::getViewData()
{
    //get view vectors.
    viewData.z = spaceCenter - spaceEye;
    viewData.z.normalize();
    viewData.y = spaceUp;
    viewData.y.normalize();
    viewData.x = viewData.y ^ viewData.z;
    viewData.x.normalize();
}

void SpaceballManipulator::goOrtho(const SpaceballOSGEvent *event)
{
    osg::Vec3d newEye, newCenter, newUp;
    newEye = spaceEye;
    newCenter = spaceCenter;
    newUp = spaceUp;

    //rotation point.
    osg::Vec3d rotationPoint;
    rotationPoint = newCenter;

    //derive motion factor.
    double width = projectionData.right - projectionData.left;
    double height = projectionData.top - projectionData.bottom;
    double gauge = std::min(width, height) / 512.0d;

    double rotateFactor = .0002;
    osg::Quat rx, ry, rz;
    rx.makeRotate(event->rotationX * rotateFactor, viewData.x);
    ry.makeRotate(event->rotationY * rotateFactor * -1.0d, viewData.y);
    rz.makeRotate(event->rotationZ * rotateFactor, viewData.z);
    newUp = rx * ry * rz * newUp;
    osg::Vec3d tempEye = newEye - rotationPoint;
    tempEye = rx * ry * rz * tempEye;
    newEye = tempEye + rotationPoint;

    double transFactor = gauge * .05;
    osg::Vec3d transVec = viewData.x * (event->translationX * transFactor) +
            viewData.y * (event->translationY * transFactor * -1.0d);//don't move z
    newEye = transVec + newEye;
    newCenter = transVec + newCenter;
    scaleView(event->translationZ * -.0001);

    //ortho views want to stay outside of model boundary.
    double camDistance = (camSphere.center() - newEye).length();
    if (camDistance < camSphere.radius())
        newEye = projectToBound(newEye, newCenter);

    //move the center to half way point between the
    //new and far clipping planes.
    osg::Vec3d lookVector = newCenter - newEye;
    lookVector.normalize();
    lookVector *= (projectionData.far - projectionData.near) / 2.0d + projectionData.near;
    if (lookVector.length() > 0.0)
        newCenter = newEye + lookVector;

    spaceEye = newEye;
    spaceCenter = newCenter;
    spaceUp = newUp;
}

void dumpVector(const osg::Vec3d &vector)
{
    std::cout << vector.x() << "         " << vector.y() << "          " << vector.z() << "           length: " << vector.length();

}

osg::Vec3d SpaceballManipulator::projectToBound(const osg::Vec3d &eye, osg::Vec3d lookCenter) const
{
    osg::Vec3d boundCenter = camSphere.center();
    boundCenter -= eye;
    lookCenter -= eye;

    //dot product.
    osg::Vec3d corner = lookCenter;
    corner.normalize();
    corner *= (boundCenter * corner);

    double leg = (boundCenter - corner).length();
    double hypotenuse = camSphere.radius();
    double otherLeg = sqrt(pow(hypotenuse, 2.0d) - pow(leg, 2.0d));

    osg::Vec3d projection = corner * -1.0d;
    projection.normalize();
    projection *= otherLeg;
    projection += corner;

    osg::Vec3d out;
    out = eye + projection;

    return out;
}

void SpaceballManipulator::goPerspective(const SpaceballOSGEvent *event)
{
    osg::Vec3d newEye, newCenter, newUp;
    newEye = spaceEye;
    newCenter = spaceCenter;
    newUp = spaceUp;

    //rotation point.
    osg::Vec3d rotationPoint;
    rotationPoint = newCenter;

    //derive motion factor.
    double gauge = projectionData.fovy;
    if (projectionData.aspectRatio > 1)
        gauge *= projectionData.aspectRatio;

    double rotateFactor = gauge * 0.000004;
    osg::Quat rx, ry, rz;
    rx.makeRotate(event->rotationX * rotateFactor, viewData.x);
    ry.makeRotate(event->rotationY * rotateFactor * -1.0d, viewData.y);
    rz.makeRotate(event->rotationZ * rotateFactor, viewData.z);
    newUp = rx * ry * rz * newUp;
    osg::Vec3d tempEye = newEye - rotationPoint;
    tempEye = rx * ry * rz * tempEye;
    newEye = tempEye + rotationPoint;

    double transFactor = gauge * .0002;
    osg::Vec3d transVec = viewData.x * (event->translationX * transFactor) +
            viewData.y * (event->translationY * transFactor * -1.0d) +
            viewData.z * (event->translationZ * transFactor);
    newEye = transVec + newEye;
    newCenter = transVec + newCenter;

    //move the center to half way point between the
    //new and far clipping planes.
    osg::Vec3d lookVector = newCenter - newEye;
    lookVector.normalize();
    lookVector *= (projectionData.far - projectionData.near) / 2.0d + projectionData.near;
    if (lookVector.length() > 0.0)
        newCenter = newEye + lookVector;

    spaceEye = newEye;
    spaceCenter = newCenter;
    spaceUp = newUp;
}

void SpaceballManipulator::setView(osg::Vec3d lookDirection, osg::Vec3d upDirection)
{
    if (projectionData.isCamOrtho)
    {
        spaceEye = -lookDirection * camSphere.radius() + camSphere.center();
        spaceCenter = camSphere.center();
        spaceUp = upDirection;
    }
}

void SpaceballManipulator::scaleFit()
{
    if (projectionData.isCamOrtho)
    {
        double viewportWidth = static_cast<double>(cam->getViewport()->width());
        double viewportHeight = static_cast<double>(cam->getViewport()->height());
        double aspectFactor =  viewportWidth / viewportHeight;

        if (aspectFactor > 1)
        {
            projectionData.bottom = boundingSphere.radius() * -1.0d;
            projectionData.top = boundingSphere.radius();
            projectionData.left = projectionData.bottom * aspectFactor;
            projectionData.right = projectionData.top * aspectFactor;
            projectionData.near = 0.1;
            projectionData.far = camSphere.radius() * 2;
        }
        else
        {
            projectionData.left = boundingSphere.radius() * -1.0d;
            projectionData.right = boundingSphere.radius();
            projectionData.bottom = projectionData.left / aspectFactor;
            projectionData.top = projectionData.right / aspectFactor;
            projectionData.near = 0.1;
            projectionData.far = camSphere.radius() * 2;
        }

        cam->setProjectionMatrixAsOrtho(projectionData.left, projectionData.right, projectionData.bottom,
                                        projectionData.top, projectionData.near, projectionData.far);
    }
}

void SpaceballManipulator::viewFit()
{
    osg::Vec3d moveVector = boundingSphere.center() - spaceCenter;
    spaceCenter += moveVector;
    spaceEye += moveVector;
    if (projectionData.isCamOrtho)
        spaceEye = projectToBound(spaceEye, spaceCenter);

    scaleFit();
}

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
#include <cassert>
#include <limits>

#include <osg/ComputeBoundsVisitor>

#include <modelviz/nodemaskdefs.h>
#include <viewer/spaceballosgevent.h>
#include <viewer/spaceballmanipulator.h>

using namespace vwr;

SpaceballManipulator::SpaceballManipulator(osg::Camera *camIn) : inherited(), boundingSphere(),
    cam(camIn), spaceEye(0.0, 0.0, 1.0), spaceCenter(0.0, 0.0, 0.0), spaceUp(0.0, 1.0, 0.0)
{
}

//don't know why I even have this copy constructor?
SpaceballManipulator::SpaceballManipulator
(
  const SpaceballManipulator& manipIn,
  const osg::CopyOp& copyOp
) : 
  osg::Object(manipIn, copyOp),
  osg::Callback(manipIn, copyOp),
  inherited(manipIn, copyOp),
  cam(manipIn.cam)
{
  boundingSphere = manipIn.boundingSphere;
  cam = manipIn.cam;
  spaceEye = manipIn.spaceEye;
  spaceCenter = manipIn.spaceCenter;
  spaceUp = manipIn.spaceUp;
}

SpaceballManipulator::~SpaceballManipulator() //need here for forward declarations
{

}

void SpaceballManipulator::init(const osgGA::GUIEventAdapter &, osgGA::GUIActionAdapter &us)
{
    us.requestContinuousUpdate(false);
}

void SpaceballManipulator::setByMatrix(const osg::Matrixd&)
{
//    matrix.getLookAt(spaceEye, spaceCenter, spaceUp);
}

void SpaceballManipulator::setByInverseMatrix(const osg::Matrixd&)
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

void SpaceballManipulator::setTransformation(const osg::Vec3d&, const osg::Quat&)
{
  assert(0); //not used.
}

void SpaceballManipulator::setTransformation(const osg::Vec3d&, const osg::Vec3d&, const osg::Vec3d&)
{
  assert(0); //not used.
}

void SpaceballManipulator::getTransformation(osg::Vec3d&, osg::Quat&) const
{
  assert(0); //not used.
}

void SpaceballManipulator::getTransformation(osg::Vec3d&, osg::Vec3d&, osg::Vec3d&) const
{
  assert(0); //not used.
}

void SpaceballManipulator::dump()
{
    std::cout << std::endl << "stats: " << std::endl <<
                 "   spaceEye: " << spaceEye.x() << "   " << spaceEye.y() << "   " << spaceEye.z() << std::endl <<
                 "   spaceCenter: " << spaceCenter.x() << "   " << spaceCenter.y() << "   " << spaceCenter.z() << std::endl <<
                 "   spaceUp: " << spaceUp.x() << "   " << spaceUp.y() << "   " << spaceUp.z() << std::endl;
}

void SpaceballManipulator::computeHomePosition(const osg::Camera *, bool useBoundingBox)
{
    if (useBoundingBox)
    {
      // (bounding box computes model center more precisely than bounding sphere)
      osg::ComputeBoundsVisitor cbVisitor(osg::ComputeBoundsVisitor::TRAVERSE_ACTIVE_CHILDREN);
      cbVisitor.setTraversalMask(~(mdv::csys));
      _node->accept(cbVisitor);
      osg::BoundingBox &boundingBox = cbVisitor.getBoundingBox();
      if (boundingBox.valid())
        boundingSphere = osg::BoundingSphere(boundingBox);
      else
        boundingSphere = _node->getBound();
    }
    else
      boundingSphere = _node->getBound();
    
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

void SpaceballManipulator::home(const osgGA::GUIEventAdapter&, osgGA::GUIActionAdapter& us)
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
        const spb::SpaceballOSGEvent *event = static_cast<const spb::SpaceballOSGEvent *>(ea.getUserData());
        if (!event || !cam || !_node.get())
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
    
    if (handleMouse(ea, us))
      return true;
    else
      return inherited::handle(ea, us);
}

bool SpaceballManipulator::handleMouse(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter &aa)
{
  using namespace osgGA;
  typedef GUIEventAdapter EA;
  
  //control key is going to have to be down before dragging occurs.
  //if a drag followed by the control key won't work.
  if (ea.getKey() & EA::KEY_Control_L)
  {
    if (ea.getEventType() == EA::KEYDOWN)
    {
      currentStateMask = ControlKey; //this resets the state.
      return true;
    }
    if (ea.getEventType() == EA::KEYUP)
    {
      currentStateMask.reset();
      return true;
    }
  }
  
  //nothing works unless the control is down.
  if ((currentStateMask & ControlKey).none())
    return false;
  
  //left button
  static double rotationFactor = 0.0;
  if (ea.getButton() == EA::LEFT_MOUSE_BUTTON)
  {
    if (ea.getEventType() == EA::PUSH)
    {
      currentStateMask |= LeftButton;
      lastScreenPoint = osg::Vec2f(ea.getX(), ea.getY());
      //setup rotation factor.
      //min of width and height of viewport make 1 complete rotation
      getProjectionData();
      rotationFactor = std::min(projectionData.width(), projectionData.height()) / (2.0 * osg::PI);
      
      return true;
    }
    if (ea.getEventType() == EA::RELEASE)
    {
      currentStateMask &= ~LeftButton;
      return true;
    }
  }
  
  //middle button
  if (ea.getButton() == EA::MIDDLE_MOUSE_BUTTON)
  {
    if (ea.getEventType() == EA::PUSH)
    {
      currentStateMask |= MiddleButton;
      lastScreenPoint = osg::Vec2f(ea.getX(), ea.getY());
      return true;
    }
    if (ea.getEventType() == EA::RELEASE)
    {
      currentStateMask &= ~MiddleButton;
      return true;
    }
  }
  
  //scroll wheel
  if (ea.getEventType() == EA::SCROLL)
  {
    if
    (
      (ea.getScrollingMotion() != EA::SCROLL_DOWN)
      && (ea.getScrollingMotion() != EA::SCROLL_UP)
    )
      return false;
      
    getProjectionData();
    
    static const double scrollFactor = 0.05;
    double directionFactor = 1.0; //assume scroll down.
    
    //establish 2 screen points. 1 will be viewport center and the other
    //will be the current cursor position. These points will be transformed
    //into world coordinates. The cursor position will be on the front clipping
    //plane for ortho views and be on the back clipping plane for perspective.
    //The camera will be shifted along a line constructed from both points.
    //implemented with perspective in mind but no testing has been done.
    osg::Vec3d worldPoint1, dummy1;
    windowToWorld(cam->getViewport()->width() / 2.0, cam->getViewport()->height() / 2.0, worldPoint1, dummy1);
    
    osg::Vec3d worldPoint2Front, worldPoint2Back;
    windowToWorld(ea.getX(), ea.getY(), worldPoint2Front, worldPoint2Back);
    
    osg::Vec3d worldPoint2 = worldPoint2Back; //assume perspective camera.
    
    if (ea.getScrollingMotion() == EA::SCROLL_UP)
      directionFactor = -1.0;
    
    if (projectionData.isCamOrtho)
    {
      projectionData.left -= projectionData.left * scrollFactor * directionFactor;
      projectionData.right -= projectionData.right * scrollFactor * directionFactor;
      projectionData.top -= projectionData.top * scrollFactor * directionFactor;
      projectionData.bottom -= projectionData.bottom * scrollFactor * directionFactor;
      
      cam->setProjectionMatrixAsOrtho(projectionData.left, projectionData.right, projectionData.bottom,
				      projectionData.top, projectionData.near, projectionData.far);
      worldPoint2 = worldPoint2Front;
    }
    
    osg::Vec3d cLine = worldPoint2 - worldPoint1;
    double mag = cLine.length() * scrollFactor; //5 percent along line.
    cLine.normalize();
    cLine *= mag * directionFactor;
    spaceEye += cLine;
    spaceCenter += cLine;
    aa.requestRedraw();
    return true;
  }
  
  //dragging
  if (ea.getEventType() == EA::DRAG)
  {
    if
    (
      (currentStateMask != Rotate)
      && (currentStateMask != Pan)
    )
      return false;
      
    osg::Vec3d oldStart, oldEnd;
    windowToWorld(lastScreenPoint.x(), lastScreenPoint.y(), oldStart, oldEnd);
    osg::Vec3d newStart, newEnd;
    windowToWorld(ea.getX(), ea.getY(), newStart, newEnd);
    osg::Vec3d movement = oldStart - newStart;
    double moveLength = movement.length();
    if (moveLength < static_cast<double>(std::numeric_limits<float>::epsilon()))
      return true;  
    
    if (currentStateMask == Rotate)
    {
      getViewData();
      
      //rotation axis will be normal to drag vector.
      osg::Quat dragRotation(osg::PI_2, viewData.z);
      osg::Vec3d axis = dragRotation * movement;
      axis.normalize();
      axis = -axis;
      
      osg::Quat viewRotation(moveLength / rotationFactor, axis);
      spaceEye = viewRotation * spaceEye;
      spaceUp = viewRotation * spaceUp;
    }
    
    if (currentStateMask == Pan)
    {
      spaceEye += movement;
      spaceCenter += movement;
    }
    
    lastScreenPoint = osg::Vec2f(ea.getX(), ea.getY());
    aa.requestRedraw();
    return true;
  }

  return false;
}

bool SpaceballManipulator::windowToWorld(float xIn, float yIn, osg::Vec3d &startOut, osg::Vec3d &endOut) const
{
  //build matrix.
  osg::Matrixd m = cam->getViewMatrix();
  m.postMult(cam->getProjectionMatrix()); //shouldn't be necessary as working with only ortho.
  m.postMult(cam->getViewport()->computeWindowMatrix());
  osg::Matrixd i = osg::Matrixd::inverse(m);
  startOut = osg::Vec3d(xIn, yIn, 0.0) * i;
  endOut = osg::Vec3d(xIn, yIn, 1.0) * i;
  return true;
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

void SpaceballManipulator::goOrtho(const spb::SpaceballOSGEvent *event)
{
    osg::Vec3d newEye, newCenter, newUp;
    newEye = spaceEye;
    newCenter = spaceCenter;
    newUp = spaceUp;

    //rotation point.
    osg::Vec3d rotationPoint;
    rotationPoint = newCenter;

    //derive motion factor.
    double gauge = std::min(projectionData.width(), projectionData.height()) / 512.0d;

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

void SpaceballManipulator::goPerspective(const spb::SpaceballOSGEvent *event)
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

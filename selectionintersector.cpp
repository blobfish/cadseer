#include <iostream>

#include <osg/Geometry>
#include <osg/PrimitiveSet>

#include "selectionintersector.h"

using namespace osg;

SelectionIntersector::SelectionIntersector(CoordinateFrame frame, double x, double y) :
    LineSegmentIntersector(frame, x, y), pickRadius(1.0)
{

}

SelectionIntersector::SelectionIntersector(const osg::Vec3& startIn, const osg::Vec3& endIn) :
    LineSegmentIntersector(startIn, endIn), pickRadius(1.0)
{

}


osgUtil::Intersector* SelectionIntersector::clone(osgUtil::IntersectionVisitor &iv)
{
    if ( _coordinateFrame==MODEL && iv.getModelMatrix()==0 )
    {
        osg::ref_ptr<SelectionIntersector> cloned = new SelectionIntersector( _start, _end );
        cloned->_parent = this;
        cloned->pickRadius = this->pickRadius;
        return cloned.release();
    }

    osg::Matrix matrix;
    switch ( _coordinateFrame )
    {
    case WINDOW:
        if (iv.getWindowMatrix()) matrix.preMult( *iv.getWindowMatrix() );
        if (iv.getProjectionMatrix()) matrix.preMult( *iv.getProjectionMatrix() );
        if (iv.getViewMatrix()) matrix.preMult( *iv.getViewMatrix() );
        if (iv.getModelMatrix()) matrix.preMult( *iv.getModelMatrix() );
        break;
    case PROJECTION:
        if (iv.getProjectionMatrix()) matrix.preMult( *iv.getProjectionMatrix() );
        if (iv.getViewMatrix()) matrix.preMult( *iv.getViewMatrix() );
        if (iv.getModelMatrix()) matrix.preMult( *iv.getModelMatrix() );
        break;
    case VIEW:
        if (iv.getViewMatrix()) matrix.preMult( *iv.getViewMatrix() );
        if (iv.getModelMatrix()) matrix.preMult( *iv.getModelMatrix() );
        break;
    case MODEL:
        if (iv.getModelMatrix()) matrix = *iv.getModelMatrix();
        break;
    }

    osg::Matrix inverse = osg::Matrix::inverse(matrix);
    osg::ref_ptr<SelectionIntersector> cloned = new SelectionIntersector( _start*inverse, _end*inverse );
    cloned->_parent = this;
    cloned->pickRadius = this->pickRadius;
    return cloned.release();
}

void SelectionIntersector::intersect(osgUtil::IntersectionVisitor &iv, osg::Drawable *drawable)
{
    currentGeometry = drawable->asGeometry();
    if (!currentGeometry)
        return;

    currentVertices = dynamic_cast<Vec3Array *>(currentGeometry->getVertexArray());
    if (!currentVertices)
        return;

    setScale(iv);
    if (isnan(scale))
        return;
    if (!getLocalStartEnd())
        return;
    if (iv.getDoDummyTraversal())//not sure what this does.
        return;

    Geometry::PrimitiveSetList set = currentGeometry->getPrimitiveSetList();
    Geometry::PrimitiveSetList::const_iterator setIt;
    Intersection hitBase;
    hitBase.nodePath = iv.getNodePath();
    hitBase.drawable = drawable;
    hitBase.matrix = iv.getModelMatrix();
    for (setIt = set.begin(); setIt != set.end(); ++setIt)
    {
        hitBase.primitiveIndex = setIt - set.begin();
        if ((*setIt)->getMode() == GL_POINTS)
        {
            goPoints(*setIt, hitBase);
        }
//        if ((*setIt)->getMode() == GL_LINE_STRIP)
//            hasEdges = true;
        if ((*setIt)->getMode() == GL_TRIANGLES)
            LineSegmentIntersector::intersect(iv, drawable);
    }
}

void SelectionIntersector::goPoints(const osg::ref_ptr<osg::PrimitiveSet> primitive, Intersection &hitBase)
{
    ref_ptr<DrawArrays> drawArray = dynamic_pointer_cast<DrawArrays>(primitive);
    if (!drawArray.valid())
    {
        std::cout << "couldn't do cast in goPoints";
        return;
    }

    Vec3d currentVertex = currentVertices->at(drawArray->getFirst());
    Vec3d intersectionVector = localEnd - localStart;
    Vec3d vertexVector = currentVertex - localStart;
    double dot = vertexVector * intersectionVector;
    Vec3d projectionVector =  intersectionVector * (dot / intersectionVector.length2());
    projectionVector += localStart;
    double distance = (projectionVector - currentVertex).length();

    if (isnan(distance) || distance <= pickRadius / scale)
    {
//        std::cout << "distance: " << distance << std::endl;
        Intersection hit = hitBase;
        hit.ratio = distance;
        hit.localIntersectionPoint = currentVertex;
        insertIntersection(hit);
    }
}

void SelectionIntersector::setScale(osgUtil::IntersectionVisitor &iv)
{
    Matrixd pMatrix = *iv.getProjectionMatrix();
    Matrixd wMatrix = *iv.getWindowMatrix();
    scale = (pMatrix * wMatrix).getScale().length();
}

bool SelectionIntersector::getLocalStartEnd()
{
    localStart = _start;
    localEnd = _end;
    BoundingBox box = currentGeometry->getBound();
    Vec3d cornerProject(1.0d, 1.0d, 1.0d);
    cornerProject.normalize();
    box._min = box._min + (cornerProject * -pickRadius / scale);
    box._max = box._max + (cornerProject * pickRadius / scale);
    if (!intersectAndClip(localStart, localEnd, box))
        return false;
    return true;
}

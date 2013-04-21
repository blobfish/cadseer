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
    Geometry *geometry = drawable->asGeometry();
    if (!geometry)
        return;

    bool hasPoints = false;
    bool hasEdges = false;
    bool hasFaces = false;

    Geometry::PrimitiveSetList set = geometry->getPrimitiveSetList();
    Geometry::PrimitiveSetList::const_iterator setIt;
    for (setIt = set.begin(); setIt != set.end(); ++setIt)
    {
        if ((*setIt)->getMode() == GL_POINTS)
            hasPoints = true;
        if ((*setIt)->getMode() == GL_LINE_STRIP)
            hasEdges = true;
        if ((*setIt)->getMode() == GL_TRIANGLES)
            hasFaces = true;
    }

    if (hasFaces)
        LineSegmentIntersector::intersect(iv, drawable);

    if (!hasEdges && !hasPoints)
        return;

    Matrixd pMatrix = *iv.getProjectionMatrix();
    Matrixd wMatrix = *iv.getWindowMatrix();
    double scale = (pMatrix * wMatrix).getScale().length();

    BoundingBox box = drawable->getBound();
    Vec3d cornerProject(1.0d, 1.0d, 1.0d);
    cornerProject.normalize();
    box._min = box._min + (cornerProject * -pickRadius / scale);
    box._max = box._max + (cornerProject * pickRadius / scale);

    Vec3d localStart(_start);
    Vec3d localEnd(_end);
    if (!intersectAndClip(localStart, localEnd, box))
        return;
    if (iv.getDoDummyTraversal())//not sure what this does.
        return;

    Vec3Array *vertices = dynamic_cast<Vec3Array *>(geometry->getVertexArray());
    if (!vertices)
        return;

    Vec3d intersectionVector = localEnd - localStart;
    Vec3Array::const_iterator it;
    for (it = vertices->begin(); it != vertices->end(); ++it)
    {
        Vec3d vertexVector = (*it) - localStart;
        double dot = vertexVector * intersectionVector;
        double distance = sqrt(vertexVector.length2() - pow(dot, 2));

        if (isnan(distance) || distance <= pickRadius / scale)
        {
            Intersection hit;
            hit.ratio = distance;
            hit.nodePath = iv.getNodePath();
            hit.drawable = drawable;
            hit.matrix = iv.getModelMatrix();
            hit.localIntersectionPoint = *it;
            insertIntersection(hit);
        }
    }
}

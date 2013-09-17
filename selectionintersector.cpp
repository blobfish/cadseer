#include <iostream>
#include <assert.h>

#include <osg/Geometry>
#include <osg/PrimitiveSet>

#include <eigen3/Eigen/Eigen>

#include "selectionintersector.h"
#include "./testing/plotter.h"

using namespace osg;
using namespace Eigen;

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
    assert(currentGeometry);

    currentVertices = dynamic_cast<Vec3Array *>(currentGeometry->getVertexArray());
    assert(currentVertices);

    setScale(iv);
    if (isnan(scale))
        return;
    if (!getLocalStartEnd())
        return;
    if (iv.getDoDummyTraversal())//not sure what this does.
        return;


    //doing primitive sets are overkill now because we have only object(vertex, line, edge) per geom node.
    //I am leaving in case we go back to a merged geom node.
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
            goPoints(*setIt, hitBase);
        if ((*setIt)->getMode() == GL_LINE_STRIP)
            goEdges(*setIt, hitBase);
        if ((*setIt)->getMode() == GL_TRIANGLES)
            LineSegmentIntersector::intersect(iv, drawable);
    }
}

void SelectionIntersector::goPoints(const osg::ref_ptr<osg::PrimitiveSet> primitive, const Intersection &hitBase)
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
        hit.ratio = (projectionVector - _start).length() / ((_end - _start).length() * 1.02);
        hit.localIntersectionPoint = currentVertex;
        insertIntersection(hit);
    }
}

void SelectionIntersector::goEdges(const osg::ref_ptr<osg::PrimitiveSet> primitive, const Intersection &hitBase)
{
    ref_ptr<DrawArrays> drawArray = dynamic_pointer_cast<DrawArrays>(primitive);
    assert(drawArray);

    int first = drawArray->getFirst();
    int count = drawArray->getCount();
    for (int index = first; index < first + count - 1; ++index)
    {
        Vec3d lineStart = currentVertices->at(index);
        Vec3d lineEnd =  currentVertices->at(index + 1);
        Vec3d segmentVector = lineEnd - lineStart;

        Vec3d intersectionVector = localEnd - localStart;
        Vec3d tempIntersectVector = intersectionVector + (localEnd - lineStart);

        Vec3d testPoint;
        double segmentDot = segmentVector * tempIntersectVector;
        double segmentScalar = segmentDot / segmentVector.length2();
        testPoint = segmentVector * segmentScalar + lineStart;

        Vec3d connection = localStart - lineStart;
        Eigen::Matrix2d matrix;
        Vector2d solution;
        matrix << segmentVector * segmentVector, - (segmentVector * intersectionVector),
                intersectionVector * segmentVector, - (intersectionVector * intersectionVector);
        solution << segmentVector * connection, intersectionVector * connection;
        FullPivLU<Eigen::Matrix2d> lu1(matrix);
        Vector2d parameters = lu1.solve(solution);
        if(lu1.rank() != matrix.cols())
            continue;

        Vec3d segmentPoint = lineStart + (segmentVector * parameters.x());
        if (parameters.x() < 0)
            segmentPoint = lineStart;

        if (parameters.x() > 1)
            segmentPoint = lineEnd;

        Vec3d intersectionPoint = localStart + (intersectionVector * parameters.y());

//        Plotter::getReference().plotPoint(segmentPoint);
//        Plotter::getReference().plotPoint(intersectionPoint);

        double distance = (segmentPoint - intersectionPoint).length();
//        std::cout << "distance: " << distance << std::endl;
        if (isnan(distance) || distance <= pickRadius / scale)
        {
            Intersection hit = hitBase;
            hit.ratio = (segmentPoint - _start).length() / ((_end - _start).length() * 1.01);
            hit.localIntersectionPoint = segmentPoint;
            insertIntersection(hit);
            break;//only one intersection for primitive.
        }
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
    box._min = box._min + (cornerProject * -pickRadius / scale);
    box._max = box._max + (cornerProject * pickRadius / scale);
    if (!intersectAndClip(localStart, localEnd, box))
        return false;
    return true;
}

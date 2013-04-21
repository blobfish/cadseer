#ifndef SELECTIONINTERSECTOR_H
#define SELECTIONINTERSECTOR_H

#include <osgUtil/LineSegmentIntersector>

class SelectionIntersector : public osgUtil::LineSegmentIntersector
{
public:
    SelectionIntersector(CoordinateFrame frame, double x, double y);
    SelectionIntersector(const osg::Vec3& startIn, const osg::Vec3& endIn);

    virtual Intersector* clone(osgUtil::IntersectionVisitor &iv);
    virtual void intersect(osgUtil::IntersectionVisitor &iv, osg::Drawable *drawable);

    void setPickRadius(const double &radiusIn){pickRadius = radiusIn;}
    double getPickRadius(){return pickRadius;}

protected:
    double pickRadius;
};

#endif // SELECTIONINTERSECTOR_H

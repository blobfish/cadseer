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
    void setScale(osgUtil::IntersectionVisitor &iv);
    bool getLocalStartEnd();
    void goPoints(const osg::ref_ptr<osg::PrimitiveSet> primitive, Intersection &hitBase);
    double scale;
    double pickRadius;
    osg::Geometry *currentGeometry;
    osg::Vec3Array *currentVertices;
    osg::Vec3d localStart;
    osg::Vec3d localEnd;
};

#endif // SELECTIONINTERSECTOR_H

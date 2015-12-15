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

#ifndef SELECTIONINTERSECTOR_H
#define SELECTIONINTERSECTOR_H

#include <osgUtil/LineSegmentIntersector>

namespace slc
{
/* Intersection primitive index is for the triangles and not the primitive sets.
 * shape geometry has each face in it owns primitive set so I need
 * change the primitive index from triangle index to primitive set index
 * I couldn't modify the results returned from getIntersections, so
 * I will create my own intersections variable.
 */ 
 
class Intersector : public osgUtil::LineSegmentIntersector
{
public:
    Intersector(CoordinateFrame frame, double x, double y);
    Intersector(const osg::Vec3& startIn, const osg::Vec3& endIn);

    virtual osgUtil::Intersector* clone(osgUtil::IntersectionVisitor &iv) override;
    virtual void intersect(osgUtil::IntersectionVisitor &iv, osg::Drawable *drawable) override;
    virtual bool containsIntersections() override{return !myIntersections.empty();}

    void setPickRadius(const double &radiusIn){pickRadius = radiusIn;}
    double getPickRadius(){return pickRadius;}
    void insertMyIntersection(const osgUtil::LineSegmentIntersector::Intersection &in);
    const Intersections& getMyIntersections(){return myIntersections;}

protected:
    void setScale(osgUtil::IntersectionVisitor &iv);
    bool getLocalStartEnd();
    void goPoints(const osg::ref_ptr<osg::PrimitiveSet> primitive, const Intersection &hitBase);
    void goEdges(const osg::ref_ptr<osg::PrimitiveSet> primitive, const Intersection &hitBase);
    osg::BoundingSphere buildBoundingSphere(const osg::Vec3d& start, const osg::Vec3d& end);
    osg::BoundingSphere segmentSphere;
    double scale = 1.0;
    double pickRadius = 1.0;
    osg::Geometry *currentGeometry = nullptr;
    osg::Vec3Array *currentVertices = nullptr;
    osg::Vec3d localStart;
    osg::Vec3d localEnd;
    Intersections myIntersections;
};
}

#endif // SELECTIONINTERSECTOR_H

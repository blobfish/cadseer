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

namespace Selection
{
class Intersector : public osgUtil::LineSegmentIntersector
{
public:
    Intersector(CoordinateFrame frame, double x, double y);
    Intersector(const osg::Vec3& startIn, const osg::Vec3& endIn);

    virtual osgUtil::Intersector* clone(osgUtil::IntersectionVisitor &iv) override;
    virtual void intersect(osgUtil::IntersectionVisitor &iv, osg::Drawable *drawable);

    void setPickRadius(const double &radiusIn){pickRadius = radiusIn;}
    double getPickRadius(){return pickRadius;}

protected:
    void setScale(osgUtil::IntersectionVisitor &iv);
    bool getLocalStartEnd();
    void goPoints(const osg::ref_ptr<osg::PrimitiveSet> primitive, const Intersection &hitBase);
    void goEdges(const osg::ref_ptr<osg::PrimitiveSet> primitive, const Intersection &hitBase);
    double scale;
    double pickRadius;
    osg::Geometry *currentGeometry;
    osg::Vec3Array *currentVertices;
    osg::Vec3d localStart;
    osg::Vec3d localEnd;
};
}

#endif // SELECTIONINTERSECTOR_H

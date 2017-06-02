/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef SLC_INTERSECTION_H
#define SLC_INTERSECTION_H

#include <osgUtil/LineSegmentIntersector>
#include <osgUtil/PolytopeIntersector>

namespace osg{class Drawable; class RefMatrixd;}


namespace slc
{
  /*! @brief Intersection data for selection
   *
   * The osgUtil LineSegment and PolyTope intersector have different
   * intersection structures. This class acts as an abstraction to those
   * intersection classes. This way we can change intersectors or use 
   * both with little impact.
   */
  class Intersection
  {
  public:
    Intersection();
    Intersection(const osgUtil::LineSegmentIntersector::Intersection &);
    Intersection(const osgUtil::PolytopeIntersector::Intersection &);
    ~Intersection();
    
    osg::NodePath nodePath;
    osg::ref_ptr<osg::Drawable> drawable;
    osg::ref_ptr<osg::RefMatrixd> matrix;
    osg::Vec3d localIntersectionPoint;
    unsigned int primitiveIndex;
    
    //possible boost::variant containing the actual object.
  };
  
  typedef std::vector<Intersection> Intersections;
  Intersections& append(Intersections&, const osgUtil::LineSegmentIntersector::Intersections&);
  Intersections& append(Intersections&, const osgUtil::PolytopeIntersector::Intersections&);
}

#endif // SLC_INTERSECTION_H

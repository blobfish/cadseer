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

#ifndef LBR_CIRCLEBUILDER_H
#define LBR_CIRCLEBUILDER_H

#include <cstdlib> //for std::size_t

#include <osg/Math> //for pi.

namespace osg{class Vec3d; class Geometry;}

namespace lbr
{

/*! @brief class for constructing a circle of points with OpenSceneGraph
 * 
 * Point density can be controlled by ::setSegments or ::setDeviation.
 * Setting by segments will give a consistent mesh regardless
 * of radius. Setting by deviation will give a finer mesh as
 * the radius increases. Deviation calculation uses the radius to 
 * calculate the quantity of points,so the radius should be set
 * before calling setDeviation. Each call to the coversion operator
 * generates a new dynamically allocated osg::Geometry*, so the Circle
 * builder object can be reused. Note that when building a complete circle
 * i.e. setAngularSpan(360) the default, that the last point is excluded.
 * This prevents the duplication of the first and last point. 
 * 
 * Starting place:
 * a value of 16 for setSegments gives an average dense mesh.
 * a value of 0.020 for setDeviation and a radius of 1.0 gives
 * an average dense mesh.
 * 
 * Usage:
 * \code{.h}
 * CircleBuilder builder;
 * builder.setRadius(10.0);
 * builder.setAngularSpan(90.0) //in degrees
 * builder.setDeviation(0.020);
 * osg::Group *group = new osg::Group();
 * group->addChild(builder);
 * \endcode
 * or
 * \code{.h}
 * CircleBuilder builder;
 * builder.setRadius(10.0);
 * builder.setAngularSpan(360.0) //in degrees
 * builder.setSegments(32);
 * std::vector<osg::Vec3d> points = builder;
 * \endcode
 */
class CircleBuilder
{
public:
  void setRadius(double); //!< min is 0.01
  void setSegments(std::size_t); //!< min is 3
  void setDeviation(double); //!< set segments by a deviation calculation.
  void setAngularSpanDegrees(double); //!< set angularSpan from degrees.
  void setAngularSpanRadians(double); //!< set angularSpan from degrees.
  bool isCompleteCircle() const; //!< determin if we have a complete circle
  
  operator std::vector<osg::Vec3d> () const; //!< get result points.
  operator osg::Geometry* () const; //!< dynamically allocated. User responsible for delete.
protected:
  std::size_t segments = 16; //!< default value of 16, min 3.
  double angularSpan = 2.0 * osg::PI; //!< in radians, default 2PI, full circle.
  double radius = 1.0; //!< default value of 1.0
};
}

#endif // LBR_CIRCLEBUILDER_H

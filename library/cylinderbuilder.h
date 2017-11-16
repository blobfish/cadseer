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

#ifndef LBR_CYLINDERBUILDER_H
#define LBR_CYLINDERBUILDER_H

#include <cstdlib> //for std::size_t

#include <osg/Math> //for pi.

namespace lbr
{

/*! @brief class for constructing a cylinder with OpenSceneGraph
 * 
 * Mesh density can be controlled by ::setIsoLines or ::setDeviation.
 * Setting by Isolines will give a consistent mesh regardless
 * of radius. Setting by deviation will give a finer mesh as
 * the radius increases. Deviation calculation uses the radius to 
 * calculate the quantity of iso lines,so the radius should be set
 * before calling setDeviation. Each call to the coversion operator
 * generates a new dynamically allocated osg::Geometry*, so the cylinder
 * builder object can be reused.
 * 
 * Usage:
 * \code{.h}
 * CylinderBuilder cyBuilder;
 * cyBuilder.setIsoLines(8);
 * cyBuilder.setRadius(2.0);
 * cyBuilder.setHeight(16.0);
 * cyBuilder.setAngularSpanDegrees(360.0);
 * osg::Group *group = new osg::Group();
 * group->addChild(builder);
 * \endcode
 */
class CylinderBuilder
{
public:
  void setIsoLines(std::size_t); //!< min is 3
  void setAngularSpanDegrees(double);
  void setAngularSpanRadians(double);
  void setRadius(double); //!< min is 0.001
  void setHeight(double); //!< min is 0.001
  void setDeviation(double);
  
  operator osg::Geometry* () const; //!< dynamically allocated. User responsible for delete.
protected:
  std::size_t isoLines = 16; //!< default value of 16, min 3.
  double angularSpan = 2.0 * osg::PI; //!< in radians, default 2PI, full circle.
  double radius = 1.0; //!< default value of 1.0
  double height = 10.0; //!< default value of 10.0
};
}

#endif // LBR_CYLINDERBUILDER_H

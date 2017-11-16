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

#ifndef LBR_CONEBUILDER_H
#define LBR_CONEBUILDER_H

#include <cstdlib> //for std::size_t

#include <osg/Math> //for pi.

namespace lbr
{

/*! @brief class for constructing a cone with OpenSceneGraph
 * 
 * Mesh density can be controlled by ::setIsoLines or ::setDeviation.
 * Setting by Isolines will give a consistent mesh regardless
 * of radius. Setting by deviation will give a finer mesh as
 * the radius increases. Deviation calculation uses the radius to 
 * calculate the quantity of iso lines,so the radius should be set
 * before calling setDeviation. Each call to the coversion operator
 * generates a new dynamically allocated osg::Geometry*, so the cone
 * builder object can be reused.
 * 
 * Usage:
 * \code{.h}
 * ConeBuilder coneBuilder;
 * coneBuilder.setRadius(2.0);
 * coneBuilder.setHeight(10.0);
 * coneBuilder.setIsoLines(32);
 * osg::Group *group = new osg::Group();
 * group->addChild(coneBuilder);
 * \endcode
 */
class ConeBuilder
{
public:
  void setIsoLines(std::size_t); //!< min is 3
  void setDeviation(double deviationIn);
  void setRadius(double radiusIn);
  void setHeight(double heightIn);
  
  operator osg::Geometry* () const; //!< dynamically allocated. User responsible for delete.
protected:
  std::size_t isoLines = 16; //!< default value of 16, min 3.
  double radius = 0.2;
  double height = 1.0;
};
}

#endif // LBR_CONEBUILDER_H

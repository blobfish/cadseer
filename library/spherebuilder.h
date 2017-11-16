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

#ifndef LBR_SPHEREBUILDER_H
#define LBR_SPHEREBUILDER_H

#include <cstdlib> //for std::size_t

namespace osg{class Geometry;}

namespace lbr
{

/*! @brief class for constructing a sphere with OpenSceneGraph
 * 
 * Mesh density can be controlled by ::setIsoLines or ::setDeviation.
 * Setting by Isolines will give a consistent mesh regardless
 * of radius. Setting by deviation will give a finer mesh as
 * the radius increases. Deviation calculation uses the radius to 
 * calculate the quantity of iso lines,so the radius should be set
 * before calling setDeviation. Each call to the coversion operator
 * generates a new dynamically allocated osg::Geometry*, so the Sphere
 * builder object can be reused.
 * 
 * Starting place:
 * a value of 16 for setIsoLines gives an average dense mesh.
 * a value of 0.020 for setDeviation and a radius of 1.0 gives
 * an average dense mesh.
 * 
 * Usage:
 * \code{.h}
 * SphereBuilder builder;
 * builder.setRadius(10.0);
 * builder.setDeviation(0.020);
 * osg::Group *group = new osg::Group();
 * group->addChild(builder);
 * \endcode
 * or
 * \code{.h}
 * SphereBuilder builder;
 * builder.setRadius(10.0);
 * builder.setIsoLines(32);
 * osg::Group *group = new osg::Group();
 * group->addChild(builder);
 * \endcode
 */
class SphereBuilder
{
public:
  void setRadius(double radiusIn); //!< min is 0.01
  void setIsoLines(std::size_t isoLinesIn);
  void setDeviation(double deviationIn); //!< set isoLines by a deviation calculation.
  
  operator osg::Geometry* () const; //!< dynamically allocated. User responsible for delete.
protected:
  std::size_t isoLines = 16; //!< default value of 16
  double radius = 1.0; //!< default value of 1.0
};
}
#endif // LBR_SPHEREBUILDER_H

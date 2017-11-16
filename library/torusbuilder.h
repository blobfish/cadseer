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

#ifndef LBR_TORUSBUILDER_H
#define LBR_TORUSBUILDER_H

#include <cstdlib> //for std::size_t

#include <osg/Math> //for pi.

namespace osg{class Geometry;}

namespace lbr
{

/*! @brief class for constructing a torus with OpenSceneGraph
 * 
 * Point density can be controlled by ::setMajorIsoLines and
 * ::setMinorIsoLines or ::setDeviation.
 * Setting by *isoLines will give a consistent mesh regardless
 * of radei. Setting by deviation will give a finer mesh as
 * the radei increases. Deviation calculation uses the radei to 
 * calculate the quantity of points,so the radei should be set
 * before calling setDeviation. Each call to the coversion operator
 * generates a new dynamically allocated osg::Geometry*, so the Torus
 * builder object can be reused. 
 * 
 * Starting place:
 * a value of 16 for setSegments gives an average dense mesh.
 * a value of 0.020 for setDeviation and a radius of 1.0 gives
 * an average dense mesh.
 * 
 * Usage:
 * \code{.h}
 * TorusBuilder tBuilder;
 * tBuilder.setMajorRadius(1.0);
 * tBuilder.setMajorIsoLines(16);
 * tBuilder.setMinorRadius(0.2);
 * tBuilder.setMinorIsoLines(8);
 * tBuilder.setAngularSpanDegrees(360.0);
 * osg::Geometry *torus = tBuilder;
 * \endcode
 */
class TorusBuilder
{
public:
  void setMajorRadius(double); //!< min is 0.01
  void setMajorIsoLines(std::size_t); //!< min is 3
  void setMinorRadius(double); //!< min is 0.001
  void setMinorIsoLines(std::size_t); //!< min is 3
  void setAngularSpanDegrees(double); //in degrees. will be converted.
  void setAngularSpanRadians(double);
  void setDeviation(double);
  
  operator osg::Geometry* () const; //!< dynamically allocated. User responsible for delete.
protected:
  double majorRadius = 1.0; //!< radius of circle. Default 1.0
  std::size_t majorIsoLines = 16; //!< number of segments for majorRadius. Default 16
  double minorRadius = 0.05; //!< radius of 'section'. Default 0.05
  std::size_t minorIsoLines = 8; //!< number of segments for minorRadius. Default 8
  double angularSpan = 2.0 * osg::PI; //!< span in radians. Default 2PI, full circle.
  
  double calculateAngle(double, double) const; //!< calculate angle needed for deviation given radius.
  
};
}

#endif // LBR_TORUSBUILDER_H

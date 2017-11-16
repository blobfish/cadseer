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

#ifndef LBR_ICONBUILDER_H
#define LBR_ICONBUILDER_H

#include <osg/Vec3d>
#include <osg/Image>

namespace lbr
{

class IconBuilder
{
public:
  IconBuilder(osg::Image *);
  operator osg::Geometry* () const; //!< dynamically allocated. User responsible for delete.
  void setLowerLeftCorner(const osg::Vec3d &cornerIn){lowerLeftCorner = cornerIn;}
  void setXVector(const osg::Vec3d &xVectorIn){xVector = xVectorIn;}
  void setYVector(const osg::Vec3d &yVectorIn){yVector = yVectorIn;}
protected:
  osg::ref_ptr<osg::Image> image;
  osg::Vec3d lowerLeftCorner = osg::Vec3d(-0.5, -0.5, 0.0);
  osg::Vec3d xVector = osg::Vec3d(1.0, 0.0, 0.0);
  osg::Vec3d yVector = osg::Vec3d(0.0, 1.0, 0.0);
  
};
}

#endif // LBR_ICONBUILDER_H

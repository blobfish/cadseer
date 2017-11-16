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

#ifndef LBR_CIRCLEPROJECTOR_H
#define LBR_CIRCLEPROJECTOR_H

#include <osgManipulator/Projector>

namespace lbr
{

class CircleProjector : public osgManipulator::Projector
{
  public:
    CircleProjector();
    
    void setRadius(double radiusIn){radius = radiusIn;}
    double getRadius(){return radius;}
    
    virtual bool project(const osgManipulator::PointerInfo& pi, osg::Vec3d& projectedPoint) const override;
    
protected:
  double radius;
};
}

#endif //LBR_CIRCLEPROJECTOR_H

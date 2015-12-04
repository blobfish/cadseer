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

#include "circleprojector.h"

using namespace osg;
using namespace osgManipulator;

CircleProjector::CircleProjector() : radius(1.0)
{
  
}

bool CircleProjector::project(const PointerInfo& pi, osg::Vec3d& projectedPoint) const
{
  ref_ptr<PlaneProjector> planeProjector = new PlaneProjector(Plane(Vec3d(0.0, 0.0, 1.0), Vec3d(0.0, 0.0, 0.0)));
  planeProjector->setLocalToWorld(getLocalToWorld());
  if (!planeProjector->project(pi, projectedPoint))
    return false;
  
  projectedPoint.normalize();
  projectedPoint *= radius;

  return true;
}
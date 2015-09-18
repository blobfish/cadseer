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

#include <osg/AutoTransform>

#ifndef COORDINATESYSTEM_H
#define COORDINATESYSTEM_H

namespace CoordinateSystem
{
  osg::Node* buildCoordinateSystemNode();
  osg::Node* buildArrow(const double &size, const osg::Vec4 &color);
  osg::Node* buildPlane(const double &size, const osg::Vec4 &color);
};

#endif // COORDINATESYSTEM_H

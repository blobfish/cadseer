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

#ifndef GESTURENODE_H
#define GESTURENODE_H

namespace osg
{
class MatrixTransform;
class Geode;
}

namespace gsn
{
osg::MatrixTransform* buildMenuNode(const char *resourceName);
osg::MatrixTransform* buildCommandNode(const char *resourceName);
osg::MatrixTransform* buildCommonNode(const char *resourceName);
osg::Geode* buildIconGeode(const char *resourceName);
osg::Geode* buildLineGeode();
}

#endif // GESTURENODE_H

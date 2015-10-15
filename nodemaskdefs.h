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

#ifndef NODEMASKDEFS_H
#define NODEMASKDEFS_H

//from osg::Node documentation:
//The default value is 0xffffffff (all bits set).
//typedef unsigned int NodeMask; size of 32.

namespace NodeMaskDef
{
    static const unsigned int mainCamera =              1 << 0;
    static const unsigned int backGroundCamera =        1 << 1;
    static const unsigned int gestureCamera =           1 << 2;   //also used on gesture background.

    static const unsigned int point =                   1 << 3;   //point refers to temporary snap points.
    static const unsigned int vertex =                  1 << 4;   //vertex refers to actual geometric objects. not used right now.
    static const unsigned int edge =                    1 << 5;
    static const unsigned int face =                    1 << 6;
    static const unsigned int lod =                     1 << 7;
    static const unsigned int object =                  1 << 8;
    static const unsigned int csys =                    1 << 9;

    static const unsigned int gestureMenu =             1 << 10;
    static const unsigned int gestureCommand =          1 << 11;
}


#endif // NODEMASKDEFS_H

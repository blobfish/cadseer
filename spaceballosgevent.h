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

#ifndef SPACEBALLOSGEVENT_H
#define SPACEBALLOSGEVENT_H

#include <osg/Referenced>

class SpaceballOSGEvent : public osg::Referenced
{
public:
    enum Type {TNone, Motion, Button};
    enum ButtonState{BNone, Pressed, Released};
    SpaceballOSGEvent();
    virtual ~SpaceballOSGEvent(){}

    Type theType;
    ButtonState theButtonState;
    int buttonNumber;
    int translationX;
    int translationY;
    int translationZ;
    int rotationX;
    int rotationY;
    int rotationZ;
};

#endif // SPACEBALLOSGEVENT_H

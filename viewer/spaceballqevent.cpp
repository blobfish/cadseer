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

#include <viewer/spaceballqevent.h>

using namespace vwr;

int vwr::MotionEvent::Type = -1;
int vwr::ButtonEvent::Type = -1;

void vwr::registerEvents()
{
    vwr::MotionEvent::Type = QEvent::registerEventType();
    vwr::ButtonEvent::Type = QEvent::registerEventType();
}

EventBase::EventBase(QEvent::Type event) : QInputEvent(static_cast<QEvent::Type>(event))
{

}

MotionEvent::MotionEvent() : EventBase(static_cast<QEvent::Type>(Type)),
    xTrans(0), yTrans(0), zTrans(0), xRot(0), yRot(0), zRot(0)
{
}

MotionEvent::MotionEvent(const MotionEvent& in) : EventBase(static_cast<QEvent::Type>(Type))
{
    xTrans  = in.xTrans;
    yTrans  = in.yTrans;
    zTrans  = in.zTrans;
    xRot    = in.xRot;
    yRot    = in.yRot;
    zRot    = in.zRot;
    myHandled = in.myHandled;
}

void MotionEvent::translations(int &xTransOut, int &yTransOut, int &zTransOut)
{
    xTransOut = xTrans;
    yTransOut = yTrans;
    zTransOut = zTrans;
}

void MotionEvent::setTranslations(const int &xTransIn, const int &yTransIn, const int &zTransIn)
{
    xTrans = xTransIn;
    yTrans = yTransIn;
    zTrans = zTransIn;
}

void MotionEvent::rotations(int &xRotOut, int &yRotOut, int &zRotOut)
{
    xRotOut = xRot;
    yRotOut = yRot;
    zRotOut = zRot;
}

void MotionEvent::setRotations(const int &xRotIn, const int &yRotIn, const int &zRotIn)
{
    xRot = xRotIn;
    yRot = yRotIn;
    zRot = zRotIn;
}


ButtonEvent::ButtonEvent() : EventBase(static_cast<QEvent::Type>(Type)),
    buttonState(BUTTON_NONE), button(0)
{
}

ButtonEvent::ButtonEvent(const ButtonEvent& in) : EventBase(static_cast<QEvent::Type>(Type))
{
    buttonState = in.buttonState;
    button = in.button;
    myHandled = in.myHandled;
}

ButtonStateType ButtonEvent::buttonStatus()
{
    return buttonState;
}

void ButtonEvent::setButtonStatus(const ButtonStateType &buttonStatusIn)
{
    buttonState = buttonStatusIn;
}

int ButtonEvent::buttonNumber()
{
    return button;
}

void ButtonEvent::setButtonNumber(const int &buttonNumberIn)
{
    button = buttonNumberIn;
}

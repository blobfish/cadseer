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

#ifndef SPB_SPACEBALLQEVENT_H
#define SPB_SPACEBALLQEVENT_H

#include <QInputEvent>
namespace vwr
{
    enum ButtonStateType {BUTTON_NONE = 0, BUTTON_PRESSED, BUTTON_RELEASED};

    void registerEvents();

    class EventBase : public QInputEvent
    {
    public:
        bool isHandled(){return myHandled;}
        void setHandled(bool sig){myHandled = sig;}

    protected:
        EventBase(QEvent::Type event);
        //Qt will not propagate custom events. so we create our own state variable. use it
        //in the qapplication notify and dispatch up the window hierarchy until handled or we
        //reach the default.
        bool myHandled = false;
    };

    class MotionEvent : public EventBase
    {
    public:
        MotionEvent();
        MotionEvent(const MotionEvent& in);
        void translations(int &xTransOut, int &yTransOut, int &zTransOut);
        void setTranslations(const int &xTransIn, const int &yTransIn, const int &zTransIn);
        int translationX(){return xTrans;}
        int translationY(){return yTrans;}
        int translationZ(){return zTrans;}

        void rotations(int &xRotOut, int &yRotOut, int &zRotOut);
        void setRotations(const int &xRotIn, const int &yRotIn, const int &zRotIn);
        int rotationX(){return xRot;}
        int rotationY(){return yRot;}
        int rotationZ(){return zRot;}

        static int Type;

    private:
        int xTrans;
        int yTrans;
        int zTrans;
        int xRot;
        int yRot;
        int zRot;
    };

    class ButtonEvent : public EventBase
    {
    public:
        ButtonEvent();
        ButtonEvent(const ButtonEvent& in);
        ButtonStateType buttonStatus();
        void setButtonStatus(const ButtonStateType &buttonStatusIn);
        int buttonNumber();
        void setButtonNumber(const int &buttonNumberIn);

        static int Type;

    private:
        ButtonStateType buttonState;
        int button;
    };
}
#endif // SPB_SPACEBALLQEVENT_H

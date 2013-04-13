#ifndef SPACEBALLQEVENT_H
#define SPACEBALLQEVENT_H

#include <QInputEvent>
namespace Spaceball
{
    enum ButtonStateType {BUTTON_NONE = 0, BUTTON_PRESSED, BUTTON_RELEASED};

    void registerEvents();

    class EventBase : public QInputEvent
    {
    public:
        bool isHandled(){return handled;}
        void setHandled(bool sig){handled = sig;}

    protected:
        EventBase(QEvent::Type event);
        //Qt will not propagate custom events. so we create our own state variable. use it
        //in the qapplication notify and dispatch up the window hierarchy until handled or we
        //reach the default.
        bool handled;
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
        bool handled;
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
#endif // SPACEBALLQEVENT_H

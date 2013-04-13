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

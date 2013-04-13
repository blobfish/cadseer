#include "spaceballosgevent.h"

SpaceballOSGEvent::SpaceballOSGEvent() : osg::Referenced()
{
    theType = TNone;
    theButtonState = BNone;
    buttonNumber = -1;
    translationX = 0;
    translationY = 0;
    translationZ = 0;
    rotationX = 0;
    rotationY = 0;
    rotationZ = 0;
}

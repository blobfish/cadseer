#ifndef NODEMASKDEFS_H
#define NODEMASKDEFS_H

//from osg::Node documentation:
//The default value is 0xffffffff (all bits set).
//typedef unsigned int NodeMask; size of 32.

namespace NodeMaskDef
{
    static const unsigned int mainCamera =              1 << 1;
    static const unsigned int backGroundCamera =        1 << 2;
    static const unsigned int gestureCamera =           1 << 3;   //also used on gesture background.

    static const unsigned int vertex =                  1 << 4;
    static const unsigned int edge =                    1 << 5;
    static const unsigned int face =                    1 << 6;
    static const unsigned int lod =                     1 << 7;
    static const unsigned int object =                  1 << 8;
    static const unsigned int csys =                    1 << 9;

    static const unsigned int gestureMenu =             1 << 10;
    static const unsigned int gestureCommand =          1 << 11;
}


#endif // NODEMASKDEFS_H

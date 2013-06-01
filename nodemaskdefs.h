#ifndef NODEMASKDEFS_H
#define NODEMASKDEFS_H

namespace NodeMask
{
    static const unsigned int noSelect = 0x1;

    static const unsigned int vertex = 0x2;
    static const unsigned int edge = 0x4;
    static const unsigned int face = 0x8;
    static const unsigned int lod = 0x10;
    static const unsigned int object = 0x20;

    static const unsigned int gestureMenu = 0x100;
    static const unsigned int gestureCommand = 0x200;
}


#endif // NODEMASKDEFS_H

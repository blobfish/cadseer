#ifndef SELECTIONSTATE_H
#define SELECTIONSTATE_H

#include <vector>
#include <string>

namespace SelectionMask
{
    static const unsigned int none = 0;
    static const unsigned int objectsEnabled = 1;
    static const unsigned int objectsSelectable = 1 << 1;
    static const unsigned int featuresEnabled = 1 << 2;
    static const unsigned int featuresSelectable = 1 << 3;
    static const unsigned int solidsEnabled = 1 << 4;
    static const unsigned int solidsSelectable = 1 << 5;
    static const unsigned int shellsEnabled = 1 << 6;
    static const unsigned int shellsSelectable = 1 << 7;
    static const unsigned int facesEnabled = 1 << 8;
    static const unsigned int facesSelectable = 1 << 9;
    static const unsigned int wiresEnabled = 1 << 10;
    static const unsigned int wiresSelectable = 1 << 11;
    static const unsigned int edgesEnabled = 1 << 12;
    static const unsigned int edgesSelectable = 1 << 13;
    static const unsigned int verticesEnabled = 1 << 14;
    static const unsigned int verticesSelectable = 1 << 15;
    static const unsigned int all = 0xFFFFu;
}

namespace SelectionTypes
{
enum Type
{
    None,
    Object,
    Feature,
    Solid,
    Shell,
    Face,
    Wire,
    Edge,
    Vertex
};

inline std::string getNameOfType(Type theType)
{
    static const std::vector<std::string> names({
                                                    "None",
                                                    "Object",
                                                    "Feature",
                                                    "Solid",
                                                    "Shell",
                                                    "Face",
                                                    "Wire",
                                                    "Edge",
                                                    "Vertex"
                                                });
    return names.at(static_cast<int>(theType));
}


}

#endif // SELECTIONSTATE_H

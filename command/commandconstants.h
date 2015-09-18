#ifndef COMMANDCONSTANTS_H
#define COMMANDCONSTANTS_H

#include <string>

namespace CommandConstants
{
static std::string attributeTitle = "CommandAttributeTitle";
enum Constants
{
    StandardViewTop,
    StandardViewFront,
    StandardViewRight,
    ViewFit,
    ConstructionBox,
    ConstructionSphere,
    ConstructionCone,
    ConstructionCylinder,
    ConstructionBlend,
    ConstructionUnion,
    FileImportOCC,
    FileExportOSG
};
}

#endif // COMMANDCONSTANTS_H

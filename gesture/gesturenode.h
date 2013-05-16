#ifndef GESTURENODE_H
#define GESTURENODE_H

namespace osg
{
class MatrixTransform;
class Geode;
}

namespace GestureNode
{
osg::MatrixTransform* buildMenuNode(const char *resourceName);
osg::MatrixTransform* buildCommandNode(const char *resourceName);
osg::MatrixTransform* buildCommonNode(const char *resourceName);
osg::Geode* buildIconGeode(const char *resourceName);
osg::Geode* buildLineGeode();
}

#endif // GESTURENODE_H

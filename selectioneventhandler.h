#ifndef SELECTIONEVENTHANDLER_H
#define SELECTIONEVENTHANDLER_H

#include <osgGA/GUIEventHandler>

class Selected
{
public:
    Selected(){}
    osg::observer_ptr<osg::Geometry> geometry;
    osg::Vec4 color;
    int colorIndex;
};

class SelectionEventHandler : public osgGA::GUIEventHandler
{
public:
    SelectionEventHandler();
    int nodeMask;
    std::vector<Selected> getSelections(){return selections;}
    void clearSelections();
protected:
    virtual bool handle(const osgGA::GUIEventAdapter& eventAdapter,
                        osgGA::GUIActionAdapter& actionAdapter, osg::Object *object,
                        osg::NodeVisitor *nodeVistor);
    void setPrehighlight(osg::Geometry *geometry);
    void clearPrehighlight();
    bool alreadySelected(osg::Geometry *geometry);
    osg::observer_ptr<osg::Geometry> lastPrehighlightGeometry;
    osg::Vec4 lastPrehighlightColor;
    int lastPrehighlightColorIndex;
    osg::Vec4 preHighlightColor;
    osg::Vec4 selectionColor;
    std::vector<Selected> selections;
};

class FadeVisitor : public osg::NodeVisitor
{
public:
    FadeVisitor(bool visIn);
    virtual void apply(osg::Switch &aSwitch);
protected:
    bool visibility;
};

#endif // SELECTIONEVENTHANDLER_H

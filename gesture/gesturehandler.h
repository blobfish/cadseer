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

#ifndef GESTUREHANDLER_H
#define GESTUREHANDLER_H

#include <osgGA/GUIEventHandler>

class GestureHandler : public osgGA::GUIEventHandler
{
public:
    GestureHandler(osg::Camera *cameraIn);
protected:
    virtual bool handle(const osgGA::GUIEventAdapter& eventAdapter,
                        osgGA::GUIActionAdapter& actionAdapter, osg::Object *object,
                        osg::NodeVisitor *nodeVistor);
    double calculateSprayRadius(int nodeCount);
    void startDrag(const osgGA::GUIEventAdapter& eventAdapter);
    void constructMenu();
    std::vector<osg::Vec3> buildNodeLocations(osg::Vec3 direction, int nodeCount);
    void spraySubNodes(osg::Vec3 cursorLocation);
    void contractSubNodes();
    bool rightButtonDown;
    bool dragStarted;
    osg::ref_ptr<osg::Switch> gestureSwitch;
    osg::ref_ptr<osg::Camera> gestureCamera;
    osg::ref_ptr<osg::MatrixTransform> startNode;
    osg::ref_ptr<osg::MatrixTransform> currentNode;
    osg::Matrixd aggregateMatrix;
    bool currentNodeLeft;
    osg::Vec3 lastHitPoint;

    double iconRadius;
    double includedAngle; //in degrees
    double mininumSprayRadius;
    double nodeSpread;
};

class GestureAllSwitchesOffVisitor : public osg::NodeVisitor
{
public:
    GestureAllSwitchesOffVisitor();
    virtual void apply(osg::Switch &aSwitch);
protected:
    bool visibility;
    int hash;
};

#endif // GESTUREHANDLER_H

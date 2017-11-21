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

#ifndef VWR_SPACEBALLMANIPULATOR_H
#define VWR_SPACEBALLMANIPULATOR_H

#include <bitset>

#include <osgGA/StandardManipulator>

namespace vwr{class SpaceballOSGEvent;}

namespace vwr
{
  
typedef std::bitset<64> StateMask;
//mouse support.
static const StateMask ControlKey(StateMask().set(0));
static const StateMask LeftButton(StateMask().set(1));
static const StateMask MiddleButton(StateMask().set(2));
static const StateMask RightButton(StateMask().set(3));

static const StateMask Rotate(ControlKey | LeftButton);
static const StateMask Pan(ControlKey | MiddleButton);

class SpaceballManipulator : public osgGA::StandardManipulator
{
    typedef StandardManipulator inherited;
public:
    META_Object(osgGA, SpaceballManipulator)
    SpaceballManipulator(osg::Camera *camIn = 0);
    SpaceballManipulator(const SpaceballManipulator& manipIn,
                         const osg::CopyOp& copyOp = osg::CopyOp::SHALLOW_COPY);
    virtual ~SpaceballManipulator() override;
    
    virtual void setByMatrix(const osg::Matrixd& matrix) override;
    virtual void setByInverseMatrix(const osg::Matrixd& matrix) override;
    virtual osg::Matrixd getMatrix() const override;
    virtual osg::Matrixd getInverseMatrix() const override;
    
    virtual void setTransformation(const osg::Vec3d&, const osg::Quat&) override;
    virtual void setTransformation(const osg::Vec3d&, const osg::Vec3d&, const osg::Vec3d&) override;
    virtual void getTransformation(osg::Vec3d&, osg::Quat&) const override;
    virtual void getTransformation(osg::Vec3d&, osg::Vec3d&, osg::Vec3d&) const override;
    
    virtual void computeHomePosition(const osg::Camera *camera, bool useBoundingBox) override;
    virtual void home(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us) override;
    virtual void home(double) override;
    virtual bool handle(const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &us) override;
    virtual void init(const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &us) override;

    void dump();
    void setView(const osg::Vec3d &lookDirection, const osg::Vec3d &upDirection);
    void viewFit();

protected:
    //both ortho and perspective contain some magic numbers. translations and rotations
    //use cam/view properties to derive transformations, but at some point the
    //spaceball numbers have to be meshed in. sensitivity mutations should be done to the
    //events before arriving here to be processed.
    void goOrtho(const vwr::SpaceballOSGEvent *event);
    void goPerspective(const vwr::SpaceballOSGEvent *event);
    void getProjectionData();
    void getViewData();
    void scaleView(double scaleFactor);//used for ortho zoom.
    void scaleFit();
    osg::Vec3d projectToBound(const osg::Vec3d &eye, osg::Vec3d lookCenter) const;
    osg::BoundingSphere boundingSphere;
    osg::BoundingSphere camSphere;
    osg::ref_ptr<osg::Camera> cam;
    osg::Vec3d spaceEye;
    osg::Vec3d spaceCenter;
    osg::Vec3d spaceUp;

    struct ProjectionData
    {
        double fovy, aspectRatio, left, right, top, bottom, near, far;
        bool isCamOrtho;
	double width(){return (right - left);}
	double height(){return (top - bottom);}
    };
    ProjectionData projectionData;

    struct ViewData
    {
        osg::Vec3d x, y, z;
    };
    ViewData viewData;
    
    bool handleMouse(const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &us);
    bool windowToWorld(float xIn, float yIn, osg::Vec3d &startOut, osg::Vec3d &endOut) const;
    StateMask currentStateMask;
    osg::Vec2f lastScreenPoint;
};
}

#endif // VWR_SPACEBALLMANIPULATOR_H

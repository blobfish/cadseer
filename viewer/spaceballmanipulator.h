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

#ifndef SPACEBALLMANIPULATOR_H
#define SPACEBALLMANIPULATOR_H

#include <osgGA/CameraManipulator>

namespace spb{class SpaceballOSGEvent;}

namespace osgGA
{
class OSGGA_EXPORT SpaceballManipulator : public osgGA::CameraManipulator
{
    typedef osgGA::CameraManipulator inherited;
public:
    META_Object(osgGA::osgGA, SpaceballManipulator)
    SpaceballManipulator(osg::Camera *camIn = 0);
    SpaceballManipulator(const SpaceballManipulator& manipIn,
                         const osg::CopyOp& copyOp = osg::CopyOp::SHALLOW_COPY);
    virtual ~SpaceballManipulator(){}
    virtual void setByMatrix(const osg::Matrixd& matrix);
    virtual void setByInverseMatrix(const osg::Matrixd& matrix);
    virtual osg::Matrixd getMatrix() const;
    virtual osg::Matrixd getInverseMatrix() const;
    virtual void setNode(osg::Node *);
    virtual osg::Node* getNode(){return node.get();}
    virtual const osg::Node* getNode() const {return node.get();}
    virtual void computeHomePosition(const osg::Camera *camera, bool useBoundingBox);
    virtual void home(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us);
    virtual void home(double);
    virtual bool handle(const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &us);
    virtual void init(const GUIEventAdapter &ea, GUIActionAdapter &us);

    void dump();
    void setView(osg::Vec3d lookDirection, osg::Vec3d upDirection);
    void viewFit();

protected:
    //both ortho and perspective contain some magic numbers. translations and rotations
    //use cam/view properties to derive transformations, but at some point the
    //spaceball numbers have to be meshed in. sensitivity mutations should be done to the
    //events before arriving here to be processed.
    void goOrtho(const spb::SpaceballOSGEvent *event);
    void goPerspective(const spb::SpaceballOSGEvent *event);
    void getProjectionData();
    void getViewData();
    void scaleView(double scaleFactor);//used for ortho zoom.
    void scaleFit();
    osg::Vec3d projectToBound(const osg::Vec3d &eye, osg::Vec3d lookCenter) const;
    osg::ref_ptr<osg::Node> node;
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
    };
    ProjectionData projectionData;

    struct ViewData
    {
        osg::Vec3d x, y, z;
    };
    ViewData viewData;
};
}

#endif // SPACEBALLMANIPULATOR_H

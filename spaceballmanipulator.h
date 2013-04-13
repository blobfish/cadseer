#ifndef SPACEBALLMANIPULATOR_H
#define SPACEBALLMANIPULATOR_H

#include <osgGA/CameraManipulator>

class SpaceballOSGEvent;

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


protected:
    //both ortho and perspective contain some magic numbers. translations and rotations
    //use cam/view properties to derive transformations, but at some point the
    //spaceball numbers have to be meshed in. sensitivity mutations should be done to the
    //events before arriving here to be processed.
    void goOrtho(const SpaceballOSGEvent *event);
    void goPerspective(const SpaceballOSGEvent *event);
    void getProjectionData();
    void getViewData();
    void scaleView(double scaleFactor);//used for ortho zoom.
    osg::Vec3d projectToBound(osg::Vec3d eye, osg::Vec3d lookCenter) const;
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

#ifndef PLOTTER_H
#define PLOTTER_H

#include <osg/Group>
#include <osg/Geometry>

class Plotter
{
public:
    static Plotter& getReference()
    {
        static Plotter instance;
        return instance;
    }
    void setBase(osg::ref_ptr<osg::Group> baseIn);
    void plotPoint(const osg::Vec3d &point);

private:
    Plotter(){}
    Plotter(const Plotter &){}
    void operator=(const Plotter &){}

    osg::ref_ptr<osg::Geometry> pointGeometry;
};

#endif // PLOTTER_H

#include <assert.h>
#include <osg/Geode>
#include <osg/Point>

#include "plotter.h"

void Plotter::setBase(osg::ref_ptr<osg::Group> baseIn)
{
    pointGeometry = new osg::Geometry();
    pointGeometry->setVertexArray(new osg::Vec3Array());
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    colors->push_back(osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f));
    pointGeometry->setColorArray(colors.get());
    pointGeometry->setColorBinding(osg::Geometry::BIND_OVERALL);
    pointGeometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    osg::Point *point = new osg::Point;
    point->setSize(10.0);
    pointGeometry->getOrCreateStateSet()->setAttribute(point);

    pointGeometry->addPrimitiveSet(new osg::DrawArrays
                          (osg::PrimitiveSet::POINTS, 0, 0));

    osg::ref_ptr<osg::Geode> pointGeode = new osg::Geode();
    pointGeode->addDrawable(pointGeometry);
    baseIn->addChild(pointGeode);
}

void Plotter::plotPoint(const osg::Vec3d &point)
{
    assert(pointGeometry.valid());

    osg::Vec3Array *currentVertices = dynamic_cast<osg::Vec3Array *>(pointGeometry->getVertexArray());
    assert(currentVertices);
    currentVertices->push_back(point);
    currentVertices->dirty();

    osg::DrawArrays *draw = dynamic_cast<osg::DrawArrays *>(pointGeometry->getPrimitiveSet(0));
    assert(draw);
    draw->setCount(draw->getCount() + 1);
    pointGeometry->dirtyDisplayList();
}

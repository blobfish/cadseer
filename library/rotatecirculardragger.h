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

#ifndef LBR_ROTATECIRCULARDRAGGER_H
#define LBR_ROTATECIRCULARDRAGGER_H

#include <osg/Geometry>
#include <osgManipulator/Dragger>
#include <osgManipulator/Projector>
#include <osgManipulator/Constraint>
#include "circleprojector.h"

namespace lbr
{

/** @brief Constraint for angle rotation.
*/
class AngleConstraint : public osgManipulator::Constraint
{
  public:
    AngleConstraint(osg::Node&, const osg::Vec3d&, osg::Vec3d::value_type);
    virtual bool constrain(osgManipulator::Rotate3DCommand& command) const /*override*/;
    
    void setStart(const osg::Vec3d& startIn) {start = startIn;}
    const osg::Vec3d& getStart() const {return start;}

    void setAngle(osg::Vec3d::value_type radiansIn) {increment = radiansIn;}
    const osg::Vec3d::value_type& getAngle() const {return increment;}
    
  protected:
    virtual ~AngleConstraint(){} /*override*/
    
    osg::Vec3d start;
    osg::Vec3d::value_type increment; //!< in radians
};


/** @brief Dragger for performing rotation on circle
*/
class RotateCircularDragger : public osgManipulator::Dragger
{
  public:
    RotateCircularDragger();
    META_OSGMANIPULATOR_Object(osgManipulator, RotateCircularDragger)
    
    void setupDefaultGeometry() /*override*/;
    void setColor(const osg::Vec4& color) {_color = color; setMaterialColor(_color,*this);}
    const osg::Vec4& getColor() const {return _color;}
    void setPickColor(const osg::Vec4& color) {_pickColor = color;}
    const osg::Vec4& getPickColor() const {return _pickColor;}
    
    void setAngularSpan(const osg::Vec3d::value_type &); //in degrees
    osg::Vec3d::value_type getAngularSpan(){return angularSpan;} //in degrees
    void setMajorRadius(const osg::Vec3d::value_type &);
    osg::Vec3d::value_type getMajorRadius(){return majorRadius;}
    void setMinorRadius(const osg::Vec3d::value_type &);
    osg::Vec3d::value_type getMinorRadius(){return minorRadius;}
    void setMajorIsoLines(const std::size_t &);
    std::size_t getMajorIsoLines(){return majorIsoLines;}
    void setMinorIsoLines(const std::size_t &);
    std::size_t getMinorIsoLines(){return minorIsoLines;}
    
    osg::Geometry* buildLine() const;
    osg::Geometry* buildTorus() const;
    osg::Vec3d calculateArcMidPoint() const;
					      
    virtual bool handle(const osgManipulator::PointerInfo&, const osgGA::GUIEventAdapter&, osgGA::GUIActionAdapter&)/*override*/;
    
  protected:
    virtual ~RotateCircularDragger() /*override*/;
    osg::ref_ptr<CircleProjector> _projector;
    osg::Vec4 _color;
    osg::Vec4 _pickColor;
    
    bool dragStarted;
    osg::Vec3d startPoint;
    osg::Matrix _startLocalToWorld, _startWorldToLocal;
    
    osg::ref_ptr<osg::Geometry> line;
    osg::ref_ptr<osg::Geometry> torus;
    
    osg::Vec3d::value_type majorRadius; //!< radius of circle.
    std::size_t majorIsoLines; //!< number of segments for majorRadius.
    osg::Vec3d::value_type minorRadius; //!< radius of torus. bigger radius = sloppier pick.
    std::size_t minorIsoLines; //!< number of segments for minorRadius.
    osg::Vec3d::value_type angularSpan; //!< span in degrees.
    
    void update();
};
}

#endif //LBR_ROTATECIRCULARDRAGGER_H

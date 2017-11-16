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

#ifndef LBR_LINEARDRAGGER_H
#define LBR_LINEARDRAGGER_H

#include <osgManipulator/Translate1DDragger>

namespace osg{class AutoTransform;}

namespace lbr
{
  class LinearDragger : public osgManipulator::Translate1DDragger
  {
  public:
    virtual osg::Object* cloneType() const {return new LinearDragger();}
    virtual bool isSameKindAs(const osg::Object* obj) const {return dynamic_cast<const LinearDragger *>(obj)!=NULL;}
    virtual const char* libraryName() const {return "osgManipulator"; }
    virtual const char* className() const {return "LinearDragger";}
    
    LinearDragger();
    void setScreenScale(double);
    double getScreenScale(){return screenScale;}
    void setIncrement(double);
    double getIncrement() const;
  protected:
    double screenScale;
    osg::ref_ptr<osg::AutoTransform> autoScaleTransform;
    osg::ref_ptr<osg::MatrixTransform> scaleTransform;
    osg::ref_ptr<osgManipulator::GridConstraint> incrementConstraint;
  };
}

#endif // LBR_LINEARDRAGGER_H

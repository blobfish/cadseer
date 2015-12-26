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

#ifndef IPGROUP_H
#define IPGROUP_H

#include <boost/signals2.hpp>

#include <osg/ref_ptr>
#include <osg/MatrixTransform>
#include <osgManipulator/Dragger>

#include <message/message.h>

namespace osg{class Switch; class AutoTransform;}
namespace ftr{class Parameter;}

namespace lbr
{
  class LinearDragger;
  class LinearDimension;
  class IPCallback;
  
  /*! @brief Interactive Parameter Group
   * 
   * Dragger and callback and dimensions
   */
  class IPGroup : public osg::MatrixTransform
  {
  public:
    IPGroup(ftr::Parameter *parameterIn);
    IPGroup(const IPGroup &copy, const osg::CopyOp& copyOp=osg::CopyOp::SHALLOW_COPY);
    META_Node(osg, IPGroup);
    
    ftr::Parameter* getParameter(){return parameter;}
    void setParameterValue(double valueIn);
    void setRotationAxis(const osg::Vec3d&, const osg::Vec3d&);
    void valueHasChanged();
    void constantHasChanged();
    osg::BoundingBox maxTextBoundingBox();
    bool processMotion(const osgManipulator::MotionCommand&);
    void draggerShow();
    void draggerHide();
    void noAutoRotateDragger();
    
    osg::ref_ptr<osg::AutoTransform> rotation;
    osg::ref_ptr<LinearDimension> mainDim;
    osg::ref_ptr<LinearDimension> differenceDim;
    osg::ref_ptr<LinearDimension> overallDim;
    
    void setMatrixDims(const osg::Matrixd &matrixIn);
    void setMatrixDragger(const osg::Matrixd &matrixIn);
    void setDimsFlipped(bool flippedIn);
    
    typedef boost::signals2::signal<void (const msg::Message &)> MessageOutSignal;
    boost::signals2::connection connectMessageOut(const MessageOutSignal::slot_type &subscriber)
    {
      return messageOutSignal.connect(subscriber);
    }
    
  protected:
    IPGroup();
    ftr::Parameter *parameter = nullptr;
    double value;
    osg::Vec3d originStart;
    osg::ref_ptr<osg::Switch> dimSwitch;
    osg::ref_ptr<LinearDragger> dragger;
    osg::ref_ptr<osg::Switch> draggerSwitch;
    osg::ref_ptr<osg::MatrixTransform> draggerMatrix;
    osg::ref_ptr<IPCallback> ipCallback;
    MessageOutSignal messageOutSignal;
  };
  

  /*! @brief class to mark the feature dirty when the dragger is used.
   * 
   * not really in love with this, but don't see a better way.
   */
  class IPCallback : public osgManipulator::DraggerTransformCallback
  {
  public:
    IPCallback(osg::MatrixTransform *t, IPGroup*);
    virtual bool receive(const osgManipulator::TranslateInLineCommand&) override;
    
  private:
    IPGroup *group;
  };
}

#endif // IPGROUP_H

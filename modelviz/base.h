/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef MDV_BASE_H
#define MDV_BASE_H

#include <osg/Geometry>

namespace mdv
{
  class Base : public osg::Geometry
  {
    public:
      Base();
      Base(const Base &rhs, const osg::CopyOp& copyOperation = osg::CopyOp::SHALLOW_COPY);
      
      META_Node(mdv, Base)
      
      virtual void setColor(const osg::Vec4 &colorIn); //!< used to change the object color.
      const osg::Vec4& getColor() const {return color;}
      void setPreHighlightColor(const osg::Vec4 &colorIn);
      const osg::Vec4& getPreHighlightColor() const {return colorPreHighlight;}
      void setHighlightColor(const osg::Vec4 &colorIn);
      const osg::Vec4& getHighlightColor() const {return colorHighlight;}
      
      virtual void setToColor(){}
      virtual void setToPreHighlight(){}
      virtual void setToHighlight(){}
      
    protected:
      osg::Vec4 color = osg::Vec4(.1f, .7f, .1f, .5f);
      osg::Vec4 colorPreHighlight = osg::Vec4(1.0, 1.0, 0.0, 1.0);
      osg::Vec4 colorHighlight = osg::Vec4(1.0, 1.0, 1.0, 1.0);
  };
}

#endif // MDV_BASE_H

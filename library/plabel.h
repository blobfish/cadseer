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

#ifndef LBR_PLABEL_H
#define LBR_PLABEL_H

#include <boost/signals2/connection.hpp>

#include <osg/MatrixTransform>
#include <osg/ref_ptr>

namespace osg{class AutoTransform;}
namespace osgText{class Text;}

namespace ftr{namespace prm{class Parameter;}}
namespace prj{namespace srl{class PLabel;}}

namespace lbr
{
  class PLabel : public osg::MatrixTransform
  {
  public:
    PLabel(ftr::prm::Parameter *parameterIn);
    PLabel(const PLabel &copy, const osg::CopyOp& copyOp=osg::CopyOp::SHALLOW_COPY);
    META_Node(osg, PLabel);
    
    void valueHasChanged();
    void constantHasChanged();
    ftr::prm::Parameter* getParameter(){return parameter;}
    void setTextColor(const osg::Vec4&);
    void setTextColor(); //!< linked is green and constant is blue.
    
    bool showName = false;
    
    prj::srl::PLabel serialOut() const;
    void serialIn(const prj::srl::PLabel&);
    
  protected:
    PLabel(); //needed for META_Node
    void build();
    void setText(); //sets the text from the parameter value
    ftr::prm::Parameter *parameter = nullptr;
    osg::ref_ptr<osg::AutoTransform> autoTransform;
    osg::ref_ptr<osgText::Text> text;
    
    boost::signals2::scoped_connection valueConnection;
    boost::signals2::scoped_connection constantConnection;
  };
}

#endif // LBR_PLABEL_H

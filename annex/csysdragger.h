/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef ANN_CSYSDRAGGER_H
#define ANN_CSYSDRAGGER_H

#include <annex/base.h>

namespace ftr{class Base; namespace prm{class Parameter;}}
namespace lbr{class CSysDragger;}
namespace osg{class Matrixd;}

namespace ann
{
  class DCallBack;
  class CSysDragger : public Base
  {
  public:
    CSysDragger() = delete;
    CSysDragger(ftr::Base*, ftr::prm::Parameter*);
    ~CSysDragger();
    virtual Type getType() override {return Type::CSysDragger;}
    
    void draggerUpdate(); //!< sets only the dragger to csys parameter
    void draggerUpdate(const osg::Matrixd&); //!< sets only the dragger to matrix
    void setCSys(const osg::Matrixd&); //!< sets parameter to matrix and applies transformation to dragger.
    
    osg::ref_ptr<lbr::CSysDragger> dragger;
    osg::ref_ptr<DCallBack> callBack;
    ftr::prm::Parameter *parameter;
  };
}

#endif // ANN_CSYSDRAGGER_H

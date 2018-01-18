/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2018  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef ANN_INSTANCEMAPPER_H
#define ANN_INSTANCEMAPPER_H

#include <annex/base.h>

namespace ftr{class ShapeHistory;}
namespace prj{namespace srl{class InstanceData;}}
namespace ann
{
  class SeerShape;
  
  /**
  * @brief class used to get consistent ids out of
  * features that create duplicates of shapes.
  */
  class InstanceMapper : public Base
  {
  public:
    InstanceMapper();
    InstanceMapper(const InstanceMapper&) = delete;
    virtual ~InstanceMapper() override;
    virtual Type getType(){return Type::InstanceMapper;}
    
  /*! @brief Initialize mapping from the source shape.
   * 
   * @param sShapeIn source of copies.
   * @param idIn id of the source shape. if nil, use whole shape.
   * @param shapeHistory update shape history passed into feature update.
   */
    void startMapping(const SeerShape &sShapeIn, const boost::uuids::uuid &idIn, const ftr::ShapeHistory &shapeHistory);
    
  /*! @brief map from initialized source shape into this shape.
   * 
   * @param sShape target of instances.
   * @param dsShape copied shape.
   * @param instance which copy instance.
   * @note dsShape should already be in the root shape of sShape.
   */
    void mapIndex(SeerShape &sShape, const TopoDS_Shape &dsShape, std::size_t instance);
    
    prj::srl::InstanceData serialOut();
    void serialIn(const prj::srl::InstanceData&);
    
  private:
    struct Data;
    std::unique_ptr<Data> data;
  };
}

#endif // ANN_INSTANCEMAPPER_H

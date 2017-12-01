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

#ifndef ANN_INTERSECTIONMAPPER_H
#define ANN_INTERSECTIONMAPPER_H

#include <annex/base.h>

class BOPAlgo_Builder;

namespace ftr{class UpdatePayload;}
namespace prj{namespace srl{class IntersectionMapper;}}

namespace ann
{
  class SeerShape;
  
  /**
  * @brief shape id mapping for booleans.
  */
  class IntersectionMapper : public Base
  {
  public:
    IntersectionMapper();
    IntersectionMapper(const IntersectionMapper&) = delete;
    virtual ~IntersectionMapper() override;
    virtual Type getType(){return Type::IntersectionMapper;}
    
    prj::srl::IntersectionMapper serialOut(); //!<convert this into serializable object.
    void serialIn(const prj::srl::IntersectionMapper &); //intialize this from serial object.
    
    void go(const ftr::UpdatePayload&, BOPAlgo_Builder&, SeerShape&);
  private:
    struct Data;
    std::unique_ptr<Data> data;
  };
}

#endif // ANN_INTERSECTIONMAPPER_H

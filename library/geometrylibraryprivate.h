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

#ifndef LBR_GEOMETRYLIBRARYPRIVATE_H
#define LBR_GEOMETRYLIBRARYPRIVATE_H

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <osg/Geometry>

#include "geometrylibrarytag.h"

namespace lbr
{
  struct MapRecord
  {
    Tag tag;
    osg::ref_ptr<osg::Geometry> geometry;
    
    //@{
    //! used as tags.
    struct ByTag{};
    //@}
  };
  
  struct KeyHash
  {
    std::size_t operator()(const Tag& tagIn) const
    {
      return boost::hash_range(tagIn.entries.begin(), tagIn.entries.end());
    }
  };
  
  struct TagEquality
  {
    bool operator()(const Tag &lhsIn, const Tag &rhsIn) const
    {
      return lhsIn == rhsIn;
    }
  };

  namespace BMI = boost::multi_index;
  typedef boost::multi_index_container
  <
    MapRecord,
    BMI::indexed_by
    <
      BMI::hashed_unique
      <
        BMI::tag<MapRecord::ByTag>,
        BMI::member<MapRecord, Tag, &MapRecord::tag>,
        KeyHash,
        TagEquality
      >
    >
  > MapContainer;
  
  struct MapWrapper //so I can forward declare.
  {
    MapContainer mapContainer;
  };
}


#endif //LBR_GEOMETRYLIBRARYPRIVATE_H

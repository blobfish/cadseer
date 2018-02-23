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

#ifndef MDV_SHAPEGEOMETRYPRIVATE_H
#define MDV_SHAPEGEOMETRYPRIVATE_H

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>

#include <tools/idtools.h>

namespace mdv
{
  namespace BMI = boost::multi_index;
  
  //! @brief Map between geometry identifier and primitive set index.
  struct IdPSetRecord
  {
    boost::uuids::uuid id = gu::createNilId();
    std::size_t primitiveSetIndex = 0; //!< primitiveset index.
    
    //@{
    //! used as tags.
    struct ById{};
    struct ByPSet{};
    //@}
  };
  std::ostream& operator<<(std::ostream& os, const IdPSetRecord& record)
  {
    os << gu::idToString(record.id) << "      " << record.primitiveSetIndex << std::endl;
    return os;
  }
  
  typedef boost::multi_index_container
  <
    IdPSetRecord,
    BMI::indexed_by
    <
      BMI::ordered_unique
      <
        BMI::tag<IdPSetRecord::ById>,
        BMI::member<IdPSetRecord, boost::uuids::uuid, &IdPSetRecord::id>
      >,
      BMI::ordered_non_unique
      <
        BMI::tag<IdPSetRecord::ByPSet>,
        BMI::member<IdPSetRecord, std::size_t, &IdPSetRecord::primitiveSetIndex>
      >
    >
  > IdPSetContainer;
  std::ostream& operator<<(std::ostream& os, const IdPSetContainer& container)
  {
    typedef IdPSetContainer::index<IdPSetRecord::ById>::type List;
    const List &list = container.get<IdPSetRecord::ById>();
    for (List::const_iterator it = list.begin(); it != list.end(); ++it)
      os << *it;
    return os;
  }
  
  //! @brief So I can forward declare.
  struct IdPSetWrapper
  {
    IdPSetContainer idPSetContainer;
    
    bool hasId(const boost::uuids::uuid &idIn) const
    {
      typedef IdPSetContainer::index<IdPSetRecord::ById>::type List;
      const List &list = idPSetContainer.get<IdPSetRecord::ById>();
      List::const_iterator it = list.find(idIn);
      return (it != list.end());
    }
    
    std::size_t findPSetFromId(const boost::uuids::uuid &idIn) const
    {
      typedef IdPSetContainer::index<IdPSetRecord::ById>::type List;
      const List &list = idPSetContainer.get<IdPSetRecord::ById>();
      List::const_iterator it = list.find(idIn);
      assert(it != list.end());
      return it->primitiveSetIndex;
    }
    
    bool hasPSet(std::size_t indexIn) const
    {
      typedef IdPSetContainer::index<IdPSetRecord::ByPSet>::type List;
      const List &list = idPSetContainer.get<IdPSetRecord::ByPSet>();
      List::const_iterator it = list.find(indexIn);
      return it != list.end();
    }
    
    boost::uuids::uuid findIdFromPSet(std::size_t indexIn) const
    {
      typedef IdPSetContainer::index<IdPSetRecord::ByPSet>::type List;
      const List &list = idPSetContainer.get<IdPSetRecord::ByPSet>();
      List::const_iterator it = list.find(indexIn);
      assert(it != list.end());
      return it->id;
    }
  };
  
  //! @brief map between primitive set indexes and primitive indexes (triangle or line)
  struct PSetPrimitiveRecord
  {
    std::size_t primitiveSetIndex = 0; //!< primitiveset index.
    std::size_t primitiveIndex = 0; //!< triangle index.
    
    //@{
    //! used as tags.
    struct ByPSet{};
    struct ByPrimitive{};
    //@}
  };
  std::ostream& operator<<(std::ostream& os, const PSetPrimitiveRecord& record)
  {
    os << record.primitiveSetIndex << "      " << record.primitiveIndex << std::endl;
    return os;
  }
  
  typedef boost::multi_index_container
  <
    PSetPrimitiveRecord,
    BMI::indexed_by
    <
      BMI::ordered_non_unique
      <
        BMI::tag<PSetPrimitiveRecord::ByPSet>,
        BMI::member<PSetPrimitiveRecord, std::size_t, &PSetPrimitiveRecord::primitiveSetIndex>
      >,
      BMI::ordered_unique
      <
        BMI::tag<PSetPrimitiveRecord::ByPrimitive>,
        BMI::member<PSetPrimitiveRecord, std::size_t, &PSetPrimitiveRecord::primitiveIndex>
      >
    >
  > PSetPrimitiveContainer;
  std::ostream& operator<<(std::ostream& os, const PSetPrimitiveContainer& container)
  {
    typedef PSetPrimitiveContainer::index<PSetPrimitiveRecord::ByPSet>::type List;
    const List &list = container.get<PSetPrimitiveRecord::ByPSet>();
    for (List::const_iterator it = list.begin(); it != list.end(); ++it)
      os << *it;
    return os;
  }
  
  //! @brief So I can forward declare.
  struct PSetPrimitiveWrapper
  {
    PSetPrimitiveContainer pSetPrimitiveContainer;
    bool hasPSet(std::size_t indexIn)
    {
      typedef PSetPrimitiveContainer::index<PSetPrimitiveRecord::ByPSet>::type List;
      const List &list = pSetPrimitiveContainer.get<PSetPrimitiveRecord::ByPSet>();
      List::const_iterator it = list.find(indexIn);
      return it != list.end();
    }
    
    std::size_t findPSetFromPrimitive(std::size_t indexIn)
    {
      typedef PSetPrimitiveContainer::index<PSetPrimitiveRecord::ByPrimitive>::type List;
      const List &list = pSetPrimitiveContainer.get<PSetPrimitiveRecord::ByPrimitive>();
      List::const_iterator it = list.find(indexIn);
      assert(it != list.end());
      return it->primitiveSetIndex;
    }
  };
}

#endif // MDV_SHAPEGEOMETRYPRIVATE_H

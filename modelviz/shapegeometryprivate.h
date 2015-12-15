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

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/composite_key.hpp>

#include <osg/BoundingSphere>

namespace mdv
{
  namespace BMI = boost::multi_index;
  
  //! @brief struct for map record
  struct IdPSetRecord
  {
    boost::uuids::uuid id = boost::uuids::nil_generator()();
    std::size_t primitiveSetIndex = 0; //!< primitiveset index.
    osg::BoundingSphere bSphere; //!< only used for edges. faces use kdtree
    
    //@{
    //! used as tags.
    struct ById{};
    struct ByPSet{};
    //@}
  };
  std::ostream& operator<<(std::ostream& os, const IdPSetRecord& record)
  {
    os << boost::uuids::to_string(record.id) << "      " << record.primitiveSetIndex << std::endl;
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
    
    const osg::BoundingSphere& findBSphereFromPSet(std::size_t indexIn) const
    {
      typedef IdPSetContainer::index<IdPSetRecord::ByPSet>::type List;
      const List &list = idPSetContainer.get<IdPSetRecord::ByPSet>();
      List::const_iterator it = list.find(indexIn);
      assert(it != list.end());
      return it->bSphere;
    }
  };
  
  //! @brief map between primitiveset indexes and triangle indexes
  struct PSetVertexRecord
  {
    std::size_t primitiveSetIndex = 0; //!< primitiveset index.
    std::size_t vertexIndex = 0; //!< triangle index.
    
    //@{
    //! used as tags.
    struct ByPSet{};
    struct ByVertex{};
    //@}
  };
  std::ostream& operator<<(std::ostream& os, const PSetVertexRecord& record)
  {
    os << record.primitiveSetIndex << "      " << record.vertexIndex << std::endl;
    return os;
  }
  
  typedef boost::multi_index_container
  <
    PSetVertexRecord,
    BMI::indexed_by
    <
      BMI::ordered_non_unique
      <
	BMI::tag<PSetVertexRecord::ByPSet>,
	BMI::member<PSetVertexRecord, std::size_t, &PSetVertexRecord::primitiveSetIndex>
      >,
      BMI::ordered_unique
      <
	BMI::tag<PSetVertexRecord::ByVertex>,
	BMI::member<PSetVertexRecord, std::size_t, &PSetVertexRecord::vertexIndex>
      >
    >
  > PSetVertexContainer;
  std::ostream& operator<<(std::ostream& os, const PSetVertexContainer& container)
  {
    typedef PSetVertexContainer::index<PSetVertexRecord::ByPSet>::type List;
    const List &list = container.get<PSetVertexRecord::ByPSet>();
    for (List::const_iterator it = list.begin(); it != list.end(); ++it)
      os << *it;
    return os;
  }
  
  //! @brief So I can forward declare.
  struct PSetVertexWrapper
  {
    PSetVertexContainer pSetVertexContainer;
    bool hasPSet(std::size_t indexIn)
    {
      typedef PSetVertexContainer::index<PSetVertexRecord::ByPSet>::type List;
      const List &list = pSetVertexContainer.get<PSetVertexRecord::ByPSet>();
      List::const_iterator it = list.find(indexIn);
      return it != list.end();
    }
    
    std::size_t findPSetFromVertex(std::size_t indexIn)
    {
      typedef PSetVertexContainer::index<PSetVertexRecord::ByVertex>::type List;
      const List &list = pSetVertexContainer.get<PSetVertexRecord::ByVertex>();
      List::const_iterator it = list.find(indexIn);
      assert(it != list.end());
      return it->primitiveSetIndex;
    }
  };
}

#endif // MDV_SHAPEGEOMETRYPRIVATE_H

/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013  tanderson <email>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef MAPS_H
#define MAPS_H

#include <limits.h>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/nil_generator.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <TopoDS_Shape.hxx>

#include "../globalutilities.h"

namespace Feature
{
  namespace BMI = boost::multi_index;
  
  //Evolution Container
  //!@ multi index struct for evolution map
  struct EvolutionRecord
  {
    boost::uuids::uuid inId;
    boost::uuids::uuid outId;
    EvolutionRecord() : inId(boost::uuids::nil_generator()()), outId(boost::uuids::nil_generator()()) {}
    
    //@{
    //! used as tags.
    struct ByInId{};
    struct ByOutId{};
    //@}
  };
  
  std::ostream& operator<<(std::ostream& os, const EvolutionRecord& record);
  
  typedef boost::multi_index_container
  <
    EvolutionRecord,
    BMI::indexed_by
    <
      BMI::ordered_non_unique
      <
        BMI::tag<EvolutionRecord::ByInId>,
        BMI::member<EvolutionRecord, boost::uuids::uuid, &EvolutionRecord::inId>
      >,
      BMI::ordered_non_unique
      <
        BMI::tag<EvolutionRecord::ByOutId>,
        BMI::member<EvolutionRecord, boost::uuids::uuid, &EvolutionRecord::outId>
      >
    >
  > EvolutionContainer;
  
  std::ostream& operator<<(std::ostream& os, const EvolutionContainer& container);


  //ResultShapeContainer
  struct ResultRecord
  {
    boost::uuids::uuid id;
    TopoDS_Shape shape;
    std::size_t shapeOffset; //!< only used for read and write of file
    
    ResultRecord() : id(boost::uuids::nil_generator()()), shape(TopoDS_Shape()), shapeOffset(0) {}
    
    //@{
    //! used as tags. no index on shapeOffset
    struct ById{};
    struct ByShape{};
    //@}
  };
  
  struct KeyHash
  {
    std::size_t operator()(const TopoDS_Shape& shape)const
    {
      int hashOut = GU::getShapeHash(shape);
      return static_cast<std::size_t>(hashOut);
    }
  };
  
  struct ShapeEquality
  {
    bool operator()(const TopoDS_Shape &shape1, const TopoDS_Shape &shape2) const
    {
      return shape1.IsSame(shape2);
    }
  };
  
  std::ostream& operator<<(std::ostream& os, const ResultRecord& record);
  
  typedef boost::multi_index_container
  <
    ResultRecord,
    BMI::indexed_by
    <
      BMI::ordered_unique
      <
        BMI::tag<ResultRecord::ById>,
        BMI::member<ResultRecord, boost::uuids::uuid, &ResultRecord::id>
      >,
      BMI::hashed_unique
      <
        BMI::tag<ResultRecord::ByShape>,
        BMI::member<ResultRecord, TopoDS_Shape, &ResultRecord::shape>,
        KeyHash,
        ShapeEquality
      >
    >
  > ResultContainer;

  std::ostream& operator<<(std::ostream& os, const ResultContainer& container);
  void buildResultContainer(const TopoDS_Shape &shapeIn, ResultContainer &containerInOut);
  const ResultRecord& findResultByShape(const ResultContainer &containerIn, const TopoDS_Shape &shapeIn);
  const ResultRecord& findResultById(const ResultContainer &containerIn, const boost::uuids::uuid &idIn);
  void updateShapeById(ResultContainer& containerIn, const boost::uuids::uuid &idIn, const TopoDS_Shape &shapeIn);
  
  //FeatureMap
  struct FeatureRecord
  {
    boost::uuids::uuid id;
    std::string tag;
    
    FeatureRecord() : id(boost::uuids::nil_generator()()), tag() {}
    
    //@{
    //! used as tags. no index on shapeOffset
    struct ById{};
    struct ByTag{};
    //@}
  };
  
  std::ostream& operator<<(std::ostream& os, const FeatureRecord& record);
  
  typedef boost::multi_index_container
  <
    FeatureRecord,
    BMI::indexed_by
    <
      BMI::ordered_unique
      <
        BMI::tag<FeatureRecord::ById>,
        BMI::member<FeatureRecord, boost::uuids::uuid, &FeatureRecord::id>
      >,
      BMI::ordered_unique
      <
        BMI::tag<FeatureRecord::ByTag>,
        BMI::member<FeatureRecord, std::string, &FeatureRecord::tag>
      >
    >
  > FeatureContainer;
  
  std::ostream& operator<<(std::ostream& os, const FeatureContainer& container);
  const FeatureRecord& findFeatureByTag(const FeatureContainer &containerIn, const std::string &featureTagIn);
  
}

#endif //MAPS_H

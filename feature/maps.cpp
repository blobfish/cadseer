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

#include <assert.h>

#include <boost/uuid/random_generator.hpp>

#include <globalutilities.h>
#include <feature/maps.h>

using namespace ftr; //needed for parameters.

static const std::vector<std::string> shapeStrings
({
     "Compound",
     "Compound Solid",
     "Solid",
     "Shell",
     "Face",
     "Wire",
     "Edge",
     "Vertex",
     "Shape",
 });

ostream& ftr::operator<<(ostream& os, const EvolutionRecord& record)
{
  os << boost::uuids::to_string(record.inId) << "      " << boost::uuids::to_string(record.outId) << std::endl;
  return os;
}

ostream& ftr::operator<<(ostream& os, const EvolutionContainer& container)
{
  typedef EvolutionContainer::index<EvolutionRecord::ByInId>::type List;
  const List &list = container.get<EvolutionRecord::ByInId>();
  for (List::const_iterator it = list.begin(); it != list.end(); ++it)
    os << *it;
  return os;
}

bool ftr::hasInId(const EvolutionContainer& containerIn, const boost::uuids::uuid& inIdIn)
{
  typedef EvolutionContainer::index<EvolutionRecord::ByInId>::type List;
  const List &list = containerIn.get<EvolutionRecord::ByInId>();
  List::const_iterator it = list.find(inIdIn);
  return (it != list.end());
}

const EvolutionRecord& ftr::findRecordByIn(const EvolutionContainer& containerIn, const boost::uuids::uuid& inIdIn)
{
  typedef EvolutionContainer::index<EvolutionRecord::ByInId>::type List;
  const List &list = containerIn.get<EvolutionRecord::ByInId>();
  List::const_iterator it = list.find(inIdIn);
  return *it;
}

std::ostream& ftr::operator<<(std::ostream& os, const ResultRecord& record)
{
  os << boost::uuids::to_string(record.id) << "      " << 
    gu::getShapeHash(record.shape) << "      " <<
    record.shapeOffset << std::endl;
  return os;
}

std::ostream& ftr::operator<<(std::ostream& os, const ResultContainer& container)
{
  typedef ResultContainer::index<ResultRecord::ById>::type List;
  const List &list = container.get<ResultRecord::ById>();
  for (List::const_iterator it = list.begin(); it != list.end(); ++it)
    os << *it;
  return os;
}

const ResultRecord& ftr::findResultByShape(const ResultContainer& containerIn, const TopoDS_Shape& shapeIn)
{
  typedef ResultContainer::index<ResultRecord::ByShape>::type List;
  const List &list = containerIn.get<ResultRecord::ByShape>();
  List::const_iterator it = list.find(shapeIn);
  assert(it != list.end());
  return *it;
}

bool ftr::hasResult(const ResultContainer& containerIn, const TopoDS_Shape& shapeIn)
{
  typedef ResultContainer::index<ResultRecord::ByShape>::type List;
  const List &list = containerIn.get<ResultRecord::ByShape>();
  List::const_iterator it = list.find(shapeIn);
  return (it != list.end());
}

bool ftr::hasResult(const ResultContainer& containerIn, const boost::uuids::uuid& idIn)
{
  typedef ResultContainer::index<ResultRecord::ById>::type List;
  const List &list = containerIn.get<ResultRecord::ById>();
  List::const_iterator it = list.find(idIn);
  return (it != list.end());
}

const ResultRecord& ftr::findResultById(const ResultContainer& containerIn, const boost::uuids::uuid& idIn)
{
  typedef ResultContainer::index<ResultRecord::ById>::type List;
  const List &list = containerIn.get<ResultRecord::ById>();
  List::const_iterator it = list.find(idIn);
  assert(it != list.end());
  return *it;
}

void ftr::updateShapeById(ResultContainer& containerIn, const boost::uuids::uuid& idIn, const TopoDS_Shape& shapeIn)
{
  typedef ResultContainer::index<ResultRecord::ById>::type List;
  List &list = containerIn.get<ResultRecord::ById>();
  List::iterator it = list.find(idIn);
  assert(it != list.end());
  ResultRecord record = *it;
  record.shape = shapeIn;
  list.replace(it, record);
}

void ftr::updateId(ResultContainer& containerIn, const boost::uuids::uuid& idIn, const TopoDS_Shape& shapeIn)
{
  typedef ResultContainer::index<ResultRecord::ByShape>::type List;
  List &list = containerIn.get<ResultRecord::ByShape>();
  List::iterator it = list.find(shapeIn);
  assert(it != list.end());
  ResultRecord record = *it;
  record.id = idIn;
  list.replace(it, record);
}

void ftr::updateOffset(ResultContainer& containerIn, const TopoDS_Shape &shapeIn, std::size_t offsetIn)
{
  typedef ResultContainer::index<ResultRecord::ByShape>::type List;
  List &list = containerIn.get<ResultRecord::ByShape>();
  List::iterator it = list.find(shapeIn);
  assert(it != list.end());
  ResultRecord record = *it;
  record.shapeOffset = offsetIn;
  list.replace(it, record);
}

std::tuple<int, int, int> ftr::stats(ResultContainer& containerIn, const TopoDS_Shape &shapeIn)
{
  int equalCount = 0;
  int sameCount = 0;
  int partnerCount = 0;
  
  typedef ResultContainer::index<ResultRecord::ById>::type List;
  const List &list = containerIn.get<ResultRecord::ById>();
  for (List::const_iterator it = list.begin(); it != list.end(); ++it)
  {
    const TopoDS_Shape &currentRecordShape = it->shape;
    if (currentRecordShape.IsEqual(shapeIn))
      equalCount++;
    if (currentRecordShape.IsSame(shapeIn))
      sameCount++;
    if (currentRecordShape.IsPartner(shapeIn))
      partnerCount++;
  }
  return std::make_tuple(equalCount, sameCount, partnerCount);
}

ostream& ftr::operator<<(ostream& os, const FeatureRecord& record)
{
  os << boost::uuids::to_string(record.id) << "      " << record.tag << std::endl;
  return os;
}

ostream& ftr::operator<<(ostream& os, const FeatureContainer& container)
{
  typedef FeatureContainer::index<FeatureRecord::ById>::type List;
  const List &list = container.get<FeatureRecord::ById>();
  for (List::const_iterator it = list.begin(); it != list.end(); ++it)
    os << *it;
  return os;
}

const FeatureRecord& ftr::findFeatureByTag(const FeatureContainer &containerIn, const std::string &featureTagIn)
{
  typedef FeatureContainer::index<FeatureRecord::ByTag>::type List;
  const List &list = containerIn.get<FeatureRecord::ByTag>();
  List::const_iterator it = list.find(featureTagIn);
  assert(it != list.end());
  return *it;
}

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

#include <assert.h>
#include <set>
#include <iostream>
#include <iomanip>

#include <boost/graph/topological_sort.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/graph/adj_list_serialize.hpp>

#include <expressions/expressionedgeproperty.h>
#include <expressions/expressiongraph.h>
#include <expressions/expressionmanager.h>

//temp.
#include <sstream>

using namespace expr;

Group::Group() : id(boost::uuids::random_generator()()), name("default")
{

}

bool Group::containsFormula(const boost::uuids::uuid& fIdIn)
{
  std::vector<boost::uuids::uuid>::const_iterator it = std::find(formulaIds.begin(), formulaIds.end(), fIdIn);
  if (it == formulaIds.end())
    return false;
  return true;
}

void Group::removeFormula(const boost::uuids::uuid& fIdIn)
{
  std::vector<boost::uuids::uuid>::iterator it = std::find(formulaIds.begin(), formulaIds.end(), fIdIn);
  assert(it != formulaIds.end());
  formulaIds.erase(it);
}

ExpressionManager::ExpressionManager() : graphPtr(new GraphWrapper()), transArray(), transactionState(false),
  transactionPosition(0)
{
  //allgroup id set upon serialize restore.
  allGroup.name = "All";
}

ExpressionManager::~ExpressionManager()
{

}

GraphWrapper& ExpressionManager::getGraphWrapper()
{
  return *graphPtr;
}

void ExpressionManager::recompute()
{
  graphPtr->recompute();
}

void ExpressionManager::writeOutGraph(const std::string& pathName)
{
  graphPtr->writeOutGraph(pathName);
}

void ExpressionManager::addFormulaToAllGroup(boost::uuids::uuid idIn)
{
  allGroup.formulaIds.push_back(idIn);
}

void ExpressionManager::removeFormulaFromAllGroup(boost::uuids::uuid idIn)
{
  std::vector<boost::uuids::uuid>::iterator it = std::find(allGroup.formulaIds.begin(), allGroup.formulaIds.end(), idIn);
  assert (it != allGroup.formulaIds.end());
  allGroup.formulaIds.erase(it);
}

boost::uuids::uuid ExpressionManager::createUserGroup(const std::string& groupNameIn)
{
  //ensure unique group name.
  for (std::vector<Group>::iterator it = userDefinedGroups.begin(); it != userDefinedGroups.end(); ++it)
  {
    if (it->name == groupNameIn)
      assert(0); //name already exists. use hasGroupOfName.
  }
  
  Group virginGroup;
  virginGroup.name = groupNameIn;
  userDefinedGroups.push_back(virginGroup);
  return virginGroup.id;
}

void ExpressionManager::renameUserGroup(const boost::uuids::uuid& idIn, const std::string &newName)
{
  assert(this->hasUserGroup(idIn));
  assert(!this->hasUserGroup(newName));
  for (std::vector<Group>::iterator it = userDefinedGroups.begin(); it != userDefinedGroups.end(); ++it)
  {
    if (it->id == idIn)
    {
      it->name = newName;
      break;
    }
  }
}

void ExpressionManager::removeUserGroup(const boost::uuids::uuid& idIn)
{
  for (std::vector<Group>::iterator it = userDefinedGroups.begin(); it != userDefinedGroups.end(); ++it)
  {
    if (it->id == idIn)
    {
      userDefinedGroups.erase(it);
      return;
    }
  }
  assert(0); //no group of that id.
}

bool ExpressionManager::hasUserGroup(const std::string& groupNameIn) const
{
  for (std::vector<Group>::const_iterator it = userDefinedGroups.begin(); it != userDefinedGroups.end(); ++it)
  {
    if (it->name == groupNameIn)
      return true;
  }
  return false;
}

bool ExpressionManager::hasUserGroup(const boost::uuids::uuid& groupIdIn) const
{
  for (std::vector<Group>::const_iterator it = userDefinedGroups.begin(); it != userDefinedGroups.end(); ++it)
  {
    if (it->id == groupIdIn)
      return true;
  }
  return false;
}

void ExpressionManager::addFormulaToUserGroup(const boost::uuids::uuid& groupIdIn, const boost::uuids::uuid& formulaIdIn)
{
  for (std::vector<Group>::iterator groupIt = userDefinedGroups.begin(); groupIt != userDefinedGroups.end(); ++groupIt)
  {
    if (groupIt->id == groupIdIn)
    {
      groupIt->formulaIds.push_back(formulaIdIn);
    }
  }
}

bool ExpressionManager::doesUserGroupContainFormula(const boost::uuids::uuid& groupIdIn, const boost::uuids::uuid& formulaIdIn)
{
  for (std::vector<Group>::iterator groupIt = userDefinedGroups.begin(); groupIt != userDefinedGroups.end(); ++groupIt)
  {
    if (groupIt->id == groupIdIn)
    {
      if (groupIt->containsFormula(formulaIdIn))
        return true;
    }
  }
  return false;
}

void ExpressionManager::removeFormulaFromUserGroup(const boost::uuids::uuid& groupIdIn, const boost::uuids::uuid& formulaIdIn)
{
  for (std::vector<Group>::iterator groupIt = userDefinedGroups.begin(); groupIt != userDefinedGroups.end(); ++groupIt)
  {
    if (groupIt->id == groupIdIn)
    {
      std::vector<boost::uuids::uuid>::iterator it = std::find(groupIt->formulaIds.begin(), groupIt->formulaIds.end(), formulaIdIn);
      if (it != groupIt->formulaIds.end())
      {
        groupIt->formulaIds.erase(it);
        return;
      }
    }
  }
  assert(0); //couldn't find group with id and or formula with id.
}

boost::uuids::uuid ExpressionManager::getUserGroupId(const std::string& groupNameIn) const
{
  for (std::vector<Group>::const_iterator groupIt = userDefinedGroups.begin(); groupIt != userDefinedGroups.end(); ++groupIt)
  {
    if (groupIt->name == groupNameIn)
    {
      return groupIt->id;
    }
  }
  assert(0); //no user group of name.
}

boost::uuids::uuid ExpressionManager::getFormulaId(const std::string& nameIn) const
{
  assert(graphPtr->hasFormula(nameIn));
  return (graphPtr->getFormulaId(nameIn));
}

std::string ExpressionManager::getFormulaName(const boost::uuids::uuid& idIn) const
{
  assert(graphPtr->hasFormula(idIn));
  return graphPtr->getFormulaName(idIn);
}


std::string ExpressionManager::getUserGroupName(const boost::uuids::uuid& groupIdIn) const
{
  for (std::vector<Group>::const_iterator groupIt = userDefinedGroups.begin(); groupIt != userDefinedGroups.end(); ++groupIt)
  {
    if (groupIt->id == groupIdIn)
    {
      return groupIt->name;
    }
  }
  assert(0); //no user group of id.
}

std::string ExpressionManager::idToString(const boost::uuids::uuid& idIn)
{
  std::string out;
  std::stringstream stream;
  stream << idIn;
  stream >> out;
  return out;
}

boost::uuids::uuid ExpressionManager::stringToId(const std::string& stringIn)
{
  boost::uuids::uuid out;
  std::stringstream stream;
  stream << stringIn;
  stream >> out;
  return out;
}

void ExpressionManager::removeFormula(const std::string& nameIn)
{
  assert(graphPtr->hasFormula(nameIn));
  boost::uuids::uuid fId = graphPtr->getFormulaId(nameIn);
  this->removeFormula(fId);
}

void ExpressionManager::removeFormula(const boost::uuids::uuid& idIn)
{
  removeFormulaFromAllGroup(idIn);
  for (std::vector<Group>::iterator it = userDefinedGroups.begin(); it != userDefinedGroups.end(); ++it)
  {
    if (it->containsFormula(idIn))
      it->removeFormula(idIn);
  }
  graphPtr->removeFormula(idIn);
  
  //update any linked properties.
//   FormulaLinkContainerType::index<FormulaLink::ByFormulaId>::type::iterator it, itEnd;
//   it = formulaLinks.get<FormulaLink::ByFormulaId>().find(idIn);
//   itEnd = formulaLinks.get<FormulaLink::ByFormulaId>().end();
//   while (it != itEnd)
//   {
//     std::string objectName = it->objectName;
//     std::string propertyName = it->propertyName;
//     App::DocumentObject *object = document->getObject(objectName.c_str());
//     //link might be alive, but the object is not.
//     if (!object) continue;
//     App::PropertyFloat *property = dynamic_cast<App::PropertyFloat *>(object->getPropertyByName(propertyName.c_str()));
//     if (!property) continue;
//     property->clearExpressionLinked();
//     formulaLinks.get<FormulaLink::ByFormulaId>().erase(it);
//     it = formulaLinks.get<FormulaLink::ByFormulaId>().find(idIn);
//     itEnd = formulaLinks.get<FormulaLink::ByFormulaId>().end();
//   }
}

//TODO: need to incorporate link container into transaction.
void ExpressionManager::beginTransaction()
{
  assert(transactionState == false);
  //chop off any beyond position.
  transArray.resize(transactionPosition + 1);
  boost::shared_ptr<GraphWrapper> copy(new GraphWrapper(*graphPtr));
  transArray.push_back(boost::make_tuple(allGroup, userDefinedGroups, copy));
  transactionState = true;
  generateValueCache(valueCache);
}

void ExpressionManager::commitTransaction()
{
  assert(transactionState == true);
  transactionPosition = transArray.size() - 1;
  transactionState = false;
  ValueCache virginCache;
  generateValueCache(virginCache);
  dispatchValueChanges(virginCache);
}

void ExpressionManager::rejectTransaction()
{
  assert(transactionState == true);
  restoreState(transArray.size()-1);
  transactionPosition = transArray.size() - 1;
  transactionState = false;
}

void ExpressionManager::undo()
{
  assert(transactionState == false);
  if (transactionPosition == 0)
    return;
  generateValueCache(valueCache);
  transactionPosition--;
  restoreState(transactionPosition);
  ValueCache virginCache;
  generateValueCache(virginCache);
  dispatchValueChanges(virginCache);
}

void ExpressionManager::redo()
{
  assert(transactionState == false);
  if (transactionPosition >= transArray.size())
    return;
  generateValueCache(valueCache);
  transactionPosition++;
  restoreState(transactionPosition);
  ValueCache virginCache;
  generateValueCache(virginCache);
  dispatchValueChanges(virginCache);
}

void ExpressionManager::restoreState(const std::size_t &index)
{
  assert(index >= 0 && index < transArray.size());
  allGroup = boost::get<0>(transArray.at(index));
  userDefinedGroups = boost::get<1>(transArray.at(index));
  graphPtr = boost::get<2>(transArray.at(index));
  //should we recompute here?
  //should we mark everything dirty? we don't want to have to analyse the changes.
  graphPtr->setAllDirty();
  recompute();
}

void ExpressionManager::generateValueCache(ValueCache &out) const
{
  out.clear();
  std::vector<boost::uuids::uuid> ids = graphPtr->getAllFormulaIds();
  std::vector<boost::uuids::uuid>::const_iterator it;
  for (it = ids.begin(); it != ids.end(); ++it)
    out.insert(std::make_pair(*it, graphPtr->getFormulaValue(*it)));
}

void ExpressionManager::dispatchValueChanges(const ValueCache& virginCache)
{
  std::set<boost::uuids::uuid> allIds;
  ValueCache::const_iterator it;
  for (it = virginCache.begin(); it != virginCache.end(); ++it)
    allIds.insert((*it).first);
  for (it = valueCache.begin(); it != valueCache.end(); ++it)
    allIds.insert((*it).first);
  
  //now we should have one id contained in either or both arrays.
  
  std::cout << std::endl << std::endl;
  
  std::set<boost::uuids::uuid>::const_iterator allIt;
  for (allIt = allIds.begin(); allIt != allIds.end(); ++allIt)
  {
    std::string stringId = idToString(*allIt);
    bool inValueCache = (valueCache.find(*allIt) != valueCache.end());
    bool inVirginCache = (virginCache.find(*allIt) != valueCache.end());
    if (inValueCache && inVirginCache)
    {
      double value = valueCache.at(*allIt);
      double virgin = virginCache.at(*allIt);
        std::cout << stringId;
      if (value == virgin) //epsilon for comparison?
        std::cout << ": No change. " << value << std::endl; //do nothing.
      else
      {
        std::cout << ": Change from: " << value << "   to: " << virgin << std::endl; //dispatch value changed
        updateLinkedProperty(*allIt, virgin);
      }
    }
    else if (!inValueCache && inVirginCache)
    {
      //new expression. shouldn't need to do anything.
      std::cout << stringId << ": New expression" << std::endl;
    }
    else if (inValueCache && !inVirginCache)
    {
      std::cout << stringId << ": Expression removed" << std::endl;
    }
  }
//   document->recompute();
  
  std::cout << std::endl << std::endl;
}

void ExpressionManager::updateLinkedProperty(const boost::uuids::uuid idIn, const double valueIn)
{
//   assert(document);
  //get all links
//   FormulaLinkContainerType::index<FormulaLink::ByFormulaId>::type::iterator it, itEnd;
//   boost::tie(it, itEnd) = formulaLinks.get<FormulaLink::ByFormulaId>().equal_range(idIn);
//   for (;it != itEnd; ++it)
//   {
//     App::DocumentObject *dObject = document->getObject((*it).objectName.c_str());
//     assert(dObject);
//     App::Property *property = dObject->getPropertyByName((*it).propertyName.c_str());
//     assert(property);
//     App::PropertyFloat *fProperty = dynamic_cast<App::PropertyFloat *>(property);
//     assert(fProperty);
//     fProperty->setValue(valueIn);
//   }
}

void ExpressionManager::addFormulaLink(const std::string& objectNameIn, const std::string& propertyNameIn, const boost::uuids::uuid& idIn)
{
  FormulaLink virginLink;
  virginLink.objectName = objectNameIn;
  virginLink.propertyName = propertyNameIn;
  virginLink.formulaId = idIn;
  
  formulaLinks.insert(virginLink);
  
  //update current object.
  double value = graphPtr->getFormulaValue(idIn);
  updateLinkedProperty(idIn, value);
//   document->recompute();
}

void ExpressionManager::addFormulaLink(const std::string& objectNameIn, const std::string& propertyNameIn, const std::string& nameIn)
{
  assert(graphPtr->hasFormula(nameIn));
  addFormulaLink(objectNameIn, propertyNameIn, graphPtr->getFormulaId(nameIn));
}

bool ExpressionManager::isObjectLinked(const std::string& objectNameIn) const
{
  const FormulaLinkContainerType::index<FormulaLink::ByObjectName>::type &list = formulaLinks.get<FormulaLink::ByObjectName>();
  FormulaLinkContainerType::index<FormulaLink::ByObjectName>::type::const_iterator it;
  it = list.find(objectNameIn);
  if (it != list.end())
    return true;
  return false;
}

bool ExpressionManager::isPropertyLinked(const std::string& objectNameIn, const std::string& propertyNameIn) const
{
  FormulaLinkContainerType::index<FormulaLink::ByCKey>::type::iterator itBegin, itEnd;
  boost::tie(itBegin, itEnd) = formulaLinks.get<FormulaLink::ByCKey>().equal_range(boost::make_tuple(objectNameIn, propertyNameIn));
  if (itBegin != itEnd)
    return true;
  else
    return false;
}

bool ExpressionManager::isFormulaLinked(const boost::uuids::uuid& idIn) const
{
  FormulaLinkContainerType::index<FormulaLink::ByFormulaId>::type::iterator itBegin, itEnd;
  boost::tie(itBegin, itEnd) = formulaLinks.get<FormulaLink::ByFormulaId>().equal_range(idIn);
  if (itBegin != itEnd)
    return true;
  else
    return false;
}

void ExpressionManager::removeFormulaLink(const std::string& objectNameIn, const std::string& propertyNameIn)
{
  FormulaLinkContainerType::index<FormulaLink::ByCKey>::type::iterator it;
  it = formulaLinks.get<FormulaLink::ByCKey>().find(boost::make_tuple(objectNameIn, propertyNameIn));
  if (it == formulaLinks.get<FormulaLink::ByCKey>().end())
    return;
  formulaLinks.get<FormulaLink::ByCKey>().erase(it);
}

void ExpressionManager::objectDeleted(const std::string& objectNameIn)
{
  //update any linked properties.
  FormulaLinkContainerType::index<FormulaLink::ByObjectName>::type::iterator it, itEnd;
  it = formulaLinks.get<FormulaLink::ByObjectName>().find(objectNameIn);
  itEnd = formulaLinks.get<FormulaLink::ByObjectName>().end();
  while (it != itEnd)
  {
    std::string objectName = it->objectName;
    formulaLinks.get<FormulaLink::ByObjectName>().erase(it);
    it = formulaLinks.get<FormulaLink::ByObjectName>().find(objectNameIn);
    itEnd = formulaLinks.get<FormulaLink::ByObjectName>().end();
  }
}

void ExpressionManager::dumpLinks(std::ostream& stream)
{
  FormulaLinkContainerType::const_iterator it;
  for (it = formulaLinks.begin(); it != formulaLinks.end(); ++it)
  {
    stream << std::left << std::setw(30) << it->objectName <<
      std::setw(30) << it->propertyName <<
      std::setw(50) << idToString(it->formulaId) << std::endl;
  }
}

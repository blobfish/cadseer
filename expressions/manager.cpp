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

#include <QTextStream>

#include <boost/graph/topological_sort.hpp>
#include <boost/graph/adj_list_serialize.hpp>

#include <application/application.h>
#include <project/project.h>
#include <feature/base.h>
#include <feature/parameter.h>
#include <message/dispatch.h>
#include <message/observer.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>

#include <tools/idtools.h>
#include <expressions/edgeproperty.h>
#include <expressions/graph.h>
#include <expressions/manager.h>

//temp.
#include <sstream>

using namespace expr;
using namespace boost::uuids;

Group::Group() : id(gu::createRandomId()), name("default")
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

Manager::Manager()
{
  graphPtr = std::move(std::unique_ptr<GraphWrapper>(new GraphWrapper()));
  
  observer = std::move(std::unique_ptr<msg::Observer>(new msg::Observer()));
  observer->name = "expr::Widget";
  setupDispatcher();
  
  //allgroup id set upon serialize restore.
  allGroup.name = "All";
}

Manager::~Manager()
{

}

GraphWrapper& Manager::getGraphWrapper()
{
  return *graphPtr;
}

void Manager::update()
{
  graphPtr->update();
  dispatchValues();
}

void Manager::writeOutGraph(const std::string& pathName)
{
  graphPtr->writeOutGraph(pathName);
}

void Manager::addFormulaToAllGroup(boost::uuids::uuid idIn)
{
  allGroup.formulaIds.push_back(idIn);
}

void Manager::removeFormulaFromAllGroup(boost::uuids::uuid idIn)
{
  std::vector<boost::uuids::uuid>::iterator it = std::find(allGroup.formulaIds.begin(), allGroup.formulaIds.end(), idIn);
  assert (it != allGroup.formulaIds.end());
  allGroup.formulaIds.erase(it);
}

boost::uuids::uuid Manager::createUserGroup(const std::string& groupNameIn)
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

void Manager::renameUserGroup(const boost::uuids::uuid& idIn, const std::string &newName)
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

void Manager::removeUserGroup(const boost::uuids::uuid& idIn)
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

bool Manager::hasUserGroup(const std::string& groupNameIn) const
{
  for (std::vector<Group>::const_iterator it = userDefinedGroups.begin(); it != userDefinedGroups.end(); ++it)
  {
    if (it->name == groupNameIn)
      return true;
  }
  return false;
}

bool Manager::hasUserGroup(const boost::uuids::uuid& groupIdIn) const
{
  for (std::vector<Group>::const_iterator it = userDefinedGroups.begin(); it != userDefinedGroups.end(); ++it)
  {
    if (it->id == groupIdIn)
      return true;
  }
  return false;
}

void Manager::addFormulaToUserGroup(const boost::uuids::uuid& groupIdIn, const boost::uuids::uuid& formulaIdIn)
{
  for (std::vector<Group>::iterator groupIt = userDefinedGroups.begin(); groupIt != userDefinedGroups.end(); ++groupIt)
  {
    if (groupIt->id == groupIdIn)
    {
      groupIt->formulaIds.push_back(formulaIdIn);
    }
  }
}

bool Manager::doesUserGroupContainFormula(const boost::uuids::uuid& groupIdIn, const boost::uuids::uuid& formulaIdIn)
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

void Manager::removeFormulaFromUserGroup(const boost::uuids::uuid& groupIdIn, const boost::uuids::uuid& formulaIdIn)
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

boost::uuids::uuid Manager::getUserGroupId(const std::string& groupNameIn) const
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

boost::uuids::uuid Manager::getFormulaId(const std::string& nameIn) const
{
  assert(graphPtr->hasFormula(nameIn));
  return (graphPtr->getFormulaId(nameIn));
}

void Manager::setFormulaId(const boost::uuids::uuid &oldIdIn, const boost::uuids::uuid &newIdIn)
{
  assert(graphPtr->hasFormula(oldIdIn));
  
  //remove oldId from the all group and add new id.
  assert (allGroup.containsFormula(oldIdIn));
  allGroup.removeFormula(oldIdIn);
  allGroup.formulaIds.push_back(newIdIn);
  
  graphPtr->setFormulaId(oldIdIn, newIdIn);
}

std::string Manager::getFormulaName(const boost::uuids::uuid& idIn) const
{
  assert(graphPtr->hasFormula(idIn));
  return graphPtr->getFormulaName(idIn);
}

bool Manager::hasFormula(const uuid& idIn) const
{
  return graphPtr->hasFormula(idIn);
}

bool Manager::hasFormula(const std::string& nameIn) const
{
  return graphPtr->hasFormula(nameIn);
}

double Manager::getFormulaValue(const uuid& idIn) const
{
  assert(graphPtr->hasFormula(idIn));
  return graphPtr->getFormulaValue(idIn);
}

void Manager::setFormulaName(const uuid& idIn, const std::string& nameIn)
{
  assert(graphPtr->hasFormula(idIn));
  graphPtr->setFormulaName(idIn, nameIn);
}

void Manager::cleanFormula(const uuid& idIn)
{
  assert(graphPtr->hasFormula(idIn));
  graphPtr->cleanFormula(idIn);
}

bool Manager::hasCycle(const uuid& idIn, std::string& nameOut)
{
  assert(graphPtr->hasFormula(idIn));
  return graphPtr->hasCycle(idIn, nameOut);
}

void Manager::setFormulaDependentsDirty(const uuid& idIn)
{
  assert(graphPtr->hasFormula(idIn));
  graphPtr->setFormulaDependentsDirty(idIn);
}

std::vector< uuid > Manager::getDependentFormulaIds(const uuid& parentIn)
{
  assert(graphPtr->hasFormula(parentIn));
  return graphPtr->getDependentFormulaIds(parentIn);
}

std::vector< uuid > Manager::getAllFormulaIdsSorted() const
{
  return graphPtr->getAllFormulaIdsSorted();
}

std::vector< uuid > Manager::getAllFormulaIds() const
{
  return graphPtr->getAllFormulaIds();
}

std::string Manager::getUserGroupName(const boost::uuids::uuid& groupIdIn) const
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

void Manager::removeFormula(const std::string& nameIn)
{
  assert(graphPtr->hasFormula(nameIn));
  boost::uuids::uuid fId = graphPtr->getFormulaId(nameIn);
  this->removeFormula(fId);
}

void Manager::removeFormula(const boost::uuids::uuid& idIn)
{
  removeFormulaFromAllGroup(idIn);
  for (std::vector<Group>::iterator it = userDefinedGroups.begin(); it != userDefinedGroups.end(); ++it)
  {
    if (it->containsFormula(idIn))
      it->removeFormula(idIn);
  }
  graphPtr->removeFormula(idIn);
  
  //update any linked parameters.
  FormulaLinkContainerType::index<FormulaLink::ByFormulaId>::type::iterator it, it2, itEnd;
  
  boost::tie(it, itEnd) = formulaLinks.get<FormulaLink::ByFormulaId>().equal_range(idIn);
  if (it == itEnd)
    return;
  it2 = it;
  
  for(; it != itEnd; ++it)
  {
    it->parameter->setConstant(true); //change parameter constant to true.
  }
  
  formulaLinks.get<FormulaLink::ByFormulaId>().erase(it2, itEnd);
}

void Manager::addLink(ftr::Parameter *parameterIn, const uuid &formulaIdIn)
{
  FormulaLink virginLink;
  virginLink.parameterId = parameterIn->getId();
  virginLink.formulaId = formulaIdIn;
  virginLink.parameter = parameterIn;
  
  formulaLinks.insert(virginLink);
  
  //update current object.
  double value = graphPtr->getFormulaValue(formulaIdIn);
  parameterIn->setValue(value);
  parameterIn->setConstant(false);
}

void Manager::removeParameterLink(const uuid &parameterIdIn)
{
  //a parameter can only have one link. so no equal range for this.
  
  FormulaLinkContainerType::index<FormulaLink::ByParameterId>::type::iterator it;
  it = formulaLinks.get<FormulaLink::ByParameterId>().find(parameterIdIn);
  assert(it != formulaLinks.get<FormulaLink::ByParameterId>().end()); //use has first.
  
  it->parameter->setConstant(true);
  
  formulaLinks.get<FormulaLink::ByParameterId>().erase(it);
}

bool Manager::hasParameterLink(const uuid &parameterIdIn) const
{
  FormulaLinkContainerType::index<FormulaLink::ByParameterId>::type::iterator it;
  it = formulaLinks.get<FormulaLink::ByParameterId>().find(parameterIdIn);
  return it != formulaLinks.get<FormulaLink::ByParameterId>().end();
}

uuid Manager::getFormulaLink(const uuid &parameterIdIn) const
{
  FormulaLinkContainerType::index<FormulaLink::ByParameterId>::type::iterator it;
  it = formulaLinks.get<FormulaLink::ByParameterId>().find(parameterIdIn);
  assert(it != formulaLinks.get<FormulaLink::ByParameterId>().end()); //use has first.
  
  return it->formulaId;
}

bool Manager::isFormulaLinked(const uuid &formulaIdIn) const
{
  FormulaLinkContainerType::index<FormulaLink::ByFormulaId>::type::iterator it, itEnd;
  
  boost::tie(it, itEnd) = formulaLinks.get<FormulaLink::ByFormulaId>().equal_range(formulaIdIn);
  return it != itEnd;
}

std::vector<uuid> Manager::getParametersLinked(const uuid &formulaIdIn) const
{
  std::vector<uuid> out;
  
  FormulaLinkContainerType::index<FormulaLink::ByFormulaId>::type::iterator it, itEnd;
  boost::tie(it, itEnd) = formulaLinks.get<FormulaLink::ByFormulaId>().equal_range(formulaIdIn);
  for (; it != itEnd; ++it)
    out.push_back(it->parameterId);
  return out;
}

void Manager::dispatchValues()
{
  /* it is safe to just send all the values because, each parameter
   * makes sure the new value is different than the old
   */
  FormulaLinkContainerType::const_iterator it;
  for (it = formulaLinks.begin(); it != formulaLinks.end(); ++it)
  {
    assert(it->parameter);
    it->parameter->setValue(graphPtr->getFormulaValue(it->formulaId));
  }
}

void Manager::dumpLinks(std::ostream& stream)
{
  FormulaLinkContainerType::const_iterator it;
  for (it = formulaLinks.begin(); it != formulaLinks.end(); ++it)
  {
    stream << "parameter id: " << gu::idToString(it->parameterId)
      << "    parameter name: " << std::left << std::setw(20) << it->parameter->getName().toStdString()
      << "    formula id: " << gu::idToString(it->formulaId) << std::endl;
  }
}

void Manager::setupDispatcher()
{
  msg::Mask mask;
  
  mask = msg::Response | msg::Pre | msg::Remove | msg::Feature;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Manager::featureRemovedDispatched, this, _1)));
}

void Manager::featureRemovedDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  prj::Message message = boost::get<prj::Message>(messageIn.payload);
  uuid featureId = message.feature->getId();
  
  const ftr::ParameterVector &pVector = static_cast<app::Application*>(qApp)->getProject()->findFeature(featureId)->getParameterVector();
  
  for (const auto &p : pVector)
  {
    auto &container = formulaLinks.get<FormulaLink::ByParameterId>();
    FormulaLinkContainerType::index<FormulaLink::ByParameterId>::type::iterator it;
    it = container.find(p->getId());
    if (it != container.end())
      container.erase(it);
  }
}

QTextStream& Manager::getInfo(QTextStream &stream) const
{
  stream << endl << QObject::tr("Formulas:") << endl;
  auto ids = getAllFormulaIds();
  for (const auto &id : ids)
  {
    stream << QString::fromStdString(getFormulaName(id))
    << "    " << QString::number(getFormulaValue(id), 'f', 12) << endl;
  }
  
  return stream;
}

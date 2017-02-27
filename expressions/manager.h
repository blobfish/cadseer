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

#ifndef EXPR_MANAGER_H
#define EXPR_MANAGER_H

#include <memory>

#include <boost/uuid/uuid.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/unordered_map.hpp>

#include <expressions/value.h>

class QTextStream;

namespace ftr{class Parameter;}
namespace msg{class Message; class Observer;}

namespace expr{

class GraphWrapper;
struct FormulaLinksWrapper;

/*! @brief A collection of formulas
 * 
 * Group contains a vector of formula ids.
 */
class Group
{
public:
  Group();
  //! Group id.
  boost::uuids::uuid id;
  //! Group name.
  std::string name;
  //! Container for formula ids.
  std::vector<boost::uuids::uuid> formulaIds;
  //! check if group contains the passed in formula id.
  bool containsFormula(const boost::uuids::uuid &fIdIn);
  //! remove formula with passed in id from this group.
  void removeFormula(const boost::uuids::uuid &fIdIn);
};

/*! @brief Container to store and manage expressions.
 * 
 * Expression data is stored in the member #graphPtr. Organization of the
 * expressions are stored in 2 groups: #allGroup and #userDefinedGroups. Because
 * naming can be volatile, groups and expressions have an id so they can
 * be referenced in a static way. Use ids over names whenever possible.
 * Groups only contain ids to the expressions they hold.
 * When modifying
 * any expression data, use the beginTransaction and commitTransaction or rejectTransaction.
 * This allows notifications to be send of individual formula modifications, along with
 * activating the undo/redo functionality.
 */
class Manager
{
public:
  Manager();
  ~Manager();
  
  //! Cycle the graph and recalculate dirty nodes.
  void update();
  
  //! return a reference to the GraphWrapper 
  GraphWrapper& getGraphWrapper();
  
  //! write a graphviz file for the graph.
  void writeOutGraph(const std::string &pathName);
  
  //! Add formula to all group.
  void addFormulaToAllGroup(boost::uuids::uuid idIn);
  
  //! Remove formula form all group.
  void removeFormulaFromAllGroup(boost::uuids::uuid idIn);
  
  //! Test if a user group with name exist.
  bool hasUserGroup(const std::string &groupNameIn) const;
  
  //! Test if a user group with id exists.
  bool hasUserGroup(const boost::uuids::uuid &groupIdIn) const;
  
  /*! @brief Create a new user group.
   * 
   * Assert if group of groupNameIn already exists; @see hasUserGroup
   */
  boost::uuids::uuid createUserGroup(const std::string &groupNameIn);
  
  /*! @brief Rename the group.
   * 
   * Assert if newName is not unique or no such group with idIn. @see hasUserGroup
   */
  void renameUserGroup(const boost::uuids::uuid &idIn, const std::string &newName);
  
  //! Remove user group. Assert if no such group. @see hasUserGroup
  void removeUserGroup(const boost::uuids::uuid &idIn);
  
  //! Add formula reference to user group.
  void addFormulaToUserGroup(const boost::uuids::uuid &groupIdIn, const boost::uuids::uuid &formulaIdIn);
  
  //! Test for formula in group.
  bool doesUserGroupContainFormula(const boost::uuids::uuid &groupIdIn, const boost::uuids::uuid &formulaIdIn);
  
  /*! @brief Remove formula from group.
   * 
   * Assert if group doesn't exist or doesn't contain formula. @see hasUserGroup @see doesUserGroupContainFormula
   */
  void removeFormulaFromUserGroup(const boost::uuids::uuid &groupIdIn, const boost::uuids::uuid &formulaIdIn);
  
  /*! @brief Get name of user group with id.
   * 
   * Assert if no user group with id. @see hasUserGroup
   */
  std::string getUserGroupName(const boost::uuids::uuid &groupIdIn) const;
  
  /*! @brief Get group id for group with name.
   * 
   * Assert if no user group exists with name. @see hasUserGroup
   */
  boost::uuids::uuid getUserGroupId(const std::string &groupNameIn) const;
  
  boost::uuids::uuid getFormulaId(const std::string &nameIn) const;
  void setFormulaId(const boost::uuids::uuid &oldIdIn, const boost::uuids::uuid &newIdIn);
  std::string getFormulaName(const boost::uuids::uuid &idIn) const;
  bool hasFormula(const std::string &nameIn) const;
  bool hasFormula(const boost::uuids::uuid &idIn) const;
  Value getFormulaValue(const boost::uuids::uuid &idIn) const;
  ValueType getFormulaValueType(const boost::uuids::uuid &idIn) const;
  void setFormulaName(const boost::uuids::uuid &idIn, const std::string &nameIn);
  void cleanFormula(const boost::uuids::uuid &idIn);
  bool hasCycle(const boost::uuids::uuid &idIn, std::string &nameOut);
  void setFormulaDependentsDirty(const boost::uuids::uuid &idIn);
  std::vector<boost::uuids::uuid> getDependentFormulaIds(const boost::uuids::uuid &parentIn);
  std::vector<boost::uuids::uuid> getAllFormulaIdsSorted() const;
  std::vector<boost::uuids::uuid> getAllFormulaIds() const;
  
  //@{
  //! Remove formula from both groups and graph. DON'T call remove formula with a dirty graph. @see update
  void removeFormula(const boost::uuids::uuid &idIn);
  void removeFormula(const std::string &nameIn);
  //@}
  
  //! Link parameter to formula. first is parameter ptr. last is formula id.
  void addLink(ftr::Parameter *, const boost::uuids::uuid &);
  //! erase link by parameter id.
  void removeParameterLink(const boost::uuids::uuid &);
  //! checks if parameter is linked by parameter id. 
  bool hasParameterLink(const boost::uuids::uuid &) const;
  //! get formula id by parameter id.
  boost::uuids::uuid getFormulaLink(const boost::uuids::uuid &) const;
  //! check if a formula has been linked by formula id.
  bool isFormulaLinked(const boost::uuids::uuid &) const;
  //! get all the parameter ids linked to formula id passed in.
  std::vector<boost::uuids::uuid> getParametersLinked(const boost::uuids::uuid &) const;
  //! Dispatch values to parameters.
  void dispatchValues();
  //! Write a list of links to stream.
  void dumpLinks(std::ostream &stream);
  //! get all the links. created for serialize.
  const FormulaLinksWrapper& getLinkContainer() const{return *formulaLinksPtr;}
  
  //! Contains an id to all existing formulas.
  Group allGroup;
  
  //! Contains ids for a subset of expressions.
  std::vector<Group> userDefinedGroups;
  
  QTextStream& getInfo(QTextStream&) const;
  
private:
  //! Pointer to GraphWrapper.
  std::unique_ptr<GraphWrapper> graphPtr;
  //! Container for formula to properties links.
  std::unique_ptr<FormulaLinksWrapper> formulaLinksPtr;
  
  std::unique_ptr<msg::Observer> observer;
  void setupDispatcher();
  void featureRemovedDispatched(const msg::Message &);
};

}

#endif // EXPR_MANAGER_H

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

#ifndef EXPRESSIONMANAGER_H
#define EXPRESSIONMANAGER_H

#include <memory>

#include <boost/uuid/uuid.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/unordered_map.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace expr{

class GraphWrapper;

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

/*! @brief Link between formula and properties
 * 
 * boost multi_index link.
 */
struct FormulaLink
{
  std::string objectName;
  std::string propertyName;
  boost::uuids::uuid formulaId;
  
  //@{
  //! used as tags.
  struct ByObjectName{};
  struct ByPropertyName{}; 
  struct ByFormulaId{};
  struct ByCKey{};
  //@}
};

//! Container to hold a transaction.
typedef boost::tuple<Group, std::vector<Group>, boost::shared_ptr<GraphWrapper> > TransContainer;
//! Array to hold transactions.
typedef std::vector<TransContainer> TransArray;
//! Temporary storage of all formula values.
typedef boost::unordered_map<boost::uuids::uuid, double> ValueCache;
namespace BMI = boost::multi_index;
//! Container type to hold formula to property links.
typedef boost::multi_index_container
<
  FormulaLink,
  BMI::indexed_by
  <
    BMI::ordered_non_unique
    <
      BMI::tag<FormulaLink::ByObjectName>,
      BMI::member<FormulaLink, std::string, &FormulaLink::objectName>
    >,
    BMI::ordered_unique
    <
      BMI::tag<FormulaLink::ByCKey>,
      BMI::composite_key
      <
        FormulaLink,
        BMI::member<FormulaLink, std::string, &FormulaLink::objectName>,
        BMI::member<FormulaLink, std::string, &FormulaLink::propertyName>
      >
    >,
    BMI::ordered_non_unique
    <
      BMI::tag<FormulaLink::ByFormulaId>,
      BMI::member<FormulaLink, boost::uuids::uuid, &FormulaLink::formulaId>
    >
  >
> FormulaLinkContainerType;

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
class ExpressionManager
{
public:
  ExpressionManager();
  ~ExpressionManager();
  
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
  std::string getFormulaName(const boost::uuids::uuid &idIn) const;
  
  //@{
  //! id, string conversions.
  static boost::uuids::uuid stringToId(const std::string &stringIn);
  static std::string idToString(const boost::uuids::uuid &idIn);
  //@}
  
  //@{
  //! Remove formula from both groups and graph.
  void removeFormula(const boost::uuids::uuid &idIn);
  void removeFormula(const std::string &nameIn);
  //@}
  
  //! Start a transaction.
  void beginTransaction();
  //! Commit a transaction.
  void commitTransaction();
  //! Reset a transaction.
  void rejectTransaction();
  //! Move back one index in transaction array.
  void undo();
  //! Move forward one index in transaction array.
  void redo();
  
  //! Link property to formula.
  void addFormulaLink(const std::string &objectNameIn, const std::string &propertyNameIn, const boost::uuids::uuid &idIn);
  //! Link property to formula.
  void addFormulaLink(const std::string &objectNameIn, const std::string &propertyNameIn, const std::string &nameIn);
  //! Test whether an object has formula links.
  bool isObjectLinked(const std::string &objectNameIn) const;
  //! Test whether a property has formula links.
  bool isPropertyLinked(const std::string &objectNameIn, const std::string &propertyNameIn) const;
  //! Test whether a formula has links.
  bool isFormulaLinked(const boost::uuids::uuid &idIn) const;
  //! Remove link to property.
  void removeFormulaLink(const std::string &objectNameIn, const std::string &propertyNameIn);
  //! Remove all links to object.
  void objectDeleted(const std::string &objectNameIn);
  //! Write a list of links to stream.
  void dumpLinks(std::ostream &stream);
  
  //! Contains an id to all existing formulas.
  Group allGroup;
  
  //! Contains ids for a subset of expressions.
  std::vector<Group> userDefinedGroups;
  
  
private:
  //! Restore the state from an index. used in undo/redo.
  void restoreState(const std::size_t &index);
  //! Store all current formula values.
  void generateValueCache(ValueCache &out) const;
  //! Notifications of formula changes.
  void dispatchValueChanges(const ValueCache &virginCache);
  //! Update properties linked to formula.
  void updateLinkedProperty(const boost::uuids::uuid idIn, const double valueIn);
  
  //! Pointer to GraphWrapper.
  boost::shared_ptr<GraphWrapper> graphPtr;
  //! Array of transactions
  TransArray transArray;
  //! State of transaction. True is transaction started, but not finished. False is no transaction
  bool transactionState;
  //! Current location in the transaction array.
  std::size_t transactionPosition;
  //! Cache pre-recompute formula values for post-recompute comparison.
  ValueCache valueCache;
  //! Container for formula to properties links.
  FormulaLinkContainerType formulaLinks;
};

}

#endif // EXPRESSIONMANAGER_H

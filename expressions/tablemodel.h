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

#ifndef TABLEMODEL_H
#define TABLEMODEL_H

#include <memory>
#include <vector>

#include <boost/uuid/uuid.hpp>
#include <boost/shared_ptr.hpp>

#include <QtCore/QAbstractTableModel>
#include <QtGui/QSortFilterProxyModel>

#include <selection/container.h>

namespace msg{class Message; class Observer;}

namespace expr{
  class ExpressionManager;
  class Group;
  class StringTranslator;

/*! @brief Main interface between ExpressionManager and Qt MVC
 * 
 * LastFailed* members are used to store information from a failed parse.
 * The delegate uses #lastFailedPosition and #lastFailedText to restore and highlight
 * failing text. #lastFailedMessage is shown to user in a messagebox.
 */
class TableModel : public QAbstractTableModel
{
  Q_OBJECT
public:
    explicit TableModel(expr::ExpressionManager &eManagerIn, QObject* parent = 0);
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    virtual QStringList mimeTypes() const override;
    QMimeData* mimeData (const QModelIndexList &) const override;
    //! Get all user defined groups.
    std::vector<expr::Group> getGroups();
    //! Add formulas to a group.
    void addFormulaToGroup(const QModelIndex &indexIn, const QString &groupIdIn);
    //! Remove formulas
    void removeFormula(const QModelIndexList &indexesIn);
    //! Export expressions to a text file.
    void exportExpressions(QModelIndexList &indexesIn, std::ostream &streamIn) const;
    //! Import expressions from a text file. Will not over write existing expressions.
    void importExpressions(std::istream &streamIn, boost::uuids::uuid groupId);
    
    //! The position causing the parse to fail.
    int lastFailedPosition;
    //! The text responsible for a failed parse.
    QString lastFailedText;
    //! Message indicating the cause of the failure.
    QString lastFailedMessage;
    
    //! Add a new row to the model.
    void addDefaultRow();
    
    //! Need to get the string translator to line edit widget.
    StringTranslator* getStringTranslator() const;
    
private:
  //! Manager containing expressions and groups.
  expr::ExpressionManager &eManager;
  //! Translator to interface with the manager.
  boost::shared_ptr<StringTranslator> sTranslator;
  //! tableview calls into ::data every paint event. Way too many! cache rhs strings for speed.
  typedef std::map<boost::uuids::uuid, std::string> IdToRhsMap;
  mutable IdToRhsMap idToRhsMap;
  void buildOrModifyMapEntry(const boost::uuids::uuid &, const std::string&) const;
  std::string getRhs(const boost::uuids::uuid&) const;
  void removeRhs(const boost::uuids::uuid&) const;
};

//! ProxyModel for behaviour common to both AllProxy and GroupProxy
class BaseProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
  explicit BaseProxyModel(QObject *parent = 0);
  
  /*! @brief Intercept for sort.
   * 
   * Calls parent for setting of data.
   * Calls a queued sort after the the 2nd column is edited for smooth user insert behaviour.
   */
  virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
private Q_SLOTS:
  //! Trigger a sort of the model. 
  void delayedSortSlot();
};

/*! @brief Proxy model for the grouping view.
 */
class GroupProxyModel : public BaseProxyModel
{
  Q_OBJECT
public:
  explicit GroupProxyModel(expr::ExpressionManager &eManagerIn, boost::uuids::uuid groupIdIn, QObject *parent = 0);
  //! Add a new formula to the source model and add it to the group.
  QModelIndex addDefaultRow();
  //! Remove the expressions from the group.
  void removeFromGroup(const QModelIndexList &indexesIn);
  //! Rename the user group this model is referencing.
  bool renameGroup(const std::string &newName);
  //! Get the name of the user group this model is referencing.
  std::string getGroupName() const;
  //! Remove the group from manager and signal tab widget remove view.
  void removeGroup();
  //! Import the expressions from the file and add to the user group.
  void importExpressions(std::istream &streamIn);
  
protected:
  virtual bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
  
public Q_SLOTS:
  /*! @brief invalidate the filter and sort.
   * 
   * Changes through in any one tab can modify the underlying data to have an effect on the other tabs.
   * We allow the hidden tabs to fall out of sync with the underlying model. So for a 
   * synchronized interface, we call this function in response to the view being shown to reflect
   * any changes made from other tabs. Now this could all be avoided by using the proxies dynamicSortFilter
   * setting. But then we loose control of when the sort happens and user chases records around the view.
   * This function is also called from removeFromGroup in a queued invoke to reflect group removal.
   */
  void refreshSlot();
private:
  //! Reference to manager
  expr::ExpressionManager &eManager;
  //! Group id this model is referencing.
  boost::uuids::uuid groupId;
};

/* this one is a little different in that we are NOT letting it get of sync.
 * I don't like this but I don't want to an observer to both model and view.
 */
//! @brief Proxy model for selection expressions view.
class SelectionProxyModel : public BaseProxyModel
{
  Q_OBJECT
public:
  explicit SelectionProxyModel(expr::ExpressionManager &, QObject *parent = 0);
  virtual ~SelectionProxyModel();
  
protected:
  virtual bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
  
private:
  //! Reference to manager
  expr::ExpressionManager &eManager;
  std::unique_ptr<msg::Observer> observer;
  void setupDispatcher();
  slc::Containers containers;
  void selectionAdditionDispatched(const msg::Message&);
  void selectionSubtractionDispatched(const msg::Message&);
};

//! @brief Proxy model for all expressions view.
class AllProxyModel : public BaseProxyModel
{
  Q_OBJECT
public:
  explicit AllProxyModel(QObject *parent = 0);
  /*! @brief invalidate sort.
   * Same reason as @see GroupProxyModel.
   */
  void refresh();
};
}

#endif // TABLEMODEL_H

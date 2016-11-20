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
#include <fstream>
#include <boost/uuid/uuid_io.hpp>

#include <QtCore/QTextStream>

#include <expressions/expressiongraph.h>
#include <expressions/expressionmanager.h>
#include <expressions/stringtranslator.h>
#include <expressions/tablemodel.h>

using namespace expr;

TableModel::TableModel(ExpressionManager &eManagerIn, QObject* parent):
  QAbstractTableModel(parent), lastFailedPosition(-1), lastFailedText(""), lastFailedMessage(""),
  eManager(eManagerIn), sTranslator(new StringTranslator(eManager))
{

}

int TableModel::columnCount(const QModelIndex&) const
{
  return 3;
}

int TableModel::rowCount(const QModelIndex&) const
{
  return eManager.allGroup.formulaIds.size();
}

QVariant TableModel::data(const QModelIndex& index, int role) const
{
  if ((role == Qt::DisplayRole) | (role == Qt::EditRole))
  {
    assert(eManager.allGroup.formulaIds.size() > static_cast<std::size_t>(index.row()));
    boost::uuids::uuid currentId = eManager.allGroup.formulaIds.at(index.row());
    Vertex fVertex = eManager.getGraphWrapper().getFormulaVertex(currentId);
    if (index.column() == 0)
      return QString::fromStdString(eManager.getGraphWrapper().getFormulaName(fVertex));
    if (index.column() == 1)
    {
      return QString::fromStdString(sTranslator->buildStringRhs(fVertex));
    }
    if (index.column() == 2)
      return eManager.getGraphWrapper().getFormulaValue(fVertex);
  }
  
  if (role == Qt::UserRole)
  {
    std::stringstream stream;
    stream << eManager.allGroup.formulaIds.at(index.row());
    return QString::fromStdString(stream.str());
  }
  
  if (role == (Qt::UserRole + 1))
    return lastFailedMessage;
  return QVariant();
}

QVariant TableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
  {
    if (section == 0)
      return tr("Name");
    if (section == 1)
      return tr("Expression");
    if (section == 2)
      return tr("Value");
  }
  return QAbstractItemModel::headerData(section, orientation, role);
}

Qt::ItemFlags TableModel::flags(const QModelIndex& index) const
{
  if (index.column() == 2)
    return Qt::NoItemFlags;
  return Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled;
}

bool TableModel::setData(const QModelIndex& index, const QVariant& value, int)
{
  boost::uuids::uuid fId = ExpressionManager::stringToId(this->data(index, Qt::UserRole).toString().toStdString());
  assert(eManager.getGraphWrapper().hasFormula(fId));
  if (index.column() == 0)
  {
    //rename formula.
    std::string oldName = eManager.getGraphWrapper().getFormulaName(fId);
    std::string newName = value.toString().toStdString();
    if (oldName == newName)
      return true;
    if (eManager.getGraphWrapper().hasFormula(newName))
    {
      lastFailedMessage = tr("Formula name already exists");
      return false;
    }
    eManager.getGraphWrapper().setFormulaName(fId, newName);
  }
  if (index.column() == 1)
  {
    std::string formulaName = eManager.getGraphWrapper().getFormulaName(fId);
    //store current expression rhs to restore if needed.
    //not really liking the construction of the main LHS. Shouldn't need it.
    std::ostringstream oldStream;
    oldStream << formulaName << "=" << sTranslator->buildStringRhs(eManager.getGraphWrapper().getFormulaVertex(fId));
    
    //reparse expression.
    eManager.getGraphWrapper().cleanFormula(fId);
    lastFailedText = value.toString();
    
    std::ostringstream stream;
    stream << formulaName << "=" << value.toString().toStdString();
    if (sTranslator->parseString(stream.str()) == StringTranslator::ParseFailed)
    {
      lastFailedMessage = tr("Parsing failed");
      //we are cacheing the failed position for now because the following call immediately over rights the value.
      lastFailedPosition = sTranslator->failedPosition - formulaName.size() + 1;
      //when parsing fails, translator reverts any changes made during parse. So just add the original.
      if (sTranslator->parseString(oldStream.str()) == StringTranslator::ParseFailed)
      {
        std::cout << "couldn't restore formula in tableModel::setdata" << std::endl;
        assert(0);
      }
      eManager.update();
      return false;
    }
    std::string cycleName;
    if (eManager.getGraphWrapper().hasCycle(fId, cycleName))
    {
      lastFailedMessage = tr("Cycle detected with ") + QString::fromStdString(cycleName);
      eManager.getGraphWrapper().cleanFormula(fId);
      if (sTranslator->parseString(oldStream.str()) == StringTranslator::ParseFailed)
      {
        std::cout << "couldn't restore formula, from cycle, in tableModel::setdata" << std::endl;
        assert(0);
      }
      lastFailedPosition = value.toString().indexOf(QString::fromStdString(cycleName));
      eManager.update();
      return false;
    }
    //why don't I have to tell the model to update the dependent formulas?
    eManager.getGraphWrapper().setFormulaDependentsDirty(fId);
    eManager.update();
  }
  lastFailedText.clear();
  lastFailedMessage.clear();
  lastFailedPosition = 0;
  return true;
}

std::vector<Group> TableModel::getGroups()
{
  return eManager.userDefinedGroups;
}

void TableModel::addFormulaToGroup(const QModelIndex& indexIn, const QString& groupIdIn)
{
  boost::uuids::uuid groupId = ExpressionManager::stringToId(groupIdIn.toStdString());
  boost::uuids::uuid formulaId = ExpressionManager::stringToId(this->data(indexIn, Qt::UserRole).toString().toStdString());
  assert(eManager.hasUserGroup(groupId));
  assert(eManager.getGraphWrapper().hasFormula(formulaId));
  eManager.addFormulaToUserGroup(groupId, formulaId);
}

void TableModel::addDefaultRow()
{
  std::string name;
  
  for (int nameIndex = 0; nameIndex < 100000; ++nameIndex) //more than 100000? something is wrong!
  {
    std::ostringstream stream;
    stream << "Default_" << nameIndex;
    if (!eManager.getGraphWrapper().hasFormula(stream.str()))
    {
      stream << "=1.0";
      name = stream.str();
      break;
    }
  }
  assert(!name.empty());
  
  this->beginInsertRows(QModelIndex(), eManager.allGroup.formulaIds.size(), eManager.allGroup.formulaIds.size());
  sTranslator->parseString(name);
  eManager.update();
  this->endInsertRows();
}

void TableModel::removeFormula(const QModelIndexList &indexesIn)
{
  std::vector<int> mappedRows;
  QMap<int, QPersistentModelIndex> indexMap;
  QModelIndexList::const_iterator it = indexesIn.constBegin();
  for (; it != indexesIn.constEnd(); ++it)
  {
    int currentRow = (*it).row();
    if (std::find(mappedRows.begin(), mappedRows.end(), currentRow) == mappedRows.end())
      mappedRows.push_back(currentRow);
    indexMap.insert(currentRow, QPersistentModelIndex(this->index((*it).row(), 0)));
  }
  std::sort(mappedRows.begin(), mappedRows.end());
  std::reverse(mappedRows.begin(), mappedRows.end());
  
  for (std::vector<int>::const_iterator rowIt = mappedRows.begin(); rowIt != mappedRows.end(); ++rowIt)
  {
    boost::uuids::uuid currentId = ExpressionManager::stringToId
      (this->data(indexMap.value(*rowIt), Qt::UserRole).toString().toStdString());
    this->beginRemoveRows(QModelIndex(), indexMap.value(*rowIt).row(), indexMap.value(*rowIt).row());
    eManager.removeFormula(currentId);
    this->endRemoveRows();
  }
}

void TableModel::exportExpressions(QModelIndexList& indexesIn, std::ostream &streamIn) const
{
  std::set<boost::uuids::uuid> selectedIds;
  QModelIndexList::const_iterator it;
  for (it = indexesIn.constBegin(); it != indexesIn.constEnd(); ++it)
    selectedIds.insert(ExpressionManager::stringToId(this->data(*it, Qt::UserRole).toString().toStdString()));
  
  //loop through all selected ids and get their dependents.
  std::vector<boost::uuids::uuid> dependentIds;
  for (it = indexesIn.constBegin(); it != indexesIn.constEnd(); ++it)
  {
    boost::uuids::uuid currentId = ExpressionManager::stringToId(this->data(*it, Qt::UserRole).toString().toStdString());
    std::vector<boost::uuids::uuid> tempIds = eManager.getGraphWrapper().getDependentFormulaIds(currentId);
    std::copy(tempIds.begin(), tempIds.end(), std::back_inserter(dependentIds));
  }
  
  //add the dependentIds to the selected set.
  std::copy(dependentIds.begin(), dependentIds.end(), std::inserter(selectedIds, selectedIds.begin()));
  
  //going to loop through all formula ids and see if they are present in the selected ids.
  //this allows us to write the formulas out in a dependent order, so the importing will build the formula
  //in the correct order.
  std::vector<boost::uuids::uuid> allFormulaIds = eManager.getGraphWrapper().getAllFormulaIdsSorted();
  std::vector<boost::uuids::uuid>::const_iterator fIt;
  for (fIt = allFormulaIds.begin(); fIt != allFormulaIds.end(); ++fIt)
  {
    if(selectedIds.find(*fIt) == selectedIds.end())
      continue;
    streamIn << sTranslator->buildStringAll(eManager.getGraphWrapper().getFormulaVertex(*fIt)) << std::endl;
  }
}

void TableModel::importExpressions(std::istream &streamIn, boost::uuids::uuid groupId)
{
  //preprocess the file.
  std::vector<std::string> filteredExpressions;
  
  std::string lineBuffer;
  while(std::getline(streamIn, lineBuffer))
  {
    if(lineBuffer.empty())
      continue;
    std::size_t position = lineBuffer.find('=');
    if (position == std::string::npos)
      continue;
    std::string name = lineBuffer.substr(0, position);
    name.erase(0, name.find_first_not_of(' '));
    name.erase(name.find_last_not_of(' ') + 1);
    if (name.empty())
      continue;
    //don't import over an expression.
    if (eManager.getGraphWrapper().hasFormula(name))
      continue;
    filteredExpressions.push_back(lineBuffer);
  }
  if (filteredExpressions.empty())
    return;
  
  if (!groupId.is_nil())
    assert(eManager.hasUserGroup(groupId));
  
  int currentSize = eManager.allGroup.formulaIds.size();
  int countAdded = 0;
  std::vector<std::string>::const_iterator it;
  for (it = filteredExpressions.begin(); it != filteredExpressions.end(); ++it)
  {
    if (sTranslator->parseString(*it) != StringTranslator::ParseSucceeded)
      continue;
    if (!groupId.is_nil())
    {
      boost::uuids::uuid fId = eManager.getGraphWrapper().getId(sTranslator->getFormulaNodeOut());
      eManager.addFormulaToUserGroup(groupId, fId);
    }
    countAdded++;
  }
  eManager.update();
  
  /* ideally I would do the 'beginInsertRows' call before actually modifing the underlying
   * model data. The problem is, I don't know how many rows are going to be added until I
   * get through the parsing. If parsing fails on a particular expression it gets skipped.
   */
  this->beginInsertRows(QModelIndex(), currentSize, currentSize + countAdded - 1);
  this->endInsertRows();
}

BaseProxyModel::BaseProxyModel(QObject* parent): QSortFilterProxyModel(parent)
{

}

//this allows us to add a row at the bottom of the table. then edit the name, then tab and edit formula and then sort.
bool BaseProxyModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  bool out = QSortFilterProxyModel::setData(index, value, role);
  if (index.column() == 1 && out)
    QMetaObject::invokeMethod(this, "delayedSortSlot", Qt::QueuedConnection);
  return out;
}

void BaseProxyModel::delayedSortSlot()
{
  this->sort(0);
}

GroupProxyModel::GroupProxyModel(ExpressionManager &eManagerIn, boost::uuids::uuid groupIdIn, QObject* parent)
  : BaseProxyModel(parent), eManager(eManagerIn), groupId(groupIdIn)
{

}

bool GroupProxyModel::filterAcceptsRow(int source_row, const QModelIndex&) const
{
  boost::uuids::uuid formulaId;
  std::stringstream stream;
  stream << sourceModel()->data(sourceModel()->index(source_row, 0), Qt::UserRole).toString().toStdString();
  stream >> formulaId;
  
  return eManager.doesUserGroupContainFormula(groupId, formulaId);
}

QModelIndex GroupProxyModel::addDefaultRow()
{
  TableModel *myModel = dynamic_cast<TableModel *>(this->sourceModel());
  assert(myModel);
  int rowCount = myModel->rowCount();
  myModel->addDefaultRow();
  QModelIndex sourceIndex = myModel->index(rowCount, 0);
  boost::uuids::uuid formulaId = ExpressionManager::stringToId(myModel->data(sourceIndex, Qt::UserRole).toString().toStdString());
  eManager.addFormulaToUserGroup(groupId, formulaId);
  this->invalidateFilter();
  QModelIndex out = this->mapFromSource(sourceIndex);
  assert(out.isValid());
  return out;
}

void GroupProxyModel::removeFromGroup(const QModelIndexList &indexesIn)
{
  std::vector <int> processedRows;
  QModelIndexList::const_iterator it;
  for (it = indexesIn.constBegin(); it != indexesIn.constEnd(); ++it)
  {
    QModelIndex currentSource = this->mapToSource(*it);
    assert(currentSource.isValid());
    if (std::find(processedRows.begin(), processedRows.end(),currentSource.row()) != processedRows.end())
      continue;
    processedRows.push_back(currentSource.row());
    boost::uuids::uuid id = ExpressionManager::stringToId
      (this->sourceModel()->data(currentSource, Qt::UserRole).toString().toStdString());
    assert(eManager.doesUserGroupContainFormula(groupId, id));
    eManager.removeFormulaFromUserGroup(groupId, id);
  }
  QMetaObject::invokeMethod(this, "refreshSlot", Qt::QueuedConnection);
}

std::string GroupProxyModel::getGroupName() const
{
  assert(eManager.hasUserGroup(groupId));
  return eManager.getUserGroupName(groupId);
}

bool GroupProxyModel::renameGroup(const std::string& newName)
{
  assert(eManager.hasUserGroup(groupId));
  if (eManager.hasUserGroup(newName))
    return false;
  eManager.renameUserGroup(groupId, newName);
  return true;
}

void GroupProxyModel::refreshSlot()
{
  this->invalidateFilter();
  this->sort(0);
}

void GroupProxyModel::removeGroup()
{
  assert(eManager.hasUserGroup(groupId));
  eManager.removeUserGroup(groupId);
}

void GroupProxyModel::importExpressions(std::istream &streamIn)
{
  TableModel *tableModel = dynamic_cast<TableModel *>(this->sourceModel());
  assert(tableModel);
  tableModel->importExpressions(streamIn, groupId);
}

AllProxyModel::AllProxyModel(QObject* parent): BaseProxyModel(parent)
{
}

void AllProxyModel::refresh()
{
  this->sort(0);
}

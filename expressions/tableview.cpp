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

#include <iostream>
#include <assert.h>
#include <fstream>

#include <boost/uuid/nil_generator.hpp>

#include <QtGui/QLineEdit>
#include <QtGui/QHeaderView>
#include <QAction>
#include <QShowEvent>
#include <QMenu>
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>

#include <expressions/tableview.h>
#include <expressions/tablemodel.h>
#include <expressions/expressionmanager.h>

using namespace expr;

static QString defaultDirectory = QDir::homePath();

TableViewAll::TableViewAll(QWidget* parentIn): QTableView(parentIn)
{
  buildActions();
  this->verticalHeader()->setVisible(false);
}

void TableViewAll::addDefaultRowSlot()
{
  TableModel *myModel = static_cast<TableModel *>(static_cast<AllProxyModel*>(this->model())->sourceModel());
  int rowCount = myModel->rowCount();
  myModel->addDefaultRow();
  QModelIndex index = this->model()->index(rowCount, 0);
  
  this->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
  this->setCurrentIndex(index);
  QMetaObject::invokeMethod(this, "edit", Qt::QueuedConnection, Q_ARG(QModelIndex, index));
}

void TableViewAll::addToGroupSlot()
{
  QAction *action = dynamic_cast<QAction *>(QObject::sender());
  assert(action);
  
  QSortFilterProxyModel *pModel = dynamic_cast<QSortFilterProxyModel *>(this->model());
  assert(pModel);
  TableModel *myModel = dynamic_cast<TableModel *>(pModel->sourceModel());
  assert(myModel);
  
  QModelIndexList indexes = this->selectedIndexes();
  for (QModelIndexList::iterator it = indexes.begin(); it != indexes.end(); ++it)
  {
    QModelIndex sourceIndex = pModel->mapToSource(*it);
    myModel->addFormulaToGroup(sourceIndex, action->data().toString());
  }
}

void TableViewAll::removeFormulaSlot()
{
  QSortFilterProxyModel *pModel = dynamic_cast<QSortFilterProxyModel *>(this->model());
  assert(pModel);
  TableModel *myModel = dynamic_cast<TableModel *>(pModel->sourceModel());
  assert(myModel);
  
  QModelIndexList indexes = this->selectedIndexes();
  QModelIndexList sourceIndexes;
  for (QModelIndexList::iterator it = indexes.begin(); it != indexes.end(); ++it)
    sourceIndexes.append(pModel->mapToSource(*it));
  myModel->removeFormula(sourceIndexes);
}

void TableViewAll::exportFormulaSlot()
{
  QSortFilterProxyModel *pModel = dynamic_cast<QSortFilterProxyModel *>(this->model());
  assert(pModel);
  TableModel *myModel = dynamic_cast<TableModel *>(pModel->sourceModel());
  assert(myModel);
  
  QString filePath = QFileDialog::getSaveFileName(this, tr("Save File Name"), defaultDirectory, tr("Expressions (*.exp)"));
  if (filePath.isEmpty())
    return;
  if (!filePath.endsWith(".exp"))
    filePath += ".exp";
  QFileInfo path(filePath);
  defaultDirectory = path.absolutePath();
  
  QModelIndexList indexes = this->selectedIndexes();
  QModelIndexList sourceIndexes;
  for (QModelIndexList::iterator it = indexes.begin(); it != indexes.end(); ++it)
    sourceIndexes.append(pModel->mapToSource(*it));
  
  std::ofstream fileStream;
  fileStream.open(filePath.toStdString().c_str());
  if (!fileStream.is_open())
  {
    QMessageBox::critical(this, tr("Error:"), tr("Couldn't Open File"));
    return;
  }
  myModel->exportExpressions(sourceIndexes, fileStream);
  fileStream.close();
}

void TableViewAll::importFormulaSlot()
{
  QSortFilterProxyModel *pModel = dynamic_cast<QSortFilterProxyModel *>(this->model());
  assert(pModel);
  TableModel *myModel = dynamic_cast<TableModel *>(pModel->sourceModel());
  assert(myModel);
  
  QString filePath = QFileDialog::getOpenFileName(this, tr("Open File Name"), defaultDirectory, tr("Expressions (*.exp)"));
  if (filePath.isEmpty())
    return;
  QFileInfo path(filePath);
  defaultDirectory = path.absolutePath();
  
  std::ifstream fileStream;
  fileStream.open(filePath.toStdString().c_str());
  if (!fileStream.is_open())
    return;
  
  myModel->importExpressions(fileStream, boost::uuids::nil_generator()());
}

void TableViewAll::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    AllProxyModel *model = dynamic_cast<AllProxyModel *>(this->model());
    assert(model);
    model->refresh();
}

void TableViewAll::contextMenuEvent(QContextMenuEvent* event)
{
  QMenu menu;
  menu.addAction(addFormulaAction);
  menu.addAction(removeFormulaAction);

  //add to group sub menu.
  QSortFilterProxyModel *pModel = dynamic_cast<QSortFilterProxyModel *>(this->model());
  assert(pModel);
  TableModel *tModel = dynamic_cast<TableModel *>(pModel->sourceModel());
  assert(tModel);
  std::vector<Group> groups = tModel->getGroups();
  QMenu *groupMenu = menu.addMenu(tr("Add To Group"));
  groupMenu->setDisabled(true); //default to off. turn on below.
  for (std::vector<Group>::iterator it = groups.begin(); it != groups.end(); ++it)
  {
    QAction *currentAction = groupMenu->addAction(QString::fromStdString(it->name));
    currentAction->setData(QVariant(QString::fromStdString(ExpressionManager::idToString(it->id))));
    connect(currentAction, SIGNAL(triggered()), this, SLOT(addToGroupSlot()));
  }
    
  menu.addSeparator();
  
  menu.addAction(exportFormulaAction);
  menu.addAction(importFormulaAction);
  
  menu.addSeparator();
  
  //add Group.
  menu.addAction(addGroupAction);
  
  if(!this->selectionModel()->hasSelection())
  {
    removeFormulaAction->setDisabled(true);
    exportFormulaAction->setDisabled(true);
  }
  else
  {
    removeFormulaAction->setEnabled(true);
    exportFormulaAction->setEnabled(true);
    if (!groups.empty())
      groupMenu->setEnabled(true);
  }
  
  menu.exec(event->globalPos());
}

void TableViewAll::buildActions()
{
  addFormulaAction = new QAction(tr("Add Formula"), this);
  addFormulaAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_N));
  addFormulaAction->setShortcutContext(Qt::WidgetShortcut);
  this->addAction(addFormulaAction);
  connect (addFormulaAction, SIGNAL(triggered()), this, SLOT(addDefaultRowSlot()));
  
  removeFormulaAction = new QAction(tr("Remove Formula"), this);
  removeFormulaAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_D));
  removeFormulaAction->setShortcutContext(Qt::WidgetShortcut);
  this->addAction(removeFormulaAction);
  connect(removeFormulaAction, SIGNAL(triggered()), this, SLOT(removeFormulaSlot()));
  
  exportFormulaAction = new QAction(tr("Export Formula"), this);
  exportFormulaAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E));
  exportFormulaAction->setShortcutContext(Qt::WidgetShortcut);
  this->addAction(exportFormulaAction);
  connect(exportFormulaAction, SIGNAL(triggered()), this, SLOT(exportFormulaSlot()));
  
  importFormulaAction = new QAction(tr("Import Formula"), this);
  importFormulaAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_I));
  importFormulaAction->setShortcutContext(Qt::WidgetShortcut);
  this->addAction(importFormulaAction);
  connect(importFormulaAction, SIGNAL(triggered()), this, SLOT(importFormulaSlot()));
  
  addGroupAction = new QAction(tr("Add Group"), this);
  addGroupAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_G));
  addGroupAction->setShortcutContext(Qt::WidgetShortcut);
  this->addAction(addGroupAction);
  connect(addGroupAction, SIGNAL(triggered()), this, SIGNAL(addGroupSignal()));
}


TableViewGroup::TableViewGroup(QWidget* parentIn): QTableView(parentIn)
{
  buildActions();
  this->verticalHeader()->setVisible(false);
}

void TableViewGroup::addDefaultRowSlot()
{
  QModelIndex proxyIndex = static_cast<GroupProxyModel*>(this->model())->addDefaultRow();
  
  this->selectionModel()->select(proxyIndex, QItemSelectionModel::ClearAndSelect);
  this->setCurrentIndex(proxyIndex);
  QMetaObject::invokeMethod(this, "edit", Qt::QueuedConnection, Q_ARG(QModelIndex, proxyIndex));
}

void TableViewGroup::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    GroupProxyModel *model = dynamic_cast<GroupProxyModel *>(this->model());
    assert(model);
    model->refreshSlot();
}

void TableViewGroup::contextMenuEvent(QContextMenuEvent* event)
{
  QMenu menu;
  
  menu.addAction(addFormulaAction);
  menu.addAction(removeFormulaAction);
  menu.addAction(removeFromGroupAction);
  
  menu.addSeparator();
  
  menu.addAction(exportFormulaAction);
  menu.addAction(importFormulaAction);
  
  menu.addSeparator();
  
  menu.addAction(renameGroupAction);
  menu.addAction(removeGroupAction);
  
  if(!this->selectionModel()->hasSelection())
  {
    removeFormulaAction->setDisabled(true);
    removeFromGroupAction->setDisabled(true);
    exportFormulaAction->setDisabled(true);
  }
  else
  {
    removeFormulaAction->setEnabled(true);
    removeFromGroupAction->setEnabled(true);
    exportFormulaAction->setEnabled(true);
  }
  
  menu.exec(event->globalPos());
}

void TableViewGroup::removeFormulaSlot()
{
  QSortFilterProxyModel *pModel = dynamic_cast<QSortFilterProxyModel *>(this->model());
  assert(pModel);
  TableModel *myModel = dynamic_cast<TableModel *>(pModel->sourceModel());
  assert(myModel);
  
  QModelIndexList indexes = this->selectedIndexes();
  QModelIndexList sourceIndexes;
  for (QModelIndexList::iterator it = indexes.begin(); it != indexes.end(); ++it)
    sourceIndexes.append(pModel->mapToSource(*it));
  myModel->removeFormula(sourceIndexes);
}

void TableViewGroup::removeFromGroupSlot()
{
  GroupProxyModel *pModel = dynamic_cast<GroupProxyModel *>(this->model());
  assert(pModel);
  pModel->removeFromGroup(this->selectedIndexes());
}

void TableViewGroup::renameGroupSlot()
{
  GroupProxyModel *pModel = dynamic_cast<GroupProxyModel *>(this->model());
  assert(pModel);
  
  QString oldName = QString::fromStdString(pModel->getGroupName());
  
  bool ok;
  QString newName = QInputDialog::getText(this, tr("Enter New Name"), tr("Name:"), QLineEdit::Normal, oldName, &ok);
  if (!ok || newName.isEmpty() || (newName == oldName))
    return;
  
  if (pModel->renameGroup(newName.toStdString()))
    Q_EMIT groupRenamedSignal(this, newName);
  else
    QMessageBox::critical(this, tr("Error:"), tr("Name Already Exists"));
}

void TableViewGroup::removeGroupSlot()
{
  GroupProxyModel *pModel = dynamic_cast<GroupProxyModel *>(this->model());
  assert(pModel);
  
  pModel->removeGroup();
  Q_EMIT(groupRemovedSignal(this));
}

void TableViewGroup::exportFormulaSlot()
{
  QSortFilterProxyModel *pModel = dynamic_cast<QSortFilterProxyModel *>(this->model());
  assert(pModel);
  TableModel *myModel = dynamic_cast<TableModel *>(pModel->sourceModel());
  assert(myModel);
  
  QString filePath = QFileDialog::getSaveFileName(this, tr("Save File Name"), defaultDirectory, tr("Expressions (*.exp)"));
  if (filePath.isEmpty())
    return;
  if (!filePath.endsWith(".exp"))
    filePath += ".exp";
  QFileInfo path(filePath);
  defaultDirectory = path.absolutePath();
  
  QModelIndexList indexes = this->selectedIndexes();
  QModelIndexList sourceIndexes;
  for (QModelIndexList::iterator it = indexes.begin(); it != indexes.end(); ++it)
    sourceIndexes.append(pModel->mapToSource(*it));
  
  std::ofstream fileStream;
  fileStream.open(filePath.toStdString().c_str());
  if (!fileStream.is_open())
  {
    QMessageBox::critical(this, tr("Error:"), tr("Couldn't Open File"));
    return;
  }
  myModel->exportExpressions(sourceIndexes, fileStream);
  fileStream.close();
}

void TableViewGroup::importFormulaSlot()
{
  GroupProxyModel *pModel = dynamic_cast<GroupProxyModel *>(this->model());
  assert(pModel);
  
  QString filePath = QFileDialog::getOpenFileName(this, tr("Open File Name"), defaultDirectory, tr("Expressions (*.exp)"));
  if (filePath.isEmpty())
    return;
  QFileInfo path(filePath);
  defaultDirectory = path.absolutePath();
  
  std::ifstream fileStream;
  fileStream.open(filePath.toStdString().c_str());
  if (!fileStream.is_open())
    return;
  
  pModel->importExpressions(fileStream);
}

void TableViewGroup::buildActions()
{
  addFormulaAction = new QAction(tr("Add Formula"), this);
  addFormulaAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_N));
  addFormulaAction->setShortcutContext(Qt::WidgetShortcut);
  this->addAction(addFormulaAction);
  connect (addFormulaAction, SIGNAL(triggered()), this, SLOT(addDefaultRowSlot()));
  
  removeFormulaAction = new QAction(tr("Remove Formula"), this);
  removeFormulaAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_D));
  removeFormulaAction->setShortcutContext(Qt::WidgetShortcut);
  this->addAction(removeFormulaAction);
  connect(removeFormulaAction, SIGNAL(triggered()), this, SLOT(removeFormulaSlot()));
  
  removeFromGroupAction = new QAction(tr("Remove From Group"), this);
  removeFromGroupAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_R));
  removeFromGroupAction->setShortcutContext(Qt::WidgetShortcut);
  this->addAction(removeFromGroupAction);
  connect(removeFromGroupAction, SIGNAL(triggered()), this, SLOT(removeFromGroupSlot()));
  
  exportFormulaAction = new QAction(tr("Export Formula"), this);
  exportFormulaAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E));
  exportFormulaAction->setShortcutContext(Qt::WidgetShortcut);
  this->addAction(exportFormulaAction);
  connect(exportFormulaAction, SIGNAL(triggered()), this, SLOT(exportFormulaSlot()));
  
  importFormulaAction = new QAction(tr("Import Formula"), this);
  importFormulaAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_I));
  importFormulaAction->setShortcutContext(Qt::WidgetShortcut);
  this->addAction(importFormulaAction);
  connect(importFormulaAction, SIGNAL(triggered()), this, SLOT(importFormulaSlot()));
  
  renameGroupAction = new QAction(tr("Rename Group"), this);
  renameGroupAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_M));
  renameGroupAction->setShortcutContext(Qt::WidgetShortcut);
  this->addAction(renameGroupAction);
  connect(renameGroupAction, SIGNAL(triggered()), this, SLOT(renameGroupSlot()));
  
  removeGroupAction = new QAction(tr("Remove Group"), this);
  removeGroupAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_K));
  removeGroupAction->setShortcutContext(Qt::WidgetShortcut);
  this->addAction(removeGroupAction);
  connect(removeGroupAction, SIGNAL(triggered()), this, SLOT(removeGroupSlot()));
}

ExpressionDelegate::ExpressionDelegate(QObject *parent): QStyledItemDelegate(parent)
{

}

QWidget* ExpressionDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
  LineEdit *lineEdit = new LineEdit(parent);
  return lineEdit;
}

void ExpressionDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
  LineEdit *lineEdit = dynamic_cast<LineEdit *>(editor);
  assert(lineEdit);
  const QSortFilterProxyModel *proxyModel = dynamic_cast<const QSortFilterProxyModel *>(index.model());
  assert(proxyModel);
  const TableModel *myModel = dynamic_cast<const TableModel *>(proxyModel->sourceModel());
  assert(myModel);
  if (myModel->lastFailedText.isEmpty())
    lineEdit->setText(index.model()->data(index, Qt::EditRole).toString());
  else
  {
    int position = myModel->lastFailedPosition;
    lineEdit->setText(myModel->lastFailedText);
    QMetaObject::invokeMethod(lineEdit, "setSelectionSlot", Qt::QueuedConnection,
                              Q_ARG(int, position), Q_ARG(int, myModel->lastFailedText.size() - position));
  }
  
  /*Hack! I don't know what else to do.
   * 
   * inside ExpressionDelegate::setModelData:
   * When the model setData fails, it (the model) caches the failing text(expression)
   * and the position of the failure and the model is returned to it's previous state.
   * We then invoke another edit.
   * 
   * inside ExpressionDelegate::setEditorData (here):
   * if the model has failing text we apply it to the edit box. This is so the user
   * doesn't loose a long expression because of a simple typo.
   * 
   * The problem arises that we can't clear this failing text when the user rejects the editing (esc key).
   * So then the editor keeps getting filled with the last failing text. Even in other rows.
   * 
   * I can't cache anything inside the delegate because all the overrides are constant.
   * Caching the data in model is only working because here we cast away the model constantness.
   */
  
  TableModel *hackModel = const_cast<TableModel *>(myModel);
  assert(hackModel);
  hackModel->lastFailedMessage.clear();
  hackModel->lastFailedPosition = 0;
  hackModel->lastFailedText.clear();
}

void ExpressionDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
  editor->setGeometry(option.rect);
}

void ExpressionDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
  QLineEdit *lineEdit = static_cast<QLineEdit *>(editor);
  if (!model->setData(index, lineEdit->text(), Qt::EditRole))
  {
    //view must be used as parent when constructing the delegate.
    QAbstractItemView *view = dynamic_cast<QAbstractItemView *>(this->parent());
    assert(view);
    QMessageBox::critical(view, tr("Error:"), model->data(index, (Qt::UserRole+1)).toString());
    QMetaObject::invokeMethod(view, "edit", Qt::QueuedConnection, Q_ARG(QModelIndex, index));
  }
}

NameDelegate::NameDelegate(QObject* parent): QStyledItemDelegate(parent)
{

}

void NameDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
  QLineEdit *lineEdit = static_cast<QLineEdit *>(editor);
  if (!model->setData(index, lineEdit->text(), Qt::EditRole))
  {
    //view must be used as parent when constructing the delegate.
    QAbstractItemView *view = dynamic_cast<QAbstractItemView *>(this->parent());
    assert(view);
    QMessageBox::critical(view, tr("Error:"), model->data(index, (Qt::UserRole+1)).toString());
    QMetaObject::invokeMethod(view, "edit", Qt::QueuedConnection, Q_ARG(QModelIndex, index));
  }
}

LineEdit::LineEdit(QWidget* parent): QLineEdit(parent)
{

}

void LineEdit::setSelectionSlot(const int& start, const int& length)
{
  this->setSelection(start, length);
}

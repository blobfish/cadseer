/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef DLG_BLEND_H
#define DLG_BLEND_H

#include <memory>
#include <set>

#include <boost/uuid/uuid.hpp>

#include <QDialog>
#include <QListWidgetItem>
#include <QStackedWidget>
#include <QStyledItemDelegate>

#include <osg/Vec3d>

class QCloseEvent;
class QListWidget;
class QTableWidget;
class QTableWidgetItem;
class QItemSelection;

namespace ftr{class Base; class Blend; class Pick;}
namespace msg{class Message; class Observer;}
namespace slc{class Message;}

namespace dlg
{
  class ExpressionEdit;
  
  struct BlendEntry
  {
    BlendEntry();
    BlendEntry(const slc::Message &);
    boost::uuids::uuid pickId; //!< the pick id.
    QString typeString; //!< the type. i.e. 'Edge' or 'Face'
    double radius; //!< this member not used for constant item.
    boost::uuids::uuid expressionLinkId; //!< nil id means no link. Not used for constant item.
    std::vector<boost::uuids::uuid> highlightIds;//!< entities to highlight. possibly includes pick id
    osg::Vec3d pointLocation;
  };
  
  class ConstantItem : public QListWidgetItem
  {
  public:
    static const int itemType = QListWidgetItem::UserType;
    ConstantItem(QListWidget *parent);
    std::vector<BlendEntry> picks;
    double radius;
    boost::uuids::uuid ftrId; //!< nil id means it has been created during dialog edit run.
    boost::uuids::uuid expressionLinkId; //!< nil id means no link.
  };
  
  class VariableItem : public QListWidgetItem
  {
  public:
    static const int itemType = QListWidgetItem::UserType + 1;
    VariableItem(QListWidget *parent);
    BlendEntry pick; //!< only 1 pick allowed per variable.
    std::vector<BlendEntry> constraints;
    boost::uuids::uuid ftrId; //!< nil id means it has been created during dialog edit run.
  };
  
  class Blend : public QDialog
  {
    Q_OBJECT
  public:
    Blend(QWidget*); //!< create blend dialog
    Blend(ftr::Blend*, QWidget*); //!< edit blend dialog
    virtual ~Blend() override;
    
  public Q_SLOTS:
    virtual void reject() override;
    virtual void accept() override;
    
  private:
    std::shared_ptr<ftr::Blend> blendSmart;
    ftr::Blend *blend = nullptr;
    const ftr::Base *blendParent = nullptr;
    std::unique_ptr<msg::Observer> observer;
    QListWidget *blendList;
    QStackedWidget *stackedWidget;
    QTableWidget *constantTableWidget;
    QTableWidget *variableTableWidget;
    dlg::ExpressionEdit *constantRadiusEdit;
    bool isAccepted = false;
    bool isEditDialog = false;
    bool overlayWasOn = false; //!< restore overlay state upon edit finish.
    std::set<boost::uuids::uuid> runningIds;
    std::vector<boost::uuids::uuid> leafChildren; //!< edit mode rewinds to blendParent. this restores state.
    
    void init();
    void buildGui();
    void fillInConstant(const ConstantItem &);
    void fillInVariable(const VariableItem &);
    void addToSelection(const boost::uuids::uuid&); //!< parent feature shape id.
    QTableWidgetItem* addVariableTableItem(const QString &, const QString&, const boost::uuids::uuid&);
    void updateBlendFeature();
    
    ftr::Pick convert(const BlendEntry&);
    BlendEntry convert(const ftr::Pick&);
    
    void setupDispatcher();
    void selectionAdditionDispatched(const msg::Message&);
    
    void finishDialog();
    
  private Q_SLOTS:
    void addConstantBlendSlot();
    void addVariableBlendSlot();
    void removeBlendSlot();
    
    void blendListCurrentItemChangedSlot(const QItemSelection&, const QItemSelection&);
    
    void constantRadiusEditingFinishedSlot();
    void constantRadiusEditedSlot(const QString &);
    void constantTableSelectionChangedSlot(const QItemSelection&, const QItemSelection&);
    void constantTableRemoveSlot();
    void requestConstantLinkSlot(const QString&);
    void requestConstantUnlinkSlot();
    
    void variableTableSelectionChangedSlot(const QItemSelection&, const QItemSelection&);
    void variableTableItemChangedSlot(QTableWidgetItem *item);
    void variableTableRemoveSlot();
    void requestVariableLinkSlot(QTableWidgetItem*, const QString&);
    void requestVariableUnlinkSlot();
    
    void selectFirstBlendSlot(); //!< used with timer for delayed initial selection.
  };
  
  //! a filter to do drag and drop for expression edit.
  class VariableDropFilter : public QObject
  {
    Q_OBJECT
  public:
    explicit VariableDropFilter(QObject *parent) : QObject(parent){}
    
  protected:
    virtual bool eventFilter(QObject *obj, QEvent *event) override;
    
  Q_SIGNALS:
    void requestLinkSignal(QTableWidgetItem*, const QString&);
  };
  
  //! @brief Delegate for variable table widget.
  class VariableDelegate : public QStyledItemDelegate
  {
    Q_OBJECT
  public:
    //! Parent must be the table view.
    explicit VariableDelegate(QObject *parent);
    //! Creates the editor.
    virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    //! Fill editor with text to edit.
    virtual void setEditorData(QWidget* editor, const QModelIndex& index) const;
    //! Match editor to cell size.
    virtual void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    //! Set the model data or re-edit upon failure.
    virtual void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const;
    
  private Q_SLOTS:
    void textEditedSlot(const QString &);
    void requestUnlinkSlot();
    
  Q_SIGNALS:
    void requestUnlinkSignal();
    
  private:
    mutable dlg::ExpressionEdit *eEditor = nullptr;
    mutable bool isExpressionLinked = false;
  };
}

#endif // DLG_BLEND_H

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
  struct BlendEntry
  {
    BlendEntry();
    BlendEntry(const slc::Message &);
    boost::uuids::uuid pickId; //!< the pick id.
    QString typeString; //!< the type. i.e. 'Edge' or 'Face'
    double radius; //!< this member not used for constant item.
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
  };
  
  class VariableItem : public QListWidgetItem
  {
  public:
    static const int itemType = QListWidgetItem::UserType + 1;
    VariableItem(QListWidget *parent);
    
    BlendEntry pick; //!< only 1 pick allowed per variable.
    std::vector<BlendEntry> constraints;
  };
  
  
  class Blend : public QDialog
  {
    Q_OBJECT
  public:
    Blend(QWidget*); //!< create blend dialog
    Blend(const boost::uuids::uuid&, QWidget*); //!< edit blend dialog
    virtual ~Blend() override;
    
  protected:
    virtual void closeEvent (QCloseEvent*) override;
    
  private:
    std::shared_ptr<ftr::Blend> blendSmart;
    ftr::Blend *blend = nullptr;
    ftr::Base *blendParent = nullptr;
    std::unique_ptr<msg::Observer> observer;
    QListWidget *blendList;
    QStackedWidget *stackedWidget;
    QTableWidget *constantTableWidget;
    QTableWidget *variableTableWidget;
    QLineEdit *constantRadiusEdit;
    bool isAccepted = false;
    bool isEditDialog = false;
    std::set<boost::uuids::uuid> runningIds;
    
    void init();
    void buildGui();
    void fillInConstant(const ConstantItem &);
    void fillInVariable(const VariableItem &);
    void addToSelection(const boost::uuids::uuid&); //!< parent feature shape id.
    void addVariableTableItem(double, const QString&, const boost::uuids::uuid&);
    
    ftr::Pick convert(const BlendEntry&);
    
    void setupDispatcher();
    void selectionAdditionDispatched(const msg::Message&);
    
  private Q_SLOTS:
    void addConstantBlendSlot();
    void addVariableBlendSlot();
    void removeBlendSlot();
    
    void myAcceptedSlot();
    void myRejectedSlot();
    
    void blendListCurrentItemChangedSlot(const QItemSelection&, const QItemSelection&);
    void constantRadiusEditingFinishedSlot();
    void constantTableSelectionChangedSlot(const QItemSelection&, const QItemSelection&);
    void constantTableRemoveSlot();
    void variableTableSelectionChangedSlot(const QItemSelection&, const QItemSelection&);
    void variableTableItemChangedSlot(QTableWidgetItem *item);
    void variableTableRemoveSlot();
  };
}

#endif // DLG_BLEND_H

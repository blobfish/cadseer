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

#include <QLabel>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QStackedWidget>
#include <QComboBox>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QCloseEvent>
#include <QAction>
#include <QSettings>
#include <QTimer>
#include <QDragMoveEvent>
#include <QMessageBox>
#include <QDebug>

#include <boost/signals2/shared_connection_block.hpp>

#include <TopoDS.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>

#include <tools/idtools.h>
#include <application/application.h>
#include <application/mainwindow.h>
#include <application/splitterdecorated.h>
#include <project/project.h>
#include <expressions/manager.h>
#include <expressions/stringtranslator.h>
#include <message/message.h>
#include <message/observer.h>
#include <selection/message.h>
#include <feature/seershape.h>
#include <feature/blend.h>
#include <dialogs/widgetgeometry.h>
#include <dialogs/expressionedit.h>
#include <dialogs/blend.h>



//this is a complete mess!



using boost::uuids::uuid;

using namespace dlg;

BlendEntry::BlendEntry()
{
  pickId = gu::createNilId();
  typeString = QString::fromStdString(slc::getNameOfType(slc::Type::None));
  radius = 1.0; //some kind of default
  expressionLinkId = gu::createNilId();
}

BlendEntry::BlendEntry(const slc::Message &sMessageIn)
{
  pickId = sMessageIn.shapeId;
  typeString = QString::fromStdString(slc::getNameOfType(sMessageIn.type));
  pointLocation = sMessageIn.pointLocation;
  radius = 1.0; //some kind of default
  expressionLinkId = gu::createNilId();
}

ConstantItem::ConstantItem(QListWidget *parent) :
  QListWidgetItem(QObject::tr("Constant"), parent, ConstantItem::itemType)
{
  radius = 1.0; //somekind of default.
  ftrId = gu::createNilId();
  expressionLinkId = gu::createNilId();
}

VariableItem::VariableItem(QListWidget *parent) :
  QListWidgetItem(QObject::tr("Variable"), parent, VariableItem::itemType)
{
  pick.pickId = gu::createNilId();
  ftrId = gu::createNilId();
}

Blend::Blend(QWidget *parent) : QDialog(parent)
{
  init();
  isEditDialog = false;
  
  blendSmart = std::make_shared<ftr::Blend>();
  assert(blendSmart);
  blend = blendSmart.get();
  assert(blend);
  //parent will be assigned upon first edge pick.
  
  //add a default blend.
  addConstantBlendSlot();
}

Blend::Blend(ftr::Blend *editBlendIn, QWidget *parent) : QDialog(parent), blend(editBlendIn)
{
  assert(blend);
  
  init();
  isEditDialog = true;
  
  //smart pointer to remain inValid in edit 'mode'.
  prj::Project *project = static_cast<app::Application*>(qApp)->getProject();
  assert(project);
  
  //what if the established feature doesn't have parent??
  ftr::EditMap editMap = project->getParentMap(blend->getId());
  assert(editMap.size() == 1);
  auto it = editMap.find(ftr::InputTypes::target);
  assert(it != editMap.end());
  blendParent = it->second;
  
  for (const auto &simpleBlend : blend->getSimpleBlends())
  {
    ConstantItem *cItem = new ConstantItem(blendList);
    cItem->ftrId = simpleBlend.id;
    cItem->radius = simpleBlend.radius->getValue();
    if (simpleBlend.radius->isConstant())
    {
      cItem->expressionLinkId = gu::createNilId();
    }
    else
    {
      const expr::Manager &eManager = static_cast<app::Application *>(qApp)->getProject()->getManager();
      assert(eManager.hasParameterLink(simpleBlend.radius->getId()));
      cItem->expressionLinkId = eManager.getFormulaLink(simpleBlend.radius->getId());
    }
    for (const auto &pick : simpleBlend.picks)
      cItem->picks.push_back(convert(pick));
  }
  
  for (const auto &variableBlend : blend->getVariableBlends())
  {
    VariableItem *vItem = new VariableItem(blendList);
    vItem->pick = convert(variableBlend.pick);
    vItem->ftrId = variableBlend.id;
    for (const auto &entry : variableBlend.entries)
    {
      BlendEntry bEntry;
      bEntry.pickId = entry.id;
      bEntry.typeString = tr("Vertex"); //only vertices for now.
      bEntry.radius = entry.radius->getValue();
      bEntry.highlightIds.push_back(entry.id);
      bEntry.pointLocation = gu::toOsg(TopoDS::Vertex(blendParent->getSeerShape().getOCCTShape(entry.id)));
      if (entry.radius->isConstant())
      {
        bEntry.expressionLinkId = gu::createNilId();
      }
      else
      {
        const expr::Manager &eManager = static_cast<app::Application *>(qApp)->getProject()->getManager();
        assert(eManager.hasParameterLink(entry.radius->getId()));
        bEntry.expressionLinkId = eManager.getFormulaLink(entry.radius->getId());
      }
      vItem->constraints.push_back(bEntry);
    }
  }
  
  leafChildren = project->getLeafChildren(blendParent->getId());
  project->setCurrentLeaf(blendParent->getId());
  
  overlayWasOn = blend->isVisibleOverlay();
  blend->hideOverlay();
  
  QTimer::singleShot(0, this, SLOT(selectFirstBlendSlot()));
}

void Blend::selectFirstBlendSlot()
{
  if (!(blendList->count() > 0))
    return;
  QModelIndex index = blendList->model()->index(0,0);
  assert(index.isValid());
  blendList->selectionModel()->select(index, QItemSelectionModel::SelectCurrent);
}

void Blend::init()
{
  observer = std::move(std::unique_ptr<msg::Observer>(new msg::Observer()));
  observer->name = "dlg::Blend";
  setupDispatcher();
  
  buildGui();
  
  WidgetGeometry *filter = new WidgetGeometry(this, "dlg::Blend");
  this->installEventFilter(filter);
  
  QSettings &settings = static_cast<app::Application*>(qApp)->getUserSettings();
  settings.beginGroup("dlg::Blend");
  settings.beginGroup("ConstantTable");
  constantTableWidget->horizontalHeader()->restoreState(settings.value("header").toByteArray());
  settings.endGroup();
  settings.beginGroup("VariableTable");
  variableTableWidget->horizontalHeader()->restoreState(settings.value("header").toByteArray());
  settings.endGroup();
  settings.endGroup();
  
  observer->out(msg::buildSelectionMask(~slc::All));
}

Blend::~Blend()
{
  QSettings &settings = static_cast<app::Application*>(qApp)->getUserSettings();
  settings.beginGroup("dlg::Blend");
  settings.beginGroup("ConstantTable");
  settings.setValue("header", constantTableWidget->horizontalHeader()->saveState());
  settings.endGroup();
  settings.beginGroup("VariableTable");
  settings.setValue("header", variableTableWidget->horizontalHeader()->saveState());
  settings.endGroup();
  settings.endGroup();
}

void Blend::reject()
{
  isAccepted = false;
  finishDialog();
  QDialog::reject();
}

void Blend::accept()
{
  isAccepted = true;
  finishDialog();
  QDialog::accept();
}

void Blend::finishDialog()
{
  prj::Project *project = static_cast<app::Application *>(qApp)->getProject();
  
  //blendSmart and blendParent might be invalid dependent on state during close.
  if (!isEditDialog && isAccepted && blendSmart && blendParent) // = creation dialog that got accepted.
  {
    assert(blend);
    assert(blendParent);
    assert(blendParent->hasSeerShape());
    
    updateBlendFeature();
    
    project->addFeature(blendSmart);
    project->connect(blendParent->getId(), blendSmart->getId(), ftr::InputTypes::target);
    
    blendParent->hide3D();
    blendParent->hideOverlay();
    blend->setColor(blendParent->getColor());
  }
  
  if (isEditDialog && isAccepted)
  {
    //blendsmart is not used during edit so it is invalid and not needed.
    assert(blend);
    assert(blendParent);
    
    //build a set of blendItem ftr ids for checking for removed definitions.
    std::set<uuid> blendItemFtrIds;
    
    for (int index = 0; index < blendList->count(); ++index)
    {
      QListWidgetItem *item = blendList->item(index);
      if (item->type() == ConstantItem::itemType)
      {
        ConstantItem *cItem = dynamic_cast<ConstantItem*>(item);
        assert(cItem);
        blendItemFtrIds.insert(cItem->ftrId);
      }
      else if(item->type() == VariableItem::itemType)
      {
        VariableItem *vItem = dynamic_cast<VariableItem*>(item);
        assert(vItem);
        blendItemFtrIds.insert(vItem->ftrId);
      }
      else
        assert(0);//unrecognized item type
    }
    
    //remove blend definitions from feature that are not present in blend list items set.
    for (auto it = blend->getSimpleBlends().begin(); it != blend->getSimpleBlends().end();)
    {
      assert(blendItemFtrIds.count(it->id) < 2); //should be zero or 1.
      
      if (blendItemFtrIds.count(it->id) == 0)
        it = blend->getSimpleBlends().erase(it);
      else
        it++;
    }
    
    for (auto it = blend->getVariableBlends().begin(); it != blend->getVariableBlends().end();)
    {
      assert(blendItemFtrIds.count(it->id) < 2); //should be zero or 1.
      
      if (blendItemFtrIds.count(it->id) == 0)
        it = blend->getVariableBlends().erase(it);
      else
        it++;
    }
    
    //from here updateBlendFeature needs to either edit or add blend definitions.
    updateBlendFeature();
    blend->setModelDirty();
  }
  
  if (isEditDialog)
  {
    for (const auto &id : leafChildren)
      project->setCurrentLeaf(id);
    
    if (overlayWasOn)
      blend->showOverlay();
  }
  
  observer->out(msg::Mask(msg::Request | msg::Command | msg::Done));
}

void Blend::updateBlendFeature()
{
  expr::Manager &eManager = 
    static_cast<app::Application *>(qApp)->getProject()->getManager();
  
  for (int index = 0; index < blendList->count(); ++index)
  {
    QListWidgetItem *item = blendList->item(index);
    if (item->type() == ConstantItem::itemType)
    {
      ConstantItem *cItem = dynamic_cast<ConstantItem*>(item);
      assert(cItem);
      
      if (cItem->ftrId.is_nil()) //this is something new even in edit mode
      {
        ftr::SimpleBlend sBlend;
        for (const auto &entry : cItem->picks)
        {
          ftr::Pick fPick = convert(entry);
          sBlend.picks.push_back(fPick);
        }
        auto radiusParameter = ftr::Blend::buildRadiusParameter();
        radiusParameter->setValue(cItem->radius);
        if (!cItem->expressionLinkId.is_nil())
        {
          assert(eManager.hasFormula(cItem->expressionLinkId));
          eManager.addLink(radiusParameter.get(), cItem->expressionLinkId);
        }
        sBlend.radius = radiusParameter;
        
        blend->addSimpleBlend(sBlend);
      }
      else //this is not new and has been edited
      {
        std::vector<ftr::SimpleBlend>::iterator it;
        for (it = blend->getSimpleBlends().begin(); it != blend->getSimpleBlends().end(); ++it)
        {
          if (it->id == cItem->ftrId)
            break;
        }
        assert(it != blend->getSimpleBlends().end()); //any non present ftr id should have nilId in item.
        it->radius->setValue(cItem->radius);
        //if it is already linked just remove it by default.
        //we will add it back if needed
        if (eManager.hasParameterLink(it->radius->getId()))
          eManager.removeParameterLink(it->radius->getId());
        if (!cItem->expressionLinkId.is_nil())
        {
          assert(eManager.hasFormula(cItem->expressionLinkId));
          eManager.addLink(it->radius.get(), cItem->expressionLinkId);
        }
        it->picks.clear();
        for (const auto &entry : cItem->picks)
        {
          ftr::Pick fPick = convert(entry);
          it->picks.push_back(fPick);
        }
      }
    }
    else if(item->type() == VariableItem::itemType)
    {
      VariableItem *vItem = dynamic_cast<VariableItem*>(item);
      assert(vItem);
      
      ftr::Pick fPick = convert(vItem->pick);
      if (vItem->ftrId.is_nil()) //new item even in edit mode.
      {
        ftr::VariableBlend blendCue = ftr::Blend::buildDefaultVariable(blendParent->getSeerShape(), fPick);
        
        for (const auto &constraint : vItem->constraints)
        {
          //buildDefaultVariable builds 2 parameters. one at each end of the spine.
          //we search already created parameters and match them against the vItem constraints
          uuid constraintId = constraint.pickId;
          auto it = blendCue.entries.begin();
          for (; it != blendCue.entries.end(); ++it)
          {
            if (it->id != constraintId)
              continue;
            it->radius->setValue(constraint.radius);
            if (eManager.hasParameterLink(it->radius->getId()))
              eManager.removeParameterLink(it->radius->getId());
            if (!constraint.expressionLinkId.is_nil())
            {
              assert(eManager.hasFormula(constraint.expressionLinkId));
              eManager.addLink(it->radius.get(), constraint.expressionLinkId);
            }
            break;
          }
          if(it == blendCue.entries.end())
          {
            //not a match so build another one.
            ftr::VariableEntry entry;
            entry.id = constraint.pickId;
            entry.radius = ftr::Blend::buildRadiusParameter();
            entry.radius->setValue(constraint.radius);
            if (!constraint.expressionLinkId.is_nil())
            {
              assert(eManager.hasFormula(constraint.expressionLinkId));
              eManager.addLink(it->radius.get(), constraint.expressionLinkId);
            }
            entry.position = ftr::Blend::buildPositionParameter();
            blendCue.entries.push_back(entry);
          }
        }
        
        blend->addVariableBlend(blendCue);
      }
      else //editing variable blend definition.
      {
        std::vector<ftr::VariableBlend> &ftrVBlends = blend->getVariableBlends();
        std::vector<ftr::VariableBlend>::iterator it;
        for (it = ftrVBlends.begin(); it != ftrVBlends.end(); ++it)
        {
          if (it->id == vItem->ftrId)
            break;
        }
        assert(it != ftrVBlends.end()); //any non present ftr id should have nilId in item.
        assert(it->pick.id == vItem->pick.pickId); //shouldn't be able to have a different pick.
        
        //build a set of all constraint picks. loop through feature constraints and
        //remove any not present in the set.
        std::set<uuid> itemConstraintIds;
        for (const auto &entry : vItem->constraints)
          itemConstraintIds.insert(entry.pickId);
        std::vector<ftr::VariableEntry>::iterator vIt;
        for (vIt = it->entries.begin(); vIt != it->entries.end();)
        {
          assert(itemConstraintIds.count(vIt->id) < 2); //should be zero or 1.
          if (itemConstraintIds.count(vIt->id) == 0)
          {
            assert(vIt->label->getParents().size() == 1);
            osg::Group *parent = vIt->label->getParent(0);
            parent->removeChild(vIt->label); //remove label from scene graph.
            vIt = it->entries.erase(vIt);
          }
          else
            vIt++;
        }
        
        for (const auto &itemEntry : vItem->constraints)
        {
          std::vector<ftr::VariableEntry>::iterator fEIt;
          for (fEIt = it->entries.begin(); fEIt != it->entries.end(); ++fEIt)
          {
            if (fEIt->id == itemEntry.pickId)
              break;
          }
          if (fEIt == it->entries.end())
          {
            ftr::VariableEntry entry;
            entry.id = itemEntry.pickId;
            entry.radius = ftr::Blend::buildRadiusParameter();
            entry.radius->setValue(itemEntry.radius);
            entry.radius->connectValue(boost::bind(&ftr::Blend::setModelDirty, blend));
            if (!itemEntry.expressionLinkId.is_nil())
            {
              assert(eManager.hasFormula(itemEntry.expressionLinkId));
              eManager.addLink(entry.radius.get(), itemEntry.expressionLinkId);
            }
            entry.position = ftr::Blend::buildPositionParameter();
            entry.position->connectValue(boost::bind(&ftr::Blend::setModelDirty, blend));
            entry.label = new lbr::PLabel(entry.radius.get());
            entry.label->valueHasChanged();
            blend->getOverlaySwitch()->addChild(entry.label.get());
            it->entries.push_back(entry);
          }
          else
          {
            fEIt->radius->setValue(itemEntry.radius);
            if (!itemEntry.expressionLinkId.is_nil())
            {
              assert(eManager.hasFormula(itemEntry.expressionLinkId));
              eManager.addLink(fEIt->radius.get(), itemEntry.expressionLinkId);
            }
          }
        }
        
      }
    }
    else
      assert(0);//unrecognized item type
  }
}

ftr::Pick Blend::convert(const BlendEntry &entryIn)
{
  assert(blendParent);
  assert(blendParent->hasSeerShape());
  
  ftr::Pick fPick;
  fPick.id = entryIn.pickId;
  fPick.shapeHistory = static_cast<app::Application*>(qApp)->getProject()->getShapeHistory().createDevolveHistory(fPick.id);
  const ftr::SeerShape &sShape = blendParent->getSeerShape();
  assert(sShape.hasShapeIdRecord(entryIn.pickId));
  const TopoDS_Shape &shape = sShape.getOCCTShape(entryIn.pickId);
  if (shape.ShapeType() == TopAbs_EDGE)
    fPick.setParameter(TopoDS::Edge(shape), entryIn.pointLocation);
  else if (shape.ShapeType() == TopAbs_FACE)
    fPick.setParameter(TopoDS::Face(shape), entryIn.pointLocation);
  else
    assert(0); // unrecognized occt shape type.
  
  return fPick;
}

BlendEntry Blend::convert(const ftr::Pick &pickIn)
{
  BlendEntry entry;
  entry.pickId = pickIn.id;
  const ftr::SeerShape &parentShape = blendParent->getSeerShape();
  assert(parentShape.hasShapeIdRecord(pickIn.id));
  const TopoDS_Shape &shape = parentShape.getOCCTShape(pickIn.id);
  BRepFilletAPI_MakeFillet blendMaker(parentShape.getRootOCCTShape());
  std::vector<uuid> contourIds;
  if (shape.ShapeType() == TopAbs_VERTEX)
  {
    entry.typeString = tr("Vertex");
    entry.pointLocation = gu::toOsg(TopoDS::Vertex(shape));
    runningIds.insert(pickIn.id);
  }
  else if (shape.ShapeType() == TopAbs_EDGE)
  {
    entry.typeString = tr("Edge");
    entry.pointLocation = pickIn.getPoint(TopoDS::Edge(shape));
    
    blendMaker.Add(TopoDS::Edge(shape));
    assert(blendMaker.NbContours() == 1);
    for (int index = 1; index <= blendMaker.NbEdges(1); ++index)
    {
      assert(parentShape.hasShapeIdRecord(blendMaker.Edge(1, index)));
      contourIds.push_back(parentShape.findShapeIdRecord(blendMaker.Edge(1, index)).id);
      runningIds.insert(contourIds.back());
    }
  }
  else if (shape.ShapeType() == TopAbs_FACE)
  {
    entry.typeString = tr("Face");
    entry.pointLocation = pickIn.getPoint(TopoDS::Face(shape));
  }
  else
    assert(0); // unrecognized occt shape type.
  
  entry.highlightIds = contourIds;
  return entry;
}

void Blend::addConstantBlendSlot()
{
  new ConstantItem(blendList);
  blendList->item(blendList->count() - 1)->setSelected(true);
}

void Blend::addVariableBlendSlot()
{
  new VariableItem(blendList);
  blendList->item(blendList->count() - 1)->setSelected(true);
}

void Blend::removeBlendSlot()
{
  constantTableWidget->clearSelection();
  variableTableWidget->clearSelection();
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  QList<QListWidgetItem*> selected = blendList->selectedItems();
  assert(selected.size() == 1); //we have single selection enabled on control.
  
  //need to update runningIds.
  QListWidgetItem *item = selected.front();
  if (item->type() == ConstantItem::itemType)
  {
    ConstantItem *cItem = dynamic_cast<ConstantItem*>(item);
    assert(cItem);
    for (const auto &pick : cItem->picks)
    {
      for (const auto &id : pick.highlightIds)
        runningIds.erase(id);
    }
  }
  else if (item->type() == VariableItem::itemType)
  {
    VariableItem *vItem = dynamic_cast<VariableItem*>(item);
    assert(vItem);
    for (const auto &id : vItem->pick.highlightIds)
      runningIds.erase(id);
    for (const auto &blendEntry : vItem->constraints)
    {
      for (const auto &id : blendEntry.highlightIds)
        runningIds.erase(id);
    }
  }
  else
    assert(0); //unrecognized item type.
  
  qDeleteAll(selected);
}

void Blend::blendListCurrentItemChangedSlot(const QItemSelection &current, const QItemSelection&)
{
  constantTableWidget->clearSelection();
  variableTableWidget->clearSelection();
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  if (current.indexes().isEmpty())
  {
    stackedWidget->hide();
    observer->outBlocked(msg::buildSelectionMask(~slc::All));
    return;
  }
  stackedWidget->show();
  
  //selection model set to single selection so only 1, the front.
  QListWidgetItem *currentItem = blendList->item(current.indexes().front().row());
  if (currentItem->type() == ConstantItem::itemType)
  {
    constantTableWidget->setRowCount(0);
    ConstantItem *cItem = dynamic_cast<ConstantItem*>(currentItem);
    assert(cItem);
    fillInConstant(*cItem);
    observer->outBlocked(msg::buildSelectionMask(~slc::All | slc::EdgesEnabled | slc::EdgesSelectable));
    stackedWidget->setCurrentIndex(0);
  }
  else if(currentItem->type() == VariableItem::itemType)
  {
    variableTableWidget->setRowCount(0);
    VariableItem *vItem = dynamic_cast<VariableItem*>(currentItem);
    assert(vItem);
    fillInVariable(*vItem);
    if (vItem->pick.pickId.is_nil())
    {
      observer->outBlocked(msg::buildSelectionMask(~slc::All | slc::EdgesEnabled | slc::EdgesSelectable));
    }
    else
    {
      observer->outBlocked(msg::buildSelectionMask
        (~slc::All | slc::PointsEnabled | slc::PointsSelectable |
        slc::EndPointsEnabled | slc::EndPointsSelectable));
    }
    //enable selection of edges if no edge picked, else end points.
    stackedWidget->setCurrentIndex(1);
  }
  else
    assert(0); //unrecognized item type.
}

void Blend::fillInConstant(const ConstantItem &itemIn)
{
  if (itemIn.expressionLinkId.is_nil())
  {
    constantRadiusEdit->lineEdit->setText(QString::number(itemIn.radius, 'f', 12));
    constantRadiusEdit->lineEdit->setReadOnly(false);
    constantRadiusEdit->lineEdit->selectAll();
    constantRadiusEdit->lineEdit->setFocus();
    constantRadiusEdit->trafficLabel->setTrafficGreenSlot();
  }
  else
  {
    const expr::Manager &eManager = static_cast<app::Application *>(qApp)->getProject()->getManager();
    assert(eManager.hasFormula(itemIn.expressionLinkId));
    constantRadiusEdit->lineEdit->setText
      (QString::fromStdString(eManager.getFormulaName(itemIn.expressionLinkId)));
    constantRadiusEdit->lineEdit->setReadOnly(true);
    constantRadiusEdit->trafficLabel->setLinkSlot();
  }
  for (const auto &entry : itemIn.picks)
  {
    QTableWidgetItem *work = nullptr;
    
    int row = constantTableWidget->rowCount();
    constantTableWidget->insertRow(row);
    
    work = new QTableWidgetItem(entry.typeString);
    work->setFlags(work->flags() & ~Qt::ItemIsEditable);
    constantTableWidget->setItem(row, 0, work);
    
    work = new QTableWidgetItem(QString::fromStdString(gu::idToString(entry.pickId)));
    work->setFlags(work->flags() & ~Qt::ItemIsEditable);
    constantTableWidget->setItem(row, 1, work);
    
    for (const auto &highlight : entry.highlightIds)
      addToSelection(highlight);
  }
}

void Blend::fillInVariable(const VariableItem &itemIn)
{
  for (const auto &constraint : itemIn.constraints)
  {
    if (constraint.expressionLinkId.is_nil())
    {
      QString radiusString = QString::number(constraint.radius, 'f', 12);
      addVariableTableItem(radiusString, constraint.typeString, constraint.pickId);
    }
    else
    {
      const expr::Manager &eManager = static_cast<app::Application *>(qApp)->getProject()->getManager();
      assert(eManager.hasFormula(constraint.expressionLinkId));
      QString radiusString = QString::fromStdString(eManager.getFormulaName(constraint.expressionLinkId));
      QTableWidgetItem *radiusItem = addVariableTableItem(radiusString, constraint.typeString, constraint.pickId);
      radiusItem->setData(Qt::UserRole, true); //used by delegate to check for linking.
    }
  }
  
  for (const auto &highlight : itemIn.pick.highlightIds)
    addToSelection(highlight);
}

void Blend::addToSelection(const boost::uuids::uuid &shapeIdIn)
{
  const ftr::SeerShape &parentShape = blendParent->getSeerShape();
  assert(parentShape.hasShapeIdRecord(shapeIdIn));
  slc::Type sType = slc::convert(parentShape.getOCCTShape(shapeIdIn).ShapeType());
  assert(sType == slc::Type::Edge || sType == slc::Type::Face);
  
  msg::Message freshMMessage(msg::Request | msg::Selection | msg::Add);
  slc::Message freshSMessage;
  freshSMessage.type = sType;
  freshSMessage.featureId = blendParent->getId();
  freshSMessage.featureType = blendParent->getType();
  freshSMessage.shapeId = shapeIdIn;
  freshMMessage.payload = freshSMessage;
  observer->outBlocked(freshMMessage);
}

void Blend::constantRadiusEditingFinishedSlot()
{
  QList<QListWidgetItem*> items = blendList->selectedItems();
  assert(items.size() == 1);
  assert(items.front()->type() == ConstantItem::itemType);
  ConstantItem *cItem = dynamic_cast<ConstantItem*>(items.front());
  assert(cItem);
  
  //parameter is linked to expression we shouldn't need to do anything.
  if (!cItem->expressionLinkId.is_nil())
    return;
  
  expr::Manager localManager;
  expr::StringTranslator translator(localManager);
  std::string formula("temp = ");
  formula += constantRadiusEdit->lineEdit->text().toStdString();
  if (translator.parseString(formula) == expr::StringTranslator::ParseSucceeded)
  {
    localManager.update();
    assert(localManager.getFormulaValueType(translator.getFormulaOutId()) == expr::ValueType::Scalar);
    double value = boost::get<double>(localManager.getFormulaValue(translator.getFormulaOutId()));
    if (!(value > 0.0))
      observer->out(msg::buildStatusMessage(QObject::tr("Need positive radei").toStdString()));
    else
      cItem->radius = value;
  }
  else
  {
    observer->out(msg::buildStatusMessage(QObject::tr("Parsing failed").toStdString()));
  }
  
  constantRadiusEdit->lineEdit->setText(QString::number(cItem->radius, 'f', 12));
  constantRadiusEdit->lineEdit->selectAll();
  constantRadiusEdit->trafficLabel->setTrafficGreenSlot();
}

void Blend::constantRadiusEditedSlot(const QString &textIn)
{
  constantRadiusEdit->trafficLabel->setTrafficYellowSlot();
  qApp->processEvents(); //need this or we never see yellow signal.
  
  expr::Manager localManager;
  expr::StringTranslator translator(localManager);
  std::string formula("temp = ");
  formula += textIn.toStdString();
  if (translator.parseString(formula) == expr::StringTranslator::ParseSucceeded)
  {
    localManager.update();
    constantRadiusEdit->trafficLabel->setTrafficGreenSlot();
    assert(localManager.getFormulaValueType(translator.getFormulaOutId()) == expr::ValueType::Scalar);
    double value = boost::get<double>(localManager.getFormulaValue(translator.getFormulaOutId()));
    constantRadiusEdit->goToolTipSlot(QString::number(value));
  }
  else
  {
    constantRadiusEdit->trafficLabel->setTrafficRedSlot();
    int position = translator.getFailedPosition() - 8; // 7 chars for 'temp = ' + 1
    constantRadiusEdit->goToolTipSlot(textIn.left(position) + "?");
  }
}

void Blend::constantTableSelectionChangedSlot(const QItemSelection &current, const QItemSelection&)
{
  if (current.indexes().isEmpty())
    return;
  
  //table is set to single selection.
  QModelIndex index = constantTableWidget->model()->index(current.indexes().front().row(), 1);
  assert(index.isValid());
  uuid pickId = gu::stringToId(constantTableWidget->model()->data(index).toString().toStdString());
  
  assert(blendList->selectedItems().size() == 1);
  ConstantItem *cItem = dynamic_cast<ConstantItem*>(blendList->selectedItems().front());
  assert(cItem);
  
  observer->outBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  for (const auto &pick : cItem->picks)
  {
    if (pick.pickId != pickId)
      continue;
    for (const auto &highlight : pick.highlightIds)
      addToSelection(highlight);
  }
}

void Blend::constantTableRemoveSlot()
{
  QModelIndexList selection = constantTableWidget->selectionModel()->selectedIndexes();
  if (selection.isEmpty())
  {
    observer->out(msg::buildStatusMessage("Nothing selected to remove"));
    return;
  }
  
  //single selection so only 1.
  QModelIndex index = constantTableWidget->model()->index(selection.front().row(), 1);
  uuid pickId = gu::stringToId(constantTableWidget->model()->data(index).toString().toStdString());
  constantTableWidget->removeRow(index.row());
  constantTableWidget->clearSelection();
  observer->outBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  ConstantItem *cItem = dynamic_cast<ConstantItem*>(blendList->selectedItems().front());
  assert(cItem);
  std::vector<BlendEntry>::iterator eraseMe = cItem->picks.end();
  for (auto it = cItem->picks.begin(); it < cItem->picks.end(); ++it)
  {
    if (it->pickId == pickId)
    {
      eraseMe = it;
      continue;
    }
    for (const auto &highlight : it->highlightIds)
      addToSelection(highlight);
  }
  assert(eraseMe != cItem->picks.end());
  
  //remove ids from runningIds.
  for (const auto &highlightId : eraseMe->highlightIds)
    runningIds.erase(highlightId);
  
  cItem->picks.erase(eraseMe);
}

void Blend::setupDispatcher()
{
  msg::Mask mask;
  
  mask = msg::Response | msg::Post | msg::Selection | msg::Add;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind
  (&Blend::selectionAdditionDispatched, this, _1)));
}

void Blend::selectionAdditionDispatched(const msg::Message &messageIn)
{
  //dont accept selection if nothing is selected in the blend list
  QList<QListWidgetItem *> items = blendList->selectedItems();
  if(items.isEmpty())
  {
    observer->out(msg::buildStatusMessage("Add and/or select blend from dialog"));
    observer->outBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return;
  }
  
  const slc::Message &sMessage = boost::get<slc::Message>(messageIn.payload);
  if (!blendParent)
  {
    prj::Project *project = static_cast<app::Application*>(qApp)->getProject();
    assert(project);
    blendParent = project->findFeature(sMessage.featureId);
    assert(blendParent);
    if (!blendParent->hasSeerShape())
    {
      observer->out(msg::buildStatusMessage("Invalid object to blend"));
      observer->outBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
      return;
    }
  }
  
  if (blendParent->getId() != sMessage.featureId)
  {
    observer->out(msg::buildStatusMessage("Can't tie blend feature to multiple bodies"));
    observer->outBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return;
  }
  
  //build a vector of all edges that will be blended and add to highlight.
  std::vector<uuid> contourIds;
  const ftr::SeerShape &parentShape = blendParent->getSeerShape();
  BRepFilletAPI_MakeFillet blendMaker(parentShape.getRootOCCTShape());
  if (sMessage.type == slc::Type::Edge)
  {
    blendMaker.Add(TopoDS::Edge(parentShape.getOCCTShape(sMessage.shapeId)));
    assert(blendMaker.NbContours() == 1);
    for (int index = 1; index <= blendMaker.NbEdges(1); ++index)
    {
      assert(parentShape.hasShapeIdRecord(blendMaker.Edge(1, index)));
      contourIds.push_back(parentShape.findShapeIdRecord(blendMaker.Edge(1, index)).id);
      if (runningIds.count(contourIds.back()) != 0)
      {
        auto block = observer->createBlocker();
        observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
        observer->out(msg::buildStatusMessage("Blend contour overlaps other contour"));
        return;
      }
    }
    for (const auto &contourId : contourIds)
    {
      runningIds.insert(contourId);
      addToSelection(contourId);
    }
  }
  
  //selection model set to single selection so only 1, the front.
  QListWidgetItem *currentItem = items.front();
  if (currentItem->type() == ConstantItem::itemType)
  {
    ConstantItem *cItem = dynamic_cast<ConstantItem*>(currentItem);
    assert(cItem);
    assert(sMessage.type == slc::Type::Edge);
    
    BlendEntry fresh(sMessage);
    fresh.highlightIds = contourIds;
    cItem->picks.push_back(fresh);
    
    //add the row to the constant table.
    QTableWidgetItem *work = nullptr;
    
    int row = constantTableWidget->rowCount();
    constantTableWidget->insertRow(row);
    
    work = new QTableWidgetItem(fresh.typeString);
    work->setFlags(work->flags() & ~Qt::ItemIsEditable);
    constantTableWidget->setItem(row, 0, work);
    
    work = new QTableWidgetItem(QString::fromStdString(gu::idToString(fresh.pickId)));
    work->setFlags(work->flags() & ~Qt::ItemIsEditable);
    constantTableWidget->setItem(row, 1, work);
  }
  else if(currentItem->type() == VariableItem::itemType)
  {
    VariableItem *vItem = dynamic_cast<VariableItem*>(currentItem);
    assert(vItem);
    
    if (vItem->pick.pickId.is_nil())
    {
      assert(sMessage.type == slc::Type::Edge);
      
      vItem->pick.pickId = sMessage.shapeId;
      vItem->pick.typeString = QString::fromStdString(slc::getNameOfType(sMessage.type));
      vItem->pick.highlightIds = contourIds;
      
      TopoDS_Vertex firstVertex = blendMaker.FirstVertex(1);
      assert(parentShape.hasShapeIdRecord(firstVertex));
      uuid firstId = parentShape.findShapeIdRecord(firstVertex).id;
      BlendEntry firstEntry;
      firstEntry.radius = 1.0; //some kind of default.
      firstEntry.typeString = tr("Vertex");
      firstEntry.pickId = firstId;
      firstEntry.pointLocation = gu::toOsg(TopoDS::Vertex(parentShape.getOCCTShape(firstId)));
      vItem->constraints.push_back(firstEntry);
      addVariableTableItem(QString::number(firstEntry.radius, 'f', 12), firstEntry.typeString, firstEntry.pickId);
      runningIds.insert(firstId);
      
      TopoDS_Vertex lastVertex = blendMaker.LastVertex(1);
      assert(parentShape.hasShapeIdRecord(lastVertex));
      uuid lastId = parentShape.findShapeIdRecord(lastVertex).id;
      BlendEntry lastEntry;
      lastEntry.radius = 1.0; //some kind of default.
      lastEntry.typeString = tr("Vertex");
      lastEntry.pickId = lastId;
      lastEntry.pointLocation = gu::toOsg(TopoDS::Vertex(parentShape.getOCCTShape(lastId)));
      vItem->constraints.push_back(lastEntry);
      addVariableTableItem(QString::number(lastEntry.radius, 'f', 12), lastEntry.typeString, lastEntry.pickId);
      runningIds.insert(lastId);

      observer->outBlocked(msg::buildSelectionMask
        (~slc::All | slc::PointsEnabled | slc::PointsSelectable |
        slc::EndPointsEnabled | slc::EndPointsSelectable));
    }
    else
    {
      double radius = 1.0; //somekind of default
      if (!vItem->constraints.empty())
        radius = vItem->constraints.back().radius;
      
      uuid point;
      if (sMessage.type == slc::Type::StartPoint)
        point = parentShape.useGetStartVertex(sMessage.shapeId);
      else if (sMessage.type == slc::Type::EndPoint)
        point = parentShape.useGetEndVertex(sMessage.shapeId);
      else
        assert(0); //unrecognized selection type.
        
      if (runningIds.count(point) > 0)
      {
        msg::Message out = messageIn;
        out.mask = msg::Request | msg::Selection | msg::Remove;
        auto block = observer->createBlocker();
        observer->out(out);
        observer->out(msg::buildStatusMessage("Point already defined"));
        return;
      }
      
      //make sure vertex is on contour.
      std::set<uuid> contourVertices;
      for (const auto &highlight : vItem->pick.highlightIds)
      {
        auto edgeVertices = blendParent->getSeerShape().useGetChildrenOfType(highlight, TopAbs_VERTEX);
        assert(edgeVertices.size() == 2);
        contourVertices.insert(edgeVertices.front());
        contourVertices.insert(edgeVertices.back());
      }
      if (contourVertices.count(point) != 1)
      {
        msg::Message out = messageIn;
        out.mask = msg::Request | msg::Selection | msg::Remove;
        auto block = observer->createBlocker();
        observer->out(out);
        observer->out(msg::buildStatusMessage("Point not on contour"));
        return;
      }
      
      //add another constraint.
      BlendEntry fresh;
      fresh.pickId = point;
      fresh.radius = radius;
      fresh.typeString = tr("Vertex");
      fresh.pointLocation = gu::toOsg(TopoDS::Vertex(parentShape.getOCCTShape(point)));
      vItem->constraints.push_back(fresh);
      addVariableTableItem(QString::number(fresh.radius, 'f', 12), fresh.typeString, fresh.pickId);
      
      variableTableWidget->selectionModel()->clearSelection();
      auto block = observer->createBlocker();
      observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
      
      //we just came from the 3d view, so in order for dialog to get
      //keyboard input we need to activate.
      variableTableWidget->activateWindow();
      
      variableTableWidget->selectRow(variableTableWidget->rowCount() - 1);
      QTableWidgetItem *item = variableTableWidget->item(variableTableWidget->rowCount() - 1, 0);
      variableTableWidget->setCurrentItem(item);
      variableTableWidget->editItem(item);
      
      observer->out(msg::buildStatusMessage("New vertex added. Set radius"));
    }
  }
}

QTableWidgetItem* Blend::addVariableTableItem(const QString &radius, const QString &typeIn, const uuid &idIn)
{
  QTableWidgetItem *work = nullptr;
  
  int row = variableTableWidget->rowCount();
  variableTableWidget->insertRow(row);
  
  //have to set the id first so when we set the radius and it triggers
  //item changed call back, the id is valid. just do in reverse then so
  //everything is set by the time the call back is called.
  work = new QTableWidgetItem(QString::fromStdString(gu::idToString(idIn))); 
  work->setFlags(work->flags() & ~Qt::ItemIsEditable & ~Qt::ItemIsDropEnabled);
  variableTableWidget->setItem(row, 2, work);
  
  work = new QTableWidgetItem(typeIn); 
  work->setFlags(work->flags() & ~Qt::ItemIsEditable & ~Qt::ItemIsDropEnabled);
  variableTableWidget->setItem(row, 1, work);
  
  work = new QTableWidgetItem(radius);
  work->setFlags(work->flags() | Qt::ItemIsDropEnabled);
  variableTableWidget->setItem(row, 0, work);
  
  return work;
}

void Blend::variableTableSelectionChangedSlot(const QItemSelection &current, const QItemSelection&)
{
  if (current.indexes().isEmpty())
    return;
  
  auto block = observer->createBlocker();
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  QModelIndex index = variableTableWidget->model()->index(current.indexes().front().row(), 2);
  assert(index.isValid());
  uuid pickId = gu::stringToId(variableTableWidget->model()->data(index).toString().toStdString());
  
  const ftr::SeerShape &parentShape = blendParent->getSeerShape();
  std::vector<uuid> parentEdges = parentShape.useGetParentsOfType(pickId, TopAbs_EDGE);
  assert(!parentEdges.empty());
  uuid parentEdge = parentEdges.front();
  slc::Type selectionType;
  if (parentShape.useGetStartVertex(parentEdge) == pickId)
    selectionType = slc::Type::StartPoint;
  else if(parentShape.useGetEndVertex(parentEdge) == pickId)
    selectionType = slc::Type::EndPoint;
  else
    assert(0);//mis matched edge to pick relationship.
    
  // highlight.
  msg::Message freshMMessage(msg::Request | msg::Selection | msg::Add);
  slc::Message freshSMessage;
  freshSMessage.type = selectionType;
  freshSMessage.featureId = blendParent->getId();
  freshSMessage.featureType = blendParent->getType();
  freshSMessage.shapeId = parentEdge;
  freshSMessage.pointLocation = gu::toOsg(TopoDS::Vertex(parentShape.getOCCTShape(pickId)));
  freshMMessage.payload = freshSMessage;
  observer->out(freshMMessage);
}

void Blend::variableTableItemChangedSlot(QTableWidgetItem *item)
{
  int row = variableTableWidget->row(item);
  int column = variableTableWidget->column(item);
  
  //even though only the radius is editable we still get here
  //when we set the other columns programmatically.
  if (column != 0)
    return;
  
  QModelIndex index = variableTableWidget->model()->index(row, 2);
  assert(index.isValid());
  uuid pickId = gu::stringToId(variableTableWidget->model()->data(index).toString().toStdString());
  
  QList<QListWidgetItem *> blendItems = blendList->selectedItems();
  assert(blendItems.size() == 1);
  VariableItem *vItem = dynamic_cast<VariableItem *>(blendItems.front());
  assert(vItem);
  for (auto &constraint : vItem->constraints)
  {
    if (constraint.pickId != pickId)
      continue;
    double temp = item->data(Qt::DisplayRole).toDouble();
    if (temp >= 0.0)
      constraint.radius = temp;
    else
    {
      observer->out(msg::buildStatusMessage("No negative radei"));
      item->setData(Qt::DisplayRole, QString::number(constraint.radius, 'f', 12));
    }
    
    break;
  }
}

void Blend::variableTableRemoveSlot()
{
  QModelIndexList selection = variableTableWidget->selectionModel()->selectedIndexes();
  if (selection.isEmpty())
  {
    observer->out(msg::buildStatusMessage("Nothing selected to remove"));
    return;
  }
  
  //the first 2 entries are the start and finish vertex of the contour.
  //these must be present so don't remove.
  if (selection.front().row() < 2)
  {
    observer->out(msg::buildStatusMessage("Can't remove start or finish vertices"));
    return;
  }
  
  //single selection so only 1.
  QModelIndex index = variableTableWidget->model()->index(selection.front().row(), 2);
  uuid pickId = gu::stringToId(variableTableWidget->model()->data(index).toString().toStdString());
  variableTableWidget->removeRow(index.row());
  variableTableWidget->clearSelection();
  observer->outBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  VariableItem *vItem = dynamic_cast<VariableItem*>(blendList->selectedItems().front());
  assert(vItem);
  std::vector<BlendEntry>::iterator eraseMe = vItem->constraints.end();
  for (auto it = vItem->constraints.begin(); it < vItem->constraints.end(); ++it)
  {
    if (it->pickId == pickId)
    {
      eraseMe = it;
      continue;
    }
  }
  assert(eraseMe != vItem->constraints.end());
  
  for (const auto &highlightId : eraseMe->highlightIds)
    runningIds.erase(highlightId);
  
  vItem->constraints.erase(eraseMe);
  
  observer->out(msg::buildStatusMessage("Vertex removed."));
}

void Blend::requestConstantLinkSlot(const QString &stringIn)
{
  boost::uuids::uuid formulaId = gu::stringToId(stringIn.toStdString());
  assert(!formulaId.is_nil());
  expr::Manager &eManager = static_cast<app::Application *>(qApp)->getProject()->getManager();
  assert(eManager.hasFormula(formulaId));
  
  ConstantItem *cItem = dynamic_cast<ConstantItem*>(blendList->selectedItems().front());
  assert(cItem);
  cItem->expressionLinkId = formulaId;
  
  constantRadiusEdit->lineEdit->setText
    (QString::fromStdString(eManager.getFormulaName(formulaId)));
  constantRadiusEdit->lineEdit->setReadOnly(true);
  constantRadiusEdit->trafficLabel->setLinkSlot();
  
  this->activateWindow();
}

void Blend::requestConstantUnlinkSlot()
{
  expr::Manager &eManager = static_cast<app::Application *>(qApp)->getProject()->getManager();
  
  ConstantItem *cItem = dynamic_cast<ConstantItem*>(blendList->selectedItems().front());
  assert(cItem);
  assert(!cItem->expressionLinkId.is_nil());
  assert(eManager.hasFormula(cItem->expressionLinkId));
  assert(eManager.getFormulaValueType(cItem->expressionLinkId) == expr::ValueType::Scalar);
  
  constantRadiusEdit->trafficLabel->setTrafficGreenSlot();
  constantRadiusEdit->lineEdit->setReadOnly(false);
  constantRadiusEdit->lineEdit->setFocus();
  constantRadiusEdit->lineEdit->setText(QString::number(
    boost::get<double>(eManager.getFormulaValue(cItem->expressionLinkId)), 'f', 12));
  constantRadiusEdit->lineEdit->selectAll();
  
  cItem->expressionLinkId = gu::createNilId();
}

void Blend::requestVariableLinkSlot(QTableWidgetItem *item, const QString &idIn)
{
  uuid expressionId = gu::stringToId(idIn.toStdString());
  assert(!expressionId.is_nil());
  expr::Manager &eManager = static_cast<app::Application *>(qApp)->getProject()->getManager();
  assert(eManager.hasFormula(expressionId));
  assert(eManager.getFormulaValueType(expressionId) == expr::ValueType::Scalar);
  if (!(boost::get<double>(eManager.getFormulaValue(expressionId)) > 0.0))
  {
    observer->out(msg::buildStatusMessage("No negative radei"));
    return;
  }
  
  int row = variableTableWidget->row(item);
  int column = variableTableWidget->column(item);
  assert(column == 0); //always radius column;
  
  QModelIndex index = variableTableWidget->model()->index(row, 2);
  assert(index.isValid());
  uuid pickId = gu::stringToId(variableTableWidget->model()->data(index).toString().toStdString());
  
  QList<QListWidgetItem *> blendItems = blendList->selectedItems();
  assert(blendItems.size() == 1);
  VariableItem *vItem = dynamic_cast<VariableItem *>(blendItems.front());
  assert(vItem);
  for (auto &constraint : vItem->constraints)
  {
    if (constraint.pickId != pickId)
      continue;
    constraint.expressionLinkId = expressionId;
    item->setText(QString::fromStdString(eManager.getFormulaName(expressionId)));
    item->setData(Qt::UserRole, true); //used by delegate to check for linking.
    break;
  }
}

void Blend::requestVariableUnlinkSlot()
{
  QList<QListWidgetItem*> selected = blendList->selectedItems();
  assert(selected.size() == 1); //we have single selection enabled on control.
  VariableItem *vItem = dynamic_cast<VariableItem*>(selected.front());
  assert(vItem);
  
  QModelIndexList vSelection = variableTableWidget->selectionModel()->selectedIndexes();
  assert(!vSelection.isEmpty());
  QModelIndex idIndex = variableTableWidget->model()->index(vSelection.front().row(), 2);
  assert(idIndex.isValid());
  uuid pickId = gu::stringToId(variableTableWidget->model()->data(idIndex).toString().toStdString());
  assert(!pickId.is_nil());
  
  expr::Manager &eManager = static_cast<app::Application *>(qApp)->getProject()->getManager();
  
  for (auto &constraint : vItem->constraints)
  {
    if (constraint.pickId != pickId)
      continue;
    
    assert(eManager.hasFormula(constraint.expressionLinkId));
    assert(eManager.getFormulaValueType(constraint.expressionLinkId) == expr::ValueType::Scalar);
    double value = boost::get<double>(eManager.getFormulaValue(constraint.expressionLinkId));
    QTableWidgetItem *item = variableTableWidget->item(vSelection.front().row(), 0);
    assert(item);
    item->setText(QString::number(value, 'f', 12));
    item->setData(Qt::UserRole, false); //used by delegate to check for linking.
    constraint.expressionLinkId = gu::createNilId();
    break;
  }
}

void Blend::buildGui()
{
  //constant radius.
  QLabel *constantRadiusLabel = new QLabel(tr("Radius"), this);
  constantRadiusEdit = new dlg::ExpressionEdit(this);
  connect(constantRadiusEdit->lineEdit, SIGNAL(editingFinished()), this, SLOT(constantRadiusEditingFinishedSlot()));
  connect(constantRadiusEdit->lineEdit, SIGNAL(textEdited(QString)), this, SLOT(constantRadiusEditedSlot(QString)));
  ExpressionEditFilter *filter = new ExpressionEditFilter(this);
  constantRadiusEdit->lineEdit->installEventFilter(filter);
  connect(filter, SIGNAL(requestLinkSignal(QString)), this, SLOT(requestConstantLinkSlot(QString)));
  connect(constantRadiusEdit->trafficLabel, SIGNAL(requestUnlinkSignal()), this, SLOT(requestConstantUnlinkSlot()));
  QHBoxLayout *constantRadiusLayout = new QHBoxLayout();
  constantRadiusLayout->addWidget(constantRadiusLabel);
  constantRadiusLayout->addWidget(constantRadiusEdit);
  
  constantTableWidget = new QTableWidget(this);
  constantTableWidget->setColumnCount(2);
  constantTableWidget->setRowCount(2); //temp for visual test.
  constantTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
  constantTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
  constantTableWidget->horizontalHeader()->setStretchLastSection(true);
  constantTableWidget->setHorizontalHeaderLabels(QStringList({tr("Type"), tr("Id")}));
  constantTableWidget->verticalHeader()->hide();
  constantTableWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
  connect(constantTableWidget->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
          this, SLOT(constantTableSelectionChangedSlot(const QItemSelection&, const QItemSelection&)));
  QAction *constantTableRemoveAction = new QAction(tr("Remove"), constantTableWidget);
  connect(constantTableRemoveAction, SIGNAL(triggered()), this, SLOT(constantTableRemoveSlot()));
  constantTableWidget->addAction(constantTableRemoveAction);
  
  
  QVBoxLayout *constantPageLayout = new QVBoxLayout();
  constantPageLayout->addLayout(constantRadiusLayout);
  constantPageLayout->addWidget(constantTableWidget);
  
  QWidget *constantPage = new QWidget(this);
  constantPage->setLayout(constantPageLayout);
  
  
  //variable radius.
  variableTableWidget = new QTableWidget(this);
  variableTableWidget->setColumnCount(3);
  variableTableWidget->setRowCount(2); //temp for visual test.
  variableTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
  variableTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
  variableTableWidget->horizontalHeader()->setStretchLastSection(true);
  variableTableWidget->setHorizontalHeaderLabels(QStringList({tr("Radius"), tr("Type"), tr("Id")}));
  variableTableWidget->verticalHeader()->hide();
  variableTableWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
  variableTableWidget->setDragDropMode(QAbstractItemView::DropOnly);
  variableTableWidget->setAcceptDrops(true);
  variableTableWidget->viewport()->setAcceptDrops(true);
  variableTableWidget->setDropIndicatorShown(true);
  connect(variableTableWidget->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
          this, SLOT(variableTableSelectionChangedSlot(const QItemSelection&, const QItemSelection&)));
  connect(variableTableWidget, SIGNAL(itemChanged(QTableWidgetItem*)),
          this, SLOT(variableTableItemChangedSlot(QTableWidgetItem*)));
  QAction *variableTableRemoveAction = new QAction(tr("Remove"), variableTableWidget);
  connect(variableTableRemoveAction, SIGNAL(triggered()), this, SLOT(variableTableRemoveSlot()));
  variableTableWidget->addAction(variableTableRemoveAction);
  
  VariableDropFilter *variableFilter = new VariableDropFilter(this);
  variableTableWidget->installEventFilter(variableFilter);
  connect(variableFilter, SIGNAL(requestLinkSignal(QTableWidgetItem*, const QString&)),
          this, SLOT(requestVariableLinkSlot(QTableWidgetItem*, const QString&)));
  
  VariableDelegate *vDelegate = new VariableDelegate(variableTableWidget);
  variableTableWidget->setItemDelegateForColumn(0, vDelegate);
  connect(vDelegate, SIGNAL(requestUnlinkSignal()), this, SLOT(requestVariableUnlinkSlot()));
  
  //Stacked widget of page constant and variable widgets.
  stackedWidget = new QStackedWidget(this);
  stackedWidget->addWidget(constantPage);
  stackedWidget->addWidget(variableTableWidget);
  stackedWidget->hide(); //shown when item in blendList is selected.
  
  
  //fillet shape.
  QLabel *filletShapeLabel = new QLabel(tr("Fillet Shape: "), this);
  QComboBox *filletShapeCombo = new QComboBox(this);
  filletShapeCombo->addItem(tr("Rational"));
  filletShapeCombo->addItem(tr("QuasiAngular"));
  filletShapeCombo->addItem(tr("Polynomial"));
  filletShapeCombo->setDisabled(true);
  QHBoxLayout *filletShapeLayout = new QHBoxLayout();
  filletShapeLayout->addStretch();
  filletShapeLayout->addWidget(filletShapeLabel);
  filletShapeLayout->addWidget(filletShapeCombo);
  
  
  //Right hand side layout.
  QVBoxLayout *rhsLayout = new QVBoxLayout();
  rhsLayout->addLayout(filletShapeLayout);
  rhsLayout->addStretch();
  rhsLayout->addWidget(stackedWidget);
  
  
  blendList = new QListWidget(this);
  blendList->setContextMenuPolicy(Qt::ActionsContextMenu);
  connect(blendList->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
          this, SLOT(blendListCurrentItemChangedSlot(const QItemSelection&, const QItemSelection&)));
  QAction *addConstantBlendAction = new QAction(tr("Add Constant"), blendList);
  QAction *addVariableBlendAction = new QAction(tr("Add Variable"), blendList);
  QAction *removeBlendAction = new QAction(tr("Remove"), blendList);
  connect(addConstantBlendAction, SIGNAL(triggered()), this, SLOT(addConstantBlendSlot()));
  connect(addVariableBlendAction, SIGNAL(triggered()), this, SLOT(addVariableBlendSlot()));
  connect(removeBlendAction, SIGNAL(triggered()), this, SLOT(removeBlendSlot()));
  blendList->addAction(addConstantBlendAction);
  blendList->addAction(addVariableBlendAction);
  blendList->addAction(removeBlendAction);
  
  //splitter.
  QWidget *rhsWidget = new QWidget(this);
  rhsWidget->setLayout(rhsLayout);
  SplitterDecorated *splitter = new SplitterDecorated(this);
  splitter->setOrientation(Qt::Horizontal);
  splitter->addWidget(blendList);
  splitter->addWidget(rhsWidget);
  splitter->setCollapsible(0, false);
  splitter->setCollapsible(1, false);
  splitter->restoreSettings("dlg::Blend");
  
  //main layout.
  QDialogButtonBox *buttons = new QDialogButtonBox
    (QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
  QHBoxLayout *buttonLayout = new QHBoxLayout();
  buttonLayout->addStretch();
  buttonLayout->addWidget(buttons);
  connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
  
  QVBoxLayout *mainLayout = new QVBoxLayout();
  mainLayout->addWidget(splitter);
  mainLayout->addLayout(buttonLayout);
  this->setLayout(mainLayout);
  
  blendList->setFocus();
}

bool VariableDropFilter::eventFilter(QObject *obj, QEvent *event)
{
  QTableWidget *tableWidget = dynamic_cast<QTableWidget*>(obj);
  assert(tableWidget);
  
  auto getId = [](const QString &stringIn)
  {
    boost::uuids::uuid idOut = gu::createNilId();
    if (stringIn.startsWith("ExpressionId;"))
    {
      QStringList split = stringIn.split(";");
      if (split.size() == 2)
        idOut = gu::stringToId(split.at(1).toStdString());
    }
    return idOut;
  };
  
  auto getItem = [&](const QPoint &pointIn)
  {
    //Really! the fucking header throws off the position.
    int headerHeight = tableWidget->horizontalHeader()->height();
    QPoint adjusted(pointIn.x(), pointIn.y() - headerHeight);
    QTableWidgetItem *item = tableWidget->itemAt(adjusted);
    return item; //might be null.
  };
  
  if (event->type() == QEvent::DragEnter)
  {
    QDragEnterEvent *dEvent = dynamic_cast<QDragEnterEvent*>(event);
    if (dEvent->mimeData()->hasText())
    {
      boost::uuids::uuid id = getId(dEvent->mimeData()->text());
      if (!id.is_nil())
        dEvent->acceptProposedAction();
    }
    return true;
  }
  else if (event->type() == QEvent::DragMove)
  {
    QDragMoveEvent *dEvent = dynamic_cast<QDragMoveEvent*>(event);
    assert(dEvent);
    
    dEvent->ignore(); //ignore by default
    QTableWidgetItem *item = getItem(dEvent->pos());
    if
    (
      dEvent->mimeData()->hasText()
      && item
      && (item->flags() & Qt::ItemIsDropEnabled)
      && (!(getId(dEvent->mimeData()->text()).is_nil()))
    )
      dEvent->acceptProposedAction();
      
    return true;
  }
  else if (event->type() == QEvent::Drop)
  {
    QDropEvent *dEvent = dynamic_cast<QDropEvent*>(event);
    assert(dEvent);
    
    dEvent->ignore(); //ignore by default
    uuid expressionId = getId(dEvent->mimeData()->text());
    QTableWidgetItem *item = getItem(dEvent->pos());
    if
    (
      dEvent->mimeData()->hasText()
      && item
      && (item->flags() & Qt::ItemIsDropEnabled)
      && (!(expressionId.is_nil()))
    )
    {
      dEvent->acceptProposedAction();
      Q_EMIT requestLinkSignal(item, QString::fromStdString(gu::idToString(expressionId)));
    }
    
    return true;
  }
  else
    return QObject::eventFilter(obj, event);
}

VariableDelegate::VariableDelegate(QObject *parent): QStyledItemDelegate(parent)
{
  
}

QWidget* VariableDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex&) const
{
  eEditor = new dlg::ExpressionEdit(parent);
  return eEditor;
}

void VariableDelegate::setEditorData(QWidget*, const QModelIndex& index) const
{
  assert(eEditor);
  eEditor->lineEdit->setText(index.model()->data(index, Qt::EditRole).toString());
  
  QVariant isLinked = index.model()->data(index, Qt::UserRole);
  if (isLinked.isNull() || (isLinked.toBool() == false))
  {
    QTimer::singleShot(0, eEditor->trafficLabel, SLOT(setTrafficGreenSlot()));
    eEditor->lineEdit->setReadOnly(false);
    isExpressionLinked = false;
  }
  else
  {
    QTimer::singleShot(0, eEditor->trafficLabel, SLOT(setLinkSlot()));
    eEditor->lineEdit->setReadOnly(true);
    isExpressionLinked = true;
  }
  
  connect (eEditor->lineEdit, SIGNAL(textEdited(QString)), this, SLOT(textEditedSlot(QString)));
  connect (eEditor->trafficLabel, SIGNAL(requestUnlinkSignal()), this, SLOT(requestUnlinkSlot()));
}

void VariableDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex&) const
{
  //this is called before setEditorData.
  editor->setGeometry(option.rect);
}

void VariableDelegate::setModelData(QWidget*, QAbstractItemModel* model, const QModelIndex& index) const
{
  assert(eEditor);
  
  if (isExpressionLinked)
    return; //shouldn't need to do anything if the expression is linked.
  
  expr::Manager localManager;
  expr::StringTranslator translator(localManager);
  std::string formula("temp = ");
  formula += eEditor->lineEdit->text().toStdString();
  if (translator.parseString(formula) == expr::StringTranslator::ParseSucceeded)
  {
    localManager.update();
    assert(localManager.getFormulaValueType(translator.getFormulaOutId()) == expr::ValueType::Scalar);
    double value = boost::get<double>(localManager.getFormulaValue(translator.getFormulaOutId()));
    QString freshValue = QString::number(value, 'f', 12);
    if (value < 0.0)
    {
      QMessageBox::critical
      (
        static_cast<app::Application*>(qApp)->getMainWindow(),
       tr("Error:"), tr("No negative numbers for radius")
       
      );
      return;
    }
    if (!model->setData(index, freshValue, Qt::EditRole))
    {
      QMessageBox::critical
      (
        static_cast<app::Application*>(qApp)->getMainWindow(),
       tr("Error:"), tr("Couldn't set model data.")
        
      );
    }
  }
  else
  {
    QMessageBox::critical
    (
      static_cast<app::Application*>(qApp)->getMainWindow(),
     tr("Error:"), tr("Couldn't parse string.")
    );
  }
}

void VariableDelegate::textEditedSlot(const QString &textIn)
{
  assert(eEditor);
  eEditor->trafficLabel->setTrafficYellowSlot();
  qApp->processEvents(); //need this or we never see yellow signal.
  
  expr::Manager localManager;
  expr::StringTranslator translator(localManager);
  std::string formula("temp = ");
  formula += textIn.toStdString();
  if (translator.parseString(formula) == expr::StringTranslator::ParseSucceeded)
  {
    localManager.update();
    eEditor->trafficLabel->setTrafficGreenSlot();
    assert(localManager.getFormulaValueType(translator.getFormulaOutId()) == expr::ValueType::Scalar);
    double value = boost::get<double>(localManager.getFormulaValue(translator.getFormulaOutId()));
    eEditor->goToolTipSlot(QString::number(value));
  }
  else
  {
    eEditor->trafficLabel->setTrafficRedSlot();
    int position = translator.getFailedPosition() - 8; // 7 chars for 'temp = ' + 1
    eEditor->goToolTipSlot(textIn.left(position) + "?");
  }
}

void VariableDelegate::requestUnlinkSlot()
{
  assert(isExpressionLinked); //shouldn't be able to get here if expression is not linked.
  
  QKeyEvent *event = new QKeyEvent (QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
  qApp->postEvent (eEditor, event);
  Q_EMIT requestUnlinkSignal(); //maybe delay this with a timer.
}

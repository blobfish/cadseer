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

#include <TopoDS.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>

#include <tools/idtools.h>
#include <application/application.h>
#include <application/splitterdecorated.h>
#include <project/project.h>
#include <message/message.h>
#include <message/observer.h>
#include <selection/message.h>
#include <feature/seershape.h>
#include <feature/blend.h>
#include <dialogs/widgetgeometry.h>
#include <dialogs/blend.h>

using boost::uuids::uuid;

using namespace dlg;

BlendEntry::BlendEntry()
{
  pickId = gu::createNilId();
  typeString = QString::fromStdString(slc::getNameOfType(slc::Type::None));
  radius = 1.0; //some kind of default
}

BlendEntry::BlendEntry(const slc::Message &sMessageIn)
{
  pickId = sMessageIn.shapeId;
  typeString = QString::fromStdString(slc::getNameOfType(sMessageIn.type));
  pointLocation = sMessageIn.pointLocation;
  radius = 1.0; //some kind of default
}

ConstantItem::ConstantItem(QListWidget *parent) :
  QListWidgetItem(QObject::tr("Constant"), parent, ConstantItem::itemType)
{
  radius = 1.0; //somekind of default.
}

VariableItem::VariableItem(QListWidget *parent) :
  QListWidgetItem(QObject::tr("Variable"), parent, VariableItem::itemType)
{
  pick.pickId = gu::createNilId();
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

Blend::Blend(const uuid &editBlendIn, QWidget *parent) : QDialog(parent)
{
  init();
  isEditDialog = true;
  
  //smart pointer to remain inValid in edit 'mode'.
  prj::Project *project = static_cast<app::Application*>(qApp)->getProject();
  assert(project);
  blend = dynamic_cast<ftr::Blend*>(project->findFeature(editBlendIn));
  assert(blend);
  
  //todo get parent feature from project and assign.
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
  
  observer->messageOutSignal(msg::buildSelectionMask(~slc::All));
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

void Blend::closeEvent(QCloseEvent *e)
{
  prj::Project *project = static_cast<app::Application *>(qApp)->getProject();
  
  //blendSmart and blendParent might be invalid dependent on state during close.
  if (!isEditDialog && isAccepted && blendSmart && blendParent) // = creation dialog that got accepted.
  {
    assert(blend);
    assert(blendParent);
    assert(blendParent->hasSeerShape());
    
    for (int index = 0; index < blendList->count(); ++index)
    {
      QListWidgetItem *item = blendList->item(index);
      if (item->type() == ConstantItem::itemType)
      {
        ConstantItem *cItem = dynamic_cast<ConstantItem*>(item);
        assert(cItem);
        ftr::SimpleBlend sBlend;
        for (const auto &entry : cItem->picks)
        {
          ftr::Pick fPick = convert(entry);
          sBlend.picks.push_back(fPick);
        }
        auto radiusParameter = ftr::Blend::buildRadiusParameter();
        radiusParameter->setValue(cItem->radius);
        sBlend.radius = radiusParameter;
        
        blend->addSimpleBlend(sBlend);
      }
      else if(item->type() == VariableItem::itemType)
      {
        VariableItem *vItem = dynamic_cast<VariableItem*>(item);
        assert(vItem);
        
        ftr::Pick fPick = convert(vItem->pick);
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
            break;
          }
          if(it == blendCue.entries.end())
          {
            //not a match so build another one.
            ftr::VariableEntry entry;
            entry.id = constraint.pickId;
            entry.radius = ftr::Blend::buildRadiusParameter();
            entry.radius->setValue(constraint.radius);
            entry.position = ftr::Blend::buildPositionParameter();
            blendCue.entries.push_back(entry);
          }
        }
        
        blend->addVariableBlend(blendCue);
      }
      else
        assert(0);//unrecognized item type
    }
    
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
    
    blend->clearBlends();
    //todo fill in new blends;
  }
  
  QDialog::closeEvent(e);
  observer->messageOutSignal(msg::Mask(msg::Request | msg::Command | msg::Done));
}

ftr::Pick Blend::convert(const BlendEntry &entryIn)
{
  assert(blendParent);
  assert(blendParent->hasSeerShape());
  
  ftr::Pick fPick;
  fPick.id = entryIn.pickId;
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
  observer->messageOutSignal(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  qDeleteAll(blendList->selectedItems());
  
  //make sure other widget selections are cleared.
}

void Blend::blendListCurrentItemChangedSlot(const QItemSelection &current, const QItemSelection&)
{
  constantTableWidget->clearSelection();
  variableTableWidget->clearSelection();
  observer->messageOutSignal(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  if (current.indexes().isEmpty())
  {
    stackedWidget->hide();
    boost::signals2::shared_connection_block block(observer->connection);
    observer->messageOutSignal(msg::buildSelectionMask(~slc::All));
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
    boost::signals2::shared_connection_block block(observer->connection);
    observer->messageOutSignal(msg::buildSelectionMask(~slc::All | slc::EdgesEnabled | slc::EdgesSelectable));
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
      boost::signals2::shared_connection_block block(observer->connection);
      observer->messageOutSignal(msg::buildSelectionMask(~slc::All | slc::EdgesEnabled | slc::EdgesSelectable));
    }
    else
    {
      boost::signals2::shared_connection_block block(observer->connection);
      observer->messageOutSignal(msg::buildSelectionMask(~slc::All | slc::EndPointsEnabled | slc::EndPointsSelectable));
    }
    //enable selection of edges if no edge picked, else end points.
    stackedWidget->setCurrentIndex(1);
  }
  else
    assert(0); //unrecognized item type.
}

void Blend::fillInConstant(const ConstantItem &itemIn)
{
  constantRadiusEdit->setText(QString::number(itemIn.radius, 'f', 12));
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
    addVariableTableItem(constraint.radius, constraint.typeString, constraint.pickId);
  
  for (const auto &highlight : itemIn.pick.highlightIds)
    addToSelection(highlight);
}

void Blend::addToSelection(const boost::uuids::uuid &shapeIdIn)
{
  // I can't ever see us needing to handeling a selection addition response
  // triggered from ourselves. Just block and revisit if needed.
  boost::signals2::shared_connection_block block(observer->connection);
  
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
  observer->messageOutSignal(freshMMessage);
}

void Blend::constantRadiusEditingFinishedSlot()
{
  double freshValue = constantRadiusEdit->text().toDouble();
  
  QList<QListWidgetItem*> items = blendList->selectedItems();
  assert(items.size() == 1);
  assert(items.front()->type() == ConstantItem::itemType);
  ConstantItem *cItem = dynamic_cast<ConstantItem*>(items.front());
  assert(cItem);
  
  if (freshValue > 0.0)
    cItem->radius = freshValue;
  else
  {
    constantRadiusEdit->setText(QString::number(cItem->radius, 'f', 12));
    observer->messageOutSignal(msg::buildStatusMessage("Need positive radei"));
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
  
  boost::signals2::shared_connection_block block(observer->connection);
  observer->messageOutSignal(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
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
    observer->messageOutSignal(msg::buildStatusMessage("Nothing selected to remove"));
    return;
  }
  
  //single selection so only 1.
  QModelIndex index = constantTableWidget->model()->index(selection.front().row(), 1);
  uuid pickId = gu::stringToId(constantTableWidget->model()->data(index).toString().toStdString());
  constantTableWidget->removeRow(index.row());
  constantTableWidget->clearSelection();
  boost::signals2::shared_connection_block block(observer->connection);
  observer->messageOutSignal(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
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

void Blend::myAcceptedSlot()
{
  isAccepted = true;
  qApp->postEvent(this, new QCloseEvent());
}

void Blend::myRejectedSlot()
{
  isAccepted = false;
  qApp->postEvent(this, new QCloseEvent());
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
    observer->messageOutSignal(msg::buildStatusMessage("Add and/or select blend from dialog"));
    boost::signals2::shared_connection_block block(observer->connection);
    observer->messageOutSignal(msg::Message(msg::Request | msg::Selection | msg::Clear));
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
      observer->messageOutSignal(msg::buildStatusMessage("Invalid object to blend"));
      boost::signals2::shared_connection_block block(observer->connection);
      observer->messageOutSignal(msg::Message(msg::Request | msg::Selection | msg::Clear));
      return;
    }
  }
  
  if (blendParent->getId() != sMessage.featureId)
  {
    observer->messageOutSignal(msg::buildStatusMessage("Can't tie blend feature to multiple bodies"));
    boost::signals2::shared_connection_block block(observer->connection);
    observer->messageOutSignal(msg::Message(msg::Request | msg::Selection | msg::Clear));
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
        boost::signals2::shared_connection_block block(observer->connection);
        observer->messageOutSignal(msg::Message(msg::Request | msg::Selection | msg::Clear));
        observer->messageOutSignal(msg::buildStatusMessage("Blend contour overlaps other contour"));
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
      addVariableTableItem(firstEntry.radius, firstEntry.typeString, firstEntry.pickId);
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
      addVariableTableItem(lastEntry.radius, lastEntry.typeString, lastEntry.pickId);
      runningIds.insert(lastId);

      boost::signals2::shared_connection_block block(observer->connection);
      observer->messageOutSignal(msg::buildSelectionMask
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
        boost::signals2::shared_connection_block block(observer->connection);
        observer->messageOutSignal(out);
        observer->messageOutSignal(msg::buildStatusMessage("Point already defined"));
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
        boost::signals2::shared_connection_block block(observer->connection);
        observer->messageOutSignal(out);
        observer->messageOutSignal(msg::buildStatusMessage("Point not on contour"));
        return;
      }
      
      //add another constraint.
      BlendEntry fresh;
      fresh.pickId = point;
      fresh.radius = radius;
      fresh.typeString = tr("Vertex");
      fresh.pointLocation = gu::toOsg(TopoDS::Vertex(parentShape.getOCCTShape(point)));
      vItem->constraints.push_back(fresh);
      addVariableTableItem(fresh.radius, fresh.typeString, fresh.pickId);
      
      variableTableWidget->selectionModel()->clearSelection();
      boost::signals2::shared_connection_block block(observer->connection);
      observer->messageOutSignal(msg::Message(msg::Request | msg::Selection | msg::Clear));
      
      //we just came from the 3d view, so in order for dialog to get
      //keyboard input we need to activate.
      variableTableWidget->activateWindow();
      
      variableTableWidget->selectRow(variableTableWidget->rowCount() - 1);
      QTableWidgetItem *item = variableTableWidget->item(variableTableWidget->rowCount() - 1, 0);
      variableTableWidget->setCurrentItem(item);
      variableTableWidget->editItem(item);
      
      observer->messageOutSignal(msg::buildStatusMessage("New vertex added. Set radius"));
    }
  }
}

void Blend::addVariableTableItem(double radius, const QString &typeIn, const boost::uuids::uuid &idIn)
{
  QTableWidgetItem *work = nullptr;
  
  int row = variableTableWidget->rowCount();
  variableTableWidget->insertRow(row);
  
  //have to set the id first so when we set the radius and it triggers
  //item changed call back, the id is valid. just do in reverse then so
  //everything is set by the time the call back is called.
  work = new QTableWidgetItem(QString::fromStdString(gu::idToString(idIn))); 
  work->setFlags(work->flags() & ~Qt::ItemIsEditable);
  variableTableWidget->setItem(row, 2, work);
  
  work = new QTableWidgetItem(typeIn); 
  work->setFlags(work->flags() & ~Qt::ItemIsEditable);
  variableTableWidget->setItem(row, 1, work);
  
  work = new QTableWidgetItem(QString::number(radius, 'f', 12)); //somekind of default.
  variableTableWidget->setItem(row, 0, work);
}

void Blend::variableTableSelectionChangedSlot(const QItemSelection &current, const QItemSelection&)
{
  if (current.indexes().isEmpty())
    return;
  
  boost::signals2::shared_connection_block block(observer->connection);
  observer->messageOutSignal(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
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
  observer->messageOutSignal(freshMMessage);
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
      observer->messageOutSignal(msg::buildStatusMessage("No negative radei"));
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
    observer->messageOutSignal(msg::buildStatusMessage("Nothing selected to remove"));
    return;
  }
  
  //the first 2 entries are the start and finish vertex of the contour.
  //these must be present so don't remove.
  if (selection.front().row() < 2)
  {
    observer->messageOutSignal(msg::buildStatusMessage("Can't remove start or finish vertices"));
    return;
  }
  
  //single selection so only 1.
  QModelIndex index = variableTableWidget->model()->index(selection.front().row(), 2);
  uuid pickId = gu::stringToId(variableTableWidget->model()->data(index).toString().toStdString());
  variableTableWidget->removeRow(index.row());
  variableTableWidget->clearSelection();
  boost::signals2::shared_connection_block block(observer->connection);
  observer->messageOutSignal(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
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
    for (const auto &highlight : it->highlightIds)
      addToSelection(highlight);
  }
  assert(eraseMe != vItem->constraints.end());
  
  for (const auto &highlightId : eraseMe->highlightIds)
    runningIds.erase(highlightId);
  
  vItem->constraints.erase(eraseMe);
  
  observer->messageOutSignal(msg::buildStatusMessage("Vertex removed."));
}

void Blend::buildGui()
{
  //constant radius.
  QLabel *constantRadiusLabel = new QLabel(tr("Radius"), this);
  constantRadiusEdit = new QLineEdit(this);
  connect(constantRadiusEdit, SIGNAL(editingFinished()), this, SLOT(constantRadiusEditingFinishedSlot()));
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
  connect(variableTableWidget->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
          this, SLOT(variableTableSelectionChangedSlot(const QItemSelection&, const QItemSelection&)));
  connect(variableTableWidget, SIGNAL(itemChanged(QTableWidgetItem*)),
          this, SLOT(variableTableItemChangedSlot(QTableWidgetItem*)));
  
  QAction *variableTableRemoveAction = new QAction(tr("Remove"), variableTableWidget);
  connect(variableTableRemoveAction, SIGNAL(triggered()), this, SLOT(variableTableRemoveSlot()));
  variableTableWidget->addAction(variableTableRemoveAction);
  
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
  connect(buttons, SIGNAL(accepted()), this, SLOT(myAcceptedSlot()));
  connect(buttons, SIGNAL(rejected()), this, SLOT(myRejectedSlot()));
  
  QVBoxLayout *mainLayout = new QVBoxLayout();
  mainLayout->addWidget(splitter);
  mainLayout->addLayout(buttonLayout);
  this->setLayout(mainLayout);
  
  blendList->setFocus();
}

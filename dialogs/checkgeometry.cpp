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

#include <QVBoxLayout>
#include <QTabWidget>
#include <QTreeWidget>
#include <QCloseEvent>
#include <QSettings>
#include <QHeaderView>
#include <QHideEvent>

#include <BRepCheck_Analyzer.hxx>
#include <TopExp_Explorer.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>

#include <osg/Group>
#include <osg/PositionAttitudeTransform>
#include <osg/Geometry>
#include <osg/BlendFunc>
#include <osg/PolygonMode>
#include <osg/ShadeModel>

#include <application/application.h>
#include <feature/base.h>
#include <feature/seershape.h>
#include <message/message.h>
#include <message/observer.h>
#include <tools/idtools.h>
#include <library/spherebuilder.h>
#include <dialogs/widgetgeometry.h>
#include <dialogs/checkgeometry.h>

using namespace dlg;
using boost::uuids::uuid;

QString checkStatusToString(int index)
{
  static QStringList names = 
  {
    "No Error",                           //    BRepCheck_NoError
    "Invalid Point On Curve",             //    BRepCheck_InvalidPointOnCurve
    "Invalid Point On Curve On Surface",  //    BRepCheck_InvalidPointOnCurveOnSurface
    "Invalid Point On Surface",           //    BRepCheck_InvalidPointOnSurface
    "No 3D Curve",                        //    BRepCheck_No3DCurve
    "Multiple 3D Curve",                  //    BRepCheck_Multiple3DCurve
    "Invalid 3D Curve",                   //    BRepCheck_Invalid3DCurve
    "No Curve On Surface",                //    BRepCheck_NoCurveOnSurface
    "Invalid Curve On Surface",           //    BRepCheck_InvalidCurveOnSurface
    "Invalid Curve On Closed Surface",    //    BRepCheck_InvalidCurveOnClosedSurface
    "Invalid Same Range Flag",            //    BRepCheck_InvalidSameRangeFlag
    "Invalid Same Parameter Flag",        //    BRepCheck_InvalidSameParameterFlag
    "Invalid Degenerated Flag",           //    BRepCheck_InvalidDegeneratedFlag
    "Free Edge",                          //    BRepCheck_FreeEdge
    "Invalid MultiConnexity",             //    BRepCheck_InvalidMultiConnexity
    "Invalid Range",                      //    BRepCheck_InvalidRange
    "Empty Wire",                         //    BRepCheck_EmptyWire
    "Redundant Edge",                     //    BRepCheck_RedundantEdge
    "Self Intersecting Wire",             //    BRepCheck_SelfIntersectingWire
    "No Surface",                         //    BRepCheck_NoSurface
    "Invalid Wire",                       //    BRepCheck_InvalidWire
    "Redundant Wire",                     //    BRepCheck_RedundantWire
    "Intersecting Wires",                 //    BRepCheck_IntersectingWires
    "Invalid Imbrication Of Wires",       //    BRepCheck_InvalidImbricationOfWires
    "Empty Shell",                        //    BRepCheck_EmptyShell
    "Redundant Face",                     //    BRepCheck_RedundantFace
    "Unorientable Shape",                 //    BRepCheck_UnorientableShape
    "Not Closed",                         //    BRepCheck_NotClosed
    "Not Connected",                      //    BRepCheck_NotConnected
    "Sub Shape Not In Shape",             //    BRepCheck_SubshapeNotInShape
    "Bad Orientation",                    //    BRepCheck_BadOrientation
    "Bad Orientation Of Sub Shape",       //    BRepCheck_BadOrientationOfSubshape
    "Invalid Tolerance Value",            //    BRepCheck_InvalidToleranceValue
    "Check Failed"                        //    BRepCheck_CheckFail
  };
  
  if (index == -1)
  {
    return QString(QObject::tr("No Result"));
  }
  if (index > 33 || index < 0)
  {
    QString message(QObject::tr("Out Of Enum Range: "));
    QString number;
    number.setNum(index);
    message += number;
    return message;
  }
  return names.at(index);
}

BasicCheckPage::BasicCheckPage(const ftr::Base &featureIn, QWidget *parent) :
  QWidget(parent), feature(featureIn)
{
  observer = std::move(std::unique_ptr<msg::Observer>(new msg::Observer()));
  observer->name = "dlg::BasicCheckPage";
  
  buildGui();
  assert(feature.hasSeerShape());
  go();
  treeWidget->expandAll();
  
  QSettings &settings = static_cast<app::Application*>(qApp)->getUserSettings();
  settings.beginGroup("CheckGeometry");
  settings.beginGroup("BasicPage");
  treeWidget->header()->restoreState(settings.value("header").toByteArray());
  settings.endGroup();
  settings.endGroup();
  
  connect(treeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(selectionChangedSlot()));
}

BasicCheckPage::~BasicCheckPage()
{
  QSettings &settings = static_cast<app::Application*>(qApp)->getUserSettings();
  settings.beginGroup("CheckGeometry");
  settings.beginGroup("BasicPage");
  settings.setValue("header", treeWidget->header()->saveState());
  settings.endGroup();
  settings.endGroup();
  
  if (boundingSphere.valid()) //remove current boundingSphere.
  {
    assert(boundingSphere->getParents().size() == 1);
    osg::Group *parent = boundingSphere->getParent(0);
    parent->removeChild(boundingSphere.get()); //this should make boundingSphere invalid.
  }
}

void BasicCheckPage::buildGui()
{
  treeWidget = new QTreeWidget(this);
  QVBoxLayout *layout = new QVBoxLayout();
  layout->addWidget(treeWidget);
  this->setLayout(layout);
  treeWidget->setColumnCount(3);
  treeWidget->setHeaderLabels
  (
    QStringList
    {
      tr("Id"),
      tr("Type"),
      tr("Error")
    }
  );
}

void BasicCheckPage::hideEvent(QHideEvent *event)
{
  treeWidget->clearSelection();
  QWidget::hideEvent(event);
}

void BasicCheckPage::go()
{
  //these will have to adjusted for sub shape entry.
  const TopoDS_Shape &shape = feature.getSeerShape().getRootOCCTShape();
  uuid rootId = feature.getSeerShape().getRootShapeId();
  
  //probably need try catch here.
  BRepCheck_Analyzer shapeCheck(shape);
  QTreeWidgetItem *rootItem = new QTreeWidgetItem();
  rootItem->setData(0, Qt::DisplayRole, QString::fromStdString(gu::idToString(rootId)));
  rootItem->setData(1, Qt::DisplayRole, QString::fromStdString(shapeStrings.at(shape.ShapeType())));
  treeWidget->addTopLevelItem(rootItem);
  itemStack.push(rootItem);
  
  if (shapeCheck.IsValid())
    rootItem->setData(2, Qt::DisplayRole, tr("Valid"));
  else
    rootItem->setData(2, Qt::DisplayRole, tr("Invalid"));
  
  recursiveCheck(shapeCheck, shape);
}

void BasicCheckPage::recursiveCheck(const BRepCheck_Analyzer &shapeCheck, const TopoDS_Shape &shape)
{
  const ftr::SeerShape &seerShape = feature.getSeerShape();
  if (seerShape.hasShapeIdRecord(shape))
  {
    uuid shapeId = seerShape.findShapeIdRecord(shape).id;

    BRepCheck_ListIteratorOfListOfStatus listIt;
    if
    (
      (!shapeCheck.Result(shape).IsNull()) &&
      (checkedIds.count(shapeId) == 0)
    )
    {
      listIt.Initialize(shapeCheck.Result(shape)->Status());
      if (listIt.Value() != BRepCheck_NoError)
      {
        QTreeWidgetItem *entry = new QTreeWidgetItem(itemStack.top());
        entry->setData(0, Qt::DisplayRole, QString::fromStdString(gu::idToString(shapeId)));
        entry->setData(1, Qt::DisplayRole, QString::fromStdString(shapeStrings.at(shape.ShapeType())));
        entry->setData(2, Qt::DisplayRole, checkStatusToString(listIt.Value()));
  //       dispatchError(entry, listIt.Value());
        itemStack.push(entry);
    
        if (shape.ShapeType() == TopAbs_SOLID)
          checkSub(shapeCheck, shape, TopAbs_SHELL);
        if (shape.ShapeType() == TopAbs_EDGE)
          checkSub(shapeCheck, shape, TopAbs_VERTEX);
        if (shape.ShapeType() == TopAbs_FACE)
        {
          checkSub(shapeCheck, shape, TopAbs_WIRE);
          checkSub(shapeCheck, shape, TopAbs_EDGE);
          checkSub(shapeCheck, shape, TopAbs_VERTEX);
        }
        itemStack.pop();
      }
    }
    //this happens to excess so no warning out. I believe this is the whole
    //orientation thing. we use TopExp::mapOfShapes in seer shape but this uses a TopoDS_Iterator.
//     else
//     {
//       std::cout << "Warning: no shapeIdRecord in BasicCheckPage::recursiveCheck" << std::endl;
//     }
    checkedIds.insert(shapeId);
  }
  for (TopoDS_Iterator it(shape); it.More(); it.Next())
    recursiveCheck(shapeCheck, it.Value());
}

void BasicCheckPage::checkSub(const BRepCheck_Analyzer &shapeCheck, const TopoDS_Shape &shape,
                                        TopAbs_ShapeEnum subType)
{
  BRepCheck_ListIteratorOfListOfStatus itl;
  TopExp_Explorer exp;
  for (exp.Init(shape,subType); exp.More(); exp.Next())
  {
    const TopoDS_Shape& sub = exp.Current();
    
    const ftr::SeerShape &seerShape = feature.getSeerShape();
    if (!seerShape.hasShapeIdRecord(sub))
    {
      std::cout << "Warning: no shapeIdRecord in BasicCheckPage::checkSub" << std::endl;
      continue;
    }
    uuid subId = seerShape.findShapeIdRecord(sub).id;
    
    const Handle(BRepCheck_Result)& res = shapeCheck.Result(sub);
    for (res->InitContextIterator(); res->MoreShapeInContext(); res->NextShapeInContext())
    {
      if (res->ContextualShape().IsSame(shape))
      {
        for (itl.Initialize(res->StatusOnShape()); itl.More(); itl.Next())
        {
          if (itl.Value() == BRepCheck_NoError)
            break;
          checkedIds.insert(subId);
          QTreeWidgetItem *entry = new QTreeWidgetItem(itemStack.top());
          entry->setData(0, Qt::DisplayRole, QString::fromStdString(gu::idToString(subId)));
          entry->setData(1, Qt::DisplayRole, QString::fromStdString(shapeStrings.at(sub.ShapeType())));
          entry->setData(2, Qt::DisplayRole, checkStatusToString(itl.Value()));
//           dispatchError(entry, itl.Value());
        }
      }
    }
  }
}

void BasicCheckPage::selectionChangedSlot()
{
  //right now we are building and destroy each bounding sphere upon
  //selection change. might want to cache.
  
  //remove previous bounding sphere and clear selection.
  if (boundingSphere.valid()) //remove current boundingSphere.
  {
    assert(boundingSphere->getParents().size() == 1);
    osg::Group *parent = boundingSphere->getParent(0);
    parent->removeChild(boundingSphere.get()); //this should make boundingSphere invalid.
  }
  observer->messageOutSignal(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  //get the fresh id.
  QList<QTreeWidgetItem*> freshSelections = treeWidget->selectedItems();
  if (freshSelections.empty())
    return;
  uuid id = gu::stringToId(freshSelections.at(0)->data(0, Qt::DisplayRole).toString().toStdString());
  assert(!id.is_nil());
  assert(feature.getSeerShape().hasShapeIdRecord(id));
  
  //select the geometry.
  msg::Message message(msg::Request | msg::Selection | msg::Add);
  slc::Message sMessage;
  sMessage.type = slc::convert(feature.getSeerShape().getOCCTShape(id).ShapeType());
  sMessage.featureId = feature.getId();
  sMessage.featureType = feature.getType();
  sMessage.shapeId = id;
  message.payload = sMessage;
  observer->messageOutSignal(message);
  
  osg::BoundingSphered bSphere = calculateBoundingSphere(id);
  if (!bSphere.valid())
  {
    observer->messageOutSignal(msg::buildStatusMessage("Unable to calculate bounding sphere"));
    return;
  }
  
  osg::ref_ptr<osg::PositionAttitudeTransform> transform = new osg::PositionAttitudeTransform();
  boundingSphere = transform;
  transform->setPosition(bSphere.center());
  lbr::SphereBuilder builder;
  builder.setRadius(bSphere.radius());
  builder.setDeviation(0.25);
  osg::Geometry *geometry = builder;
  osg::Vec4Array *color = new osg::Vec4Array();
  color->push_back(osg::Vec4(1.0, 1.0, 0.0, 0.2));
  geometry->setColorArray(color);
  geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
  geometry->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
  osg::BlendFunc* bf = new osg::BlendFunc(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_SRC_ALPHA); 
  geometry->getOrCreateStateSet()->setAttributeAndModes(bf);
  osg::PolygonMode *pm = new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
  geometry->getOrCreateStateSet()->setAttribute(pm);
  transform->addChild(geometry);
  if (feature.getOverlaySwitch()->addChild(transform))
    feature.getOverlaySwitch()->setValue(feature.getOverlaySwitch()->getNumChildren() - 1, true);
}

osg::BoundingSphered BasicCheckPage::calculateBoundingSphere(const boost::uuids::uuid &idIn)
{
  osg::BoundingSphered out;
  
  const ftr::SeerShape &seerShape = feature.getSeerShape();
  assert(seerShape.hasShapeIdRecord(idIn));
  const TopoDS_Shape& shape = seerShape.getOCCTShape(idIn);
  
  Bnd_Box bBox;
  BRepBndLib::Add(shape, bBox);
  if (bBox.IsVoid())
    return out;
  
  osg::Vec3d point1 = gu::toOsg(bBox.CornerMin());
  osg::Vec3d point2 = gu::toOsg(bBox.CornerMax());
  osg::Vec3d diagonalVec = point2 - point1;
  out.radius() = diagonalVec.length() / 2.0;
  diagonalVec.normalize();
  diagonalVec *= out.radius();
  out.center() = point1 + diagonalVec;
  
  return out;
}

BOPCheckPage::BOPCheckPage(const ftr::Base &featureIn, QWidget *parent) :
  QWidget(parent), feature(featureIn)
{
}

void BOPCheckPage::buildGui()
{
}

ToleranceCheckPage::ToleranceCheckPage(const ftr::Base &featureIn, QWidget *parent) :
  QWidget(parent), feature(featureIn)
{
  buildGui();
}

void ToleranceCheckPage::buildGui()
{
}

ShapesPage::ShapesPage(const ftr::Base &featureIn, QWidget *parent) :
  QWidget(parent), feature(featureIn)
{
  buildGui();
}

void ShapesPage::buildGui()
{
}

CheckGeometry::CheckGeometry(const ftr::Base &featureIn, QWidget *parent) :
  QDialog(parent), feature(featureIn)
{
  this->setWindowTitle(tr("Check Geometry"));
  
  observer = std::move(std::unique_ptr<msg::Observer>(new msg::Observer()));
  observer->name = "dlg::CheckGeometry";
  
  buildGui();
  
  WidgetGeometry *filter = new WidgetGeometry(this, "dlg::CheckGeometry");
  this->installEventFilter(filter);
}

CheckGeometry::~CheckGeometry(){}

void CheckGeometry::closeEvent(QCloseEvent *e)
{
  QDialog::closeEvent(e);
  observer->messageOutSignal(msg::Mask(msg::Request | msg::Command | msg::Done));
}

void CheckGeometry::buildGui()
{
  QVBoxLayout *mainLayout = new QVBoxLayout();
  this->setLayout(mainLayout);
  tabWidget = new QTabWidget(this);
  mainLayout->addWidget(tabWidget);
  
  basicCheckPage = new BasicCheckPage(feature, this);
  tabWidget->addTab(basicCheckPage, tr("Basic"));
  
  bopCheckPage = new BOPCheckPage(feature, this);
  tabWidget->addTab(bopCheckPage, tr("Boolean"));
  
  toleranceCheckPage = new ToleranceCheckPage(feature, this);
  tabWidget->addTab(toleranceCheckPage, tr("Tolerance"));
  
  shapesPage = new ShapesPage(feature, this);
  tabWidget->addTab(shapesPage, tr("Shapes"));
}

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
#include <QHBoxLayout>
#include <QTabWidget>
#include <QTreeWidget>
#include <QTableWidget>
#include <QCloseEvent>
#include <QSettings>
#include <QHeaderView>
#include <QHideEvent>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>

#include <boost/variant.hpp>

#include <BRepCheck_Analyzer.hxx>
#include <BOPAlgo_ArgumentAnalyzer.hxx>
#include <TopExp_Explorer.hxx>
#include <TopExp.hxx>
#include <BRep_Tool.hxx>
#include <TopoDS.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <BRepTools_ShapeSet.hxx>
#include <BRepBuilderAPI_Copy.hxx>

#include <osg/Group>
#include <osg/PositionAttitudeTransform>
#include <osg/Geometry>
#include <osg/BlendFunc>
#include <osg/PolygonMode>
#include <osg/Switch>

#include <application/application.h>
#include <feature/base.h>
#include <annex/seershape.h>
#include <message/message.h>
#include <message/observer.h>
#include <message/dispatch.h>
#include <tools/idtools.h>
#include <tools/occtools.h>
#include <library/spherebuilder.h>
#include <dialogs/widgetgeometry.h>
#include <dialogs/checkgeometry.h>

using namespace dlg;
using boost::uuids::uuid;

QString checkStatusToString(int index)
{
  static const QStringList names = 
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

QString BOPCheckStatusToString(BOPAlgo_CheckStatus status)
{
  static const QStringList results =
  {
    QObject::tr("BOPAlgo CheckUnknown"),                //BOPAlgo_CheckUnknown
    QObject::tr("BOPAlgo BadType"),                     //BOPAlgo_BadType
    QObject::tr("BOPAlgo SelfIntersect"),               //BOPAlgo_SelfIntersect
    QObject::tr("BOPAlgo TooSmallEdge"),                //BOPAlgo_TooSmallEdge
    QObject::tr("BOPAlgo NonRecoverableFace"),          //BOPAlgo_NonRecoverableFace
    QObject::tr("BOPAlgo IncompatibilityOfVertex"),     //BOPAlgo_IncompatibilityOfVertex
    QObject::tr("BOPAlgo IncompatibilityOfEdge"),       //BOPAlgo_IncompatibilityOfEdge
    QObject::tr("BOPAlgo IncompatibilityOfFace"),       //BOPAlgo_IncompatibilityOfFace
    QObject::tr("BOPAlgo OperationAborted"),            //BOPAlgo_OperationAborted
    QObject::tr("BOPAlgo GeomAbs_C0"),                  //BOPAlgo_GeomAbs_C0
    QObject::tr("BOPAlgo InvalidCurveOnSurface"),       //BOPAlgo_InvalidCurveOnSurface
    QObject::tr("BOPAlgo NotValid")                     //BOPAlgo_NotValid
  };
  
  return results.at(static_cast<int>(status));
}

static osg::BoundingSphered calculateBoundingSphere(const TopoDS_Shape& shape)
{
  osg::BoundingSphered out;
  
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

static osg::PositionAttitudeTransform* buildBoundingSphere(const osg::BoundingSphered &bSphere)
{
  osg::ref_ptr<osg::PositionAttitudeTransform> transform = new osg::PositionAttitudeTransform();
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
  
  return transform.release();
}

bool convertVertexSelection
(
  const ann::SeerShape& seerShapeIn,
  slc::Message &mInOut
)
{
  const TopoDS_Vertex vertex = TopoDS::Vertex(seerShapeIn.getOCCTShape(mInOut.shapeId));
  std::vector<uuid> parentEdges = seerShapeIn.useGetParentsOfType(mInOut.shapeId, TopAbs_EDGE);
  for (const auto &edge : parentEdges)
  {
    if (seerShapeIn.useGetStartVertex(edge) == mInOut.shapeId)
    {
      mInOut.shapeId = edge;
      mInOut.type = slc::Type::StartPoint;
      return true;
    }
    else if (seerShapeIn.useGetEndVertex(edge) == mInOut.shapeId)
    {
      mInOut.shapeId = edge;
      mInOut.type = slc::Type::EndPoint;
      return true;
    }
  }
  return false;
}

CheckPageBase::CheckPageBase(const ftr::Base &featureIn, QWidget *parent):
  QWidget(parent), feature(featureIn), seerShape(featureIn.getAnnex<ann::SeerShape>(ann::Type::SeerShape))
{
  observer = std::move(std::unique_ptr<msg::Observer>(new msg::Observer()));
  
  minBoundingSphere = calculateBoundingSphere(seerShape.getRootOCCTShape());
  minBoundingSphere.radius() *= .1; //10 percent.
}

CheckPageBase::~CheckPageBase()
{
  if (boundingSphere.valid()) //remove current boundingSphere.
  {
    assert(boundingSphere->getParents().size() == 1);
    osg::Group *parent = boundingSphere->getParent(0);
    parent->removeChild(boundingSphere.get()); //this should make boundingSphere invalid.
  }
}

BasicCheckPage::BasicCheckPage(const ftr::Base &featureIn, QWidget *parent) :
  CheckPageBase(featureIn, parent)
{
  observer->name = "dlg::BasicCheckPage";
  
  buildGui();
  
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
  const TopoDS_Shape &shape = seerShape.getRootOCCTShape();
  uuid rootId = seerShape.getRootShapeId();
  
  //probably need try catch here.
  BRepCheck_Analyzer shapeCheck(shape);
  QTreeWidgetItem *rootItem = new QTreeWidgetItem();
  rootItem->setData(0, Qt::DisplayRole, QString::fromStdString(gu::idToString(rootId)));
  rootItem->setData(1, Qt::DisplayRole, QString::fromStdString(shapeStrings.at(shape.ShapeType())));
  treeWidget->addTopLevelItem(rootItem);
  itemStack.push(rootItem);
  
  if (shapeCheck.IsValid())
  {
    rootItem->setData(2, Qt::DisplayRole, tr("Valid"));
    Q_EMIT(basicCheckPassed());
    return;
  }
  
  rootItem->setData(2, Qt::DisplayRole, tr("Invalid"));
  Q_EMIT(basicCheckFailed());
  recursiveCheck(shapeCheck, shape);
  treeWidget->expandAll();
}

void BasicCheckPage::recursiveCheck(const BRepCheck_Analyzer &shapeCheck, const TopoDS_Shape &shape)
{
  if (seerShape.hasShapeIdRecord(shape))
  {
    uuid shapeId = seerShape.findShapeIdRecord(shape).id;

    if
    (
      (!shapeCheck.Result(shape).IsNull()) &&
      (checkedIds.count(shapeId) == 0)
    )
    {
      
      BRepCheck_ListIteratorOfListOfStatus listIt;
      listIt.Initialize(shapeCheck.Result(shape)->Status());
      QTreeWidgetItem *entry = new QTreeWidgetItem(itemStack.top());
      entry->setData(0, Qt::DisplayRole, QString::fromStdString(gu::idToString(shapeId)));
      entry->setData(1, Qt::DisplayRole, QString::fromStdString(shapeStrings.at(shape.ShapeType())));
      entry->setData(2, Qt::DisplayRole, checkStatusToString(static_cast<int>(listIt.Value())));
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
      
      if
      (
        (itemStack.top()->childCount() == 0) &&
        (listIt.Value() == BRepCheck_NoError)
      )
      {
        itemStack.top()->parent()->removeChild(itemStack.top());
        delete itemStack.top();
      }
      
      itemStack.pop();
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
          entry->setData(2, Qt::DisplayRole, checkStatusToString(static_cast<int>(itl.Value())));
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
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  //get the fresh id.
  QList<QTreeWidgetItem*> freshSelections = treeWidget->selectedItems();
  if (freshSelections.empty())
    return;
  uuid id = gu::stringToId(freshSelections.at(0)->data(0, Qt::DisplayRole).toString().toStdString());
  assert(!id.is_nil());
  assert(seerShape.hasShapeIdRecord(id));
  
  slc::Message sMessage;
  sMessage.type = slc::convert(seerShape.getOCCTShape(id).ShapeType());
  sMessage.featureId = feature.getId();
  sMessage.featureType = feature.getType();
  sMessage.shapeId = id;
  
  osg::BoundingSphered bSphere;
  //if vertex convert to selection point and 'cheat' bounding sphere.
  if (seerShape.getOCCTShape(id).ShapeType() == TopAbs_VERTEX)
  {
    if (!convertVertexSelection(seerShape, sMessage)) //converts vertex id to edge id
    {
      std::cout << "vertex to selection failed: BasicCheckPage::selectionChangedSlot" << std::endl;
      return;
    }
    //just make bounding sphere a percentage of whole shape. what if whole shape is vertex?
    osg::Vec3d vertexPosition = gu::toOsg(BRep_Tool::Pnt(TopoDS::Vertex(seerShape.getOCCTShape(id))));
    bSphere = minBoundingSphere;
    bSphere.center() = vertexPosition;
    sMessage.pointLocation = vertexPosition;
  }
  else
  {
    bSphere = calculateBoundingSphere(seerShape.getOCCTShape(id));
    bSphere.radius() = std::max(bSphere.radius(), minBoundingSphere.radius());
  }
  
  //select the geometry.
  msg::Message message(msg::Request | msg::Selection | msg::Add);
  message.payload = sMessage;
  observer->out(message);
  
  if (!bSphere.valid())
  {
    observer->out(msg::buildStatusMessage("Unable to calculate bounding sphere"));
    return;
  }
  
  boundingSphere = buildBoundingSphere(bSphere);
  if (feature.getOverlaySwitch()->addChild(boundingSphere.get()))
    feature.getOverlaySwitch()->setValue(feature.getOverlaySwitch()->getNumChildren() - 1, true);
}

BOPCheckPage::BOPCheckPage(const ftr::Base &featureIn, QWidget *parent) :
  CheckPageBase(featureIn, parent)
{
  observer->name = "dlg::BOPCheckPage";
}

BOPCheckPage::~BOPCheckPage()
{
  if (tableWidget)
  {
    QSettings &settings = static_cast<app::Application*>(qApp)->getUserSettings();
    settings.beginGroup("CheckGeometry");
    settings.beginGroup("BOPPage");
    settings.setValue("header", tableWidget->horizontalHeader()->saveState());
    settings.endGroup();
    settings.endGroup();
  }
}

void BOPCheckPage::basicCheckFailedSlot()
{
  QLabel *label = new QLabel(this);
  label->setText(tr("BOP check unavailble when basic check failed"));
  QHBoxLayout *hLayout = new QHBoxLayout();
  hLayout->addStretch();
  hLayout->addWidget(label);
  hLayout->addStretch();
  QVBoxLayout *vLayout = new QVBoxLayout();
  vLayout->addLayout(hLayout);
  this->setLayout(vLayout);
}

void BOPCheckPage::basicCheckPassedSlot()
{
  QPushButton *goButton = new QPushButton(tr("Launch BOP check"), this);
  QHBoxLayout *hLayout = new QHBoxLayout();
  hLayout->addStretch();
  hLayout->addWidget(goButton);
  hLayout->addStretch();
  QVBoxLayout *vLayout = new QVBoxLayout();
  vLayout->addLayout(hLayout);
  this->setLayout(vLayout);
  connect(goButton, SIGNAL(clicked()), goButton, SLOT(hide()));
  connect(goButton, SIGNAL(clicked()), this, SLOT(goSlot()));
}

void BOPCheckPage::hideEvent(QHideEvent *event)
{
  if (tableWidget)
    tableWidget->clearSelection();
  QWidget::hideEvent(event);
}

void BOPCheckPage::goSlot()
{
  //todo launch bopalgo check in another process.
  
  
  delete (this->layout());
  tableWidget = new QTableWidget(this);
  tableWidget->setColumnCount(3);
  tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
  tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
  tableWidget->setHorizontalHeaderLabels
  (
    QStringList
    {
      tr("Id"),
      tr("Type"),
      tr("Error")
    }
  );
  QVBoxLayout *vLayout = new QVBoxLayout();
  vLayout->addWidget(tableWidget);
  this->setLayout(vLayout);
  
  QSettings &settings = static_cast<app::Application*>(qApp)->getUserSettings();
  settings.beginGroup("CheckGeometry");
  settings.beginGroup("BOPPage");
  tableWidget->horizontalHeader()->restoreState(settings.value("header").toByteArray());
  settings.endGroup();
  settings.endGroup();
  
  //I don't why we need to make a copy, but it doesn't work without it.
  //BRepAlgoAPI_Check also makes a copy of the shape.
  ann::SeerShape workCopy = seerShape.createWorkCopy();
  BOPAlgo_ArgumentAnalyzer BOPCheck;
  //   BOPCheck.StopOnFirstFaulty() = true; //this doesn't run any faster but gives us less results.
  BOPCheck.SetParallelMode(true); //this doesn't help for speed right now(occt 6.9.1).
  BOPCheck.SetShape1(workCopy.getRootOCCTShape());
  BOPCheck.ArgumentTypeMode() = true;
  BOPCheck.SelfInterMode() = true;
  BOPCheck.SmallEdgeMode() = true;
  BOPCheck.RebuildFaceMode() = true;
  BOPCheck.ContinuityMode() = true;
  BOPCheck.TangentMode() = true;
  BOPCheck.MergeVertexMode() = true;
  BOPCheck.CurveOnSurfaceMode() = true;
  BOPCheck.MergeEdgeMode() = true;
  
  QApplication::setOverrideCursor(Qt::WaitCursor);
  QApplication::processEvents(); //so wait cursor shows.
  //probably need try catch here.
  BOPCheck.Perform();
  QApplication::restoreOverrideCursor();
  
  const BOPAlgo_ListOfCheckResult &BOPResults = BOPCheck.GetCheckResult();
  std::vector<QTableWidgetItem> items;
  for (const auto &result : BOPResults)
  {
    for (const auto &resultShape : result.GetFaultyShapes1())
    {
      uuid resultId = workCopy.findShapeIdRecord(resultShape).id;
      std::vector<uuid> sourceIds = workCopy.devolve(resultId);
      assert(sourceIds.size() == 1);
      uuid sourceId = sourceIds.front();
      items.push_back(QTableWidgetItem(QString::fromStdString(gu::idToString(sourceId))));
      items.push_back(QTableWidgetItem(QString::fromStdString
        (shapeStrings.at(seerShape.getOCCTShape(sourceId).ShapeType()))));
      items.push_back(QTableWidgetItem(BOPCheckStatusToString(result.GetCheckStatus())));
    }
  }
  
  assert((items.size() % 3) == 0);
  tableWidget->setRowCount(items.size() / 3 + 1);
  
  QString overallStatus;
  if (!BOPCheck.HasFaulty())
    overallStatus = tr("Valid");
  else
    overallStatus = tr("Invalid");
  tableWidget->setItem(0, 0, new QTableWidgetItem
    (QString::fromStdString(gu::idToString(seerShape.getRootShapeId()))));
  tableWidget->setItem(0, 1, new QTableWidgetItem
    (QString::fromStdString(shapeStrings.at(seerShape.getRootOCCTShape().ShapeType()))));
  tableWidget->setItem(0, 2, new QTableWidgetItem(overallStatus));
  
  auto it = items.begin();
  for (std::size_t row = 0; row < (items.size() / 3); ++row)
  {
    for (std::size_t column = 0; column < 3; ++column)
    {
      tableWidget->setItem(row + 1, column, new QTableWidgetItem(*it));
      it++;
    }
  }
  
  observer = std::move(std::unique_ptr<msg::Observer>(new msg::Observer()));
  observer->name = "dlg::BOPCheckPage";
  connect(tableWidget, SIGNAL(itemSelectionChanged()), this, SLOT(selectionChangedSlot()));
}

void BOPCheckPage::selectionChangedSlot()
{
  if (boundingSphere.valid()) //remove current boundingSphere.
  {
    assert(boundingSphere->getParents().size() == 1);
    osg::Group *parent = boundingSphere->getParent(0);
    parent->removeChild(boundingSphere.get()); //this should make boundingSphere invalid.
  }
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  //get the fresh id.
  QList<QTableWidgetItem*> freshSelections = tableWidget->selectedItems();
  if (freshSelections.empty())
    return;
  uuid id = gu::stringToId(freshSelections.at(0)->data(Qt::DisplayRole).toString().toStdString());
  assert(!id.is_nil());
  assert(seerShape.hasShapeIdRecord(id));
  
  slc::Message sMessage;
  sMessage.type = slc::convert(seerShape.getOCCTShape(id).ShapeType());
  sMessage.featureId = feature.getId();
  sMessage.featureType = feature.getType();
  sMessage.shapeId = id;
  
  osg::BoundingSphered bSphere;
  //if vertex convert to selection point and 'cheat' bounding sphere.
  if (seerShape.getOCCTShape(id).ShapeType() == TopAbs_VERTEX)
  {
    if (!convertVertexSelection(seerShape, sMessage)) //converts vertex id to edge id
    {
      std::cout << "vertex to selection failed: ToleranceCheckPage::selectionChangedSlot" << std::endl;
      return;
    }
    //just make bounding sphere a percentage of whole shape. what if whole shape is vertex?
    osg::Vec3d vertexPosition = gu::toOsg(BRep_Tool::Pnt(TopoDS::Vertex(seerShape.getOCCTShape(id))));
    bSphere = minBoundingSphere;
    bSphere.center() = vertexPosition;
    sMessage.pointLocation = vertexPosition;
  }
  else
  {
    bSphere = calculateBoundingSphere(seerShape.getOCCTShape(id));
    bSphere.radius() = std::max(bSphere.radius(), minBoundingSphere.radius());
  }
  
  //select the geometry.
  msg::Message message(msg::Request | msg::Selection | msg::Add);
  message.payload = sMessage;
  observer->out(message);
  
  if (!bSphere.valid())
  {
    observer->out(msg::buildStatusMessage("Unable to calculate bounding sphere"));
    return;
  }
  
  boundingSphere = buildBoundingSphere(bSphere);
  if (feature.getOverlaySwitch()->addChild(boundingSphere.get()))
    feature.getOverlaySwitch()->setValue(feature.getOverlaySwitch()->getNumChildren() - 1, true);
}

ToleranceCheckPage::ToleranceCheckPage(const ftr::Base &featureIn, QWidget *parent) :
  CheckPageBase(featureIn, parent)
{
  observer->name = "dlg::ToleranceCheckPage";
  
  buildGui();
  
  QSettings &settings = static_cast<app::Application*>(qApp)->getUserSettings();
  settings.beginGroup("CheckGeometry");
  settings.beginGroup("TolerancePage");
  tableWidget->horizontalHeader()->restoreState(settings.value("header").toByteArray());
  settings.endGroup();
  settings.endGroup();
  
  connect(tableWidget, SIGNAL(itemSelectionChanged()), this, SLOT(selectionChangedSlot()));
}

ToleranceCheckPage::~ToleranceCheckPage()
{
  QSettings &settings = static_cast<app::Application*>(qApp)->getUserSettings();
  settings.beginGroup("CheckGeometry");
  settings.beginGroup("TolerancePage");
  settings.setValue("header", tableWidget->horizontalHeader()->saveState());
  settings.endGroup();
  settings.endGroup();
}

void ToleranceCheckPage::buildGui()
{
  tableWidget = new QTableWidget(this);
  QVBoxLayout *layout = new QVBoxLayout();
  layout->addWidget(tableWidget);
  this->setLayout(layout);
  
  tableWidget->setColumnCount(3);
  tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
  tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
  tableWidget->setHorizontalHeaderLabels
  (
    QStringList
    {
      tr("Id"),
      tr("Type"),
      tr("Tolerance")
    }
  );
}

void ToleranceCheckPage::hideEvent(QHideEvent *event)
{
  tableWidget->clearSelection();
  QWidget::hideEvent(event);
}

void ToleranceCheckPage::go()
{
  tableWidget->clearContents();
  tableWidget->setSortingEnabled(false);
  
  std::vector<uuid> ids = seerShape.getAllShapeIds();
  
  //tablewidget has to have row size set before adding items. we are only adding
  //face edges and vertices, so we need to filter down ids so we can set the
  //row count before filling in values.
  std::vector<uuid> filteredIds;
  for (const auto &id : ids)
  {
    TopAbs_ShapeEnum shapeType = seerShape.getOCCTShape(id).ShapeType();
    if
    (
      (shapeType == TopAbs_FACE) ||
      (shapeType == TopAbs_EDGE) ||
      (shapeType == TopAbs_VERTEX)
    )
      filteredIds.push_back(id);
  }
  
  tableWidget->setRowCount(filteredIds.size());
  int row = 0;
  for (const auto &id : filteredIds)
  {
    TopAbs_ShapeEnum shapeType = seerShape.getOCCTShape(id).ShapeType();
    double tolerance = 0.0;
    if (shapeType == TopAbs_FACE)
      tolerance = BRep_Tool::Tolerance(TopoDS::Face(seerShape.getOCCTShape(id)));
    else if (shapeType == TopAbs_EDGE)
      tolerance = BRep_Tool::Tolerance(TopoDS::Edge(seerShape.getOCCTShape(id)));
    else if (shapeType == TopAbs_VERTEX)
      tolerance = BRep_Tool::Tolerance(TopoDS::Vertex(seerShape.getOCCTShape(id)));
    else
      continue;
    
    tableWidget->setItem(row, 0, new QTableWidgetItem
      (QString::fromStdString(gu::idToString(id))));
    tableWidget->setItem(row, 1, new QTableWidgetItem
      (QString::fromStdString(shapeStrings.at(seerShape.getOCCTShape(id).ShapeType()))));
    tableWidget->setItem(row, 2, new QTableWidgetItem
      (QString::number(tolerance, 'f', 7))); //sorting doesn't work with sci notation.
    row++;
  }
  
  tableWidget->setSortingEnabled(true);
  tableWidget->sortItems(2, Qt::DescendingOrder);
}

void ToleranceCheckPage::selectionChangedSlot()
{
  if (boundingSphere.valid()) //remove current boundingSphere.
  {
    assert(boundingSphere->getParents().size() == 1);
    osg::Group *parent = boundingSphere->getParent(0);
    parent->removeChild(boundingSphere.get()); //this should make boundingSphere invalid.
  }
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  //get the fresh id.
  QList<QTableWidgetItem*> freshSelections = tableWidget->selectedItems();
  if (freshSelections.empty())
    return;
  uuid id = gu::stringToId(freshSelections.at(0)->data(Qt::DisplayRole).toString().toStdString());
  assert(!id.is_nil());
  assert(seerShape.hasShapeIdRecord(id));
  
  slc::Message sMessage;
  sMessage.type = slc::convert(seerShape.getOCCTShape(id).ShapeType());
  sMessage.featureId = feature.getId();
  sMessage.featureType = feature.getType();
  sMessage.shapeId = id;
  
  osg::BoundingSphered bSphere;
  //if vertex convert to selection point and 'cheat' bounding sphere.
  if (seerShape.getOCCTShape(id).ShapeType() == TopAbs_VERTEX)
  {
    if (!convertVertexSelection(seerShape, sMessage)) //converts vertex id to edge id
    {
      std::cout << "vertex to selection failed: ToleranceCheckPage::selectionChangedSlot" << std::endl;
      return;
    }
    //just make bounding sphere a percentage of whole shape. what if whole shape is vertex?
    osg::Vec3d vertexPosition = gu::toOsg(BRep_Tool::Pnt(TopoDS::Vertex(seerShape.getOCCTShape(id))));
    bSphere = minBoundingSphere;
    bSphere.center() = vertexPosition;
    sMessage.pointLocation = vertexPosition;
  }
  else
  {
    bSphere = calculateBoundingSphere(seerShape.getOCCTShape(id));
    bSphere.radius() = std::max(bSphere.radius(), minBoundingSphere.radius());
  }
  
  //select the geometry.
  msg::Message message(msg::Request | msg::Selection | msg::Add);
  message.payload = sMessage;
  observer->out(message);
  
  if (!bSphere.valid())
  {
    observer->out(msg::buildStatusMessage("Unable to calculate bounding sphere"));
    return;
  }
  
  boundingSphere = buildBoundingSphere(bSphere);
  if (feature.getOverlaySwitch()->addChild(boundingSphere.get()))
    feature.getOverlaySwitch()->setValue(feature.getOverlaySwitch()->getNumChildren() - 1, true);
}

ShapesPage::ShapesPage(const ftr::Base &featureIn, QWidget *parent) :
  CheckPageBase(featureIn, parent)
{
  observer->name = "dlg::ShapesPage"; //not using at this time.
  buildGui();
}

void ShapesPage::buildGui()
{
  QHBoxLayout *layout = new QHBoxLayout();
  
  QVBoxLayout *tl = new QVBoxLayout();
  tl->addWidget(new QLabel(tr("Content"), this));
  textEdit = new QTextEdit(this);
  textEdit->setReadOnly(true);
  tl->addWidget(textEdit);
  tl->addStretch();
  layout->addLayout(tl);
  
  QVBoxLayout *bl = new QVBoxLayout();
  bl->addWidget(new QLabel(tr("Boundaries"), this));
  boundaryTable = new QTableWidget(this);
  boundaryTable->setColumnCount(1);
  boundaryTable->setHorizontalHeaderLabels(QStringList{tr("Status")});
  boundaryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  boundaryTable->setSelectionMode(QAbstractItemView::SingleSelection);
  bl->addWidget(boundaryTable);
  bl->addStretch();
  layout->addLayout(bl);
  
  connect(boundaryTable, &QTableWidget::itemSelectionChanged, this, &ShapesPage::boundaryItemChangedSlot);
  
  this->setLayout(layout);
}

void ShapesPage::go()
{
  std::ostringstream stream;
  BRepTools_ShapeSet set;
  set.Add(seerShape.getRootOCCTShape());
  set.DumpExtent(stream);
  textEdit->setText(QString::fromStdString(stream.str()));
  
  auto processWire = [&](const TopoDS_Wire &w)
  {
    //these are wires only in the sense of a collection of edges.
    //the wires don't actually exist inside the seershape.
    TopTools_IndexedMapOfShape map;
    TopExp::MapShapes(w, TopAbs_EDGE, map);
    occt::EdgeVector ev = occt::ShapeVectorCast(map);
    std::vector<uuid> edges;
    for (const auto &e : ev)
    {
      if (!seerShape.hasShapeIdRecord(e))
      {
        std::cerr << "WARNING: skipping edge in ShapesPage::go()" << std::endl;
        continue;
      }
      edges.push_back(seerShape.findShapeIdRecord(e).id);
    }
    gu::uniquefy(edges);
    boundaries.push_back(edges);
  };
  
  occt::WireVector closed, open;
  std::tie(closed, open) = occt::getBoundaryWires(seerShape.getRootOCCTShape());
  boundaryTable->setRowCount(open.size() + closed.size());
  int row = 0;
  for (const auto &w : closed)
  {
    processWire(w);
    boundaryTable->setItem(row, 0, new QTableWidgetItem(tr("Closed")));
    row++;
  }
  for (const auto &w : open)
  {
    processWire(w);
    boundaryTable->setItem(row, 0, new QTableWidgetItem(tr("Open")));
    row++;
  }
}

void ShapesPage::boundaryItemChangedSlot()
{
  //remove previous bounding sphere and clear selection.
  if (boundingSphere.valid()) //remove current boundingSphere.
  {
    assert(boundingSphere->getParents().size() == 1);
    osg::Group *parent = boundingSphere->getParent(0);
    parent->removeChild(boundingSphere.get()); //this should make boundingSphere invalid.
  }
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  QList<QTableWidgetItem*> selected = boundaryTable->selectedItems();
  if (selected.isEmpty())
    return;
  int row = selected.front()->row();
  assert((row >= 0) && (row < static_cast<int>(boundaries.size())));
  const std::vector<uuid> &eids = boundaries.at(row);
  occt::ShapeVector ess;
  for (const auto &e : eids)
  {
    if (!seerShape.hasShapeIdRecord(e))
    {
      std::cerr << "WARNING: skipping edge in ShapesPage::boundaryItemChangedSlot()" << std::endl;
      continue;
    }
    slc::Message sMessage;
    sMessage.type = slc::Type::Edge;
    sMessage.featureId = feature.getId();
    sMessage.featureType = feature.getType();
    sMessage.shapeId = seerShape.findShapeIdRecord(e).id;
    msg::Message message(msg::Request | msg::Selection | msg::Add);
    message.payload = sMessage;
    observer->out(message);
    
    ess.push_back(seerShape.findShapeIdRecord(e).shape);
  }
  TopoDS_Compound c = occt::ShapeVectorCast(ess);
  osg::BoundingSphered bs = calculateBoundingSphere(c);
  bs.radius() = std::max(bs.radius(), minBoundingSphere.radius());
  if (!bs.valid())
  {
    observer->out(msg::buildStatusMessage("Unable to calculate bounding sphere"));
    return;
  }
  boundingSphere = buildBoundingSphere(bs);
  if (feature.getOverlaySwitch()->addChild(boundingSphere.get()))
    feature.getOverlaySwitch()->setValue(feature.getOverlaySwitch()->getNumChildren() - 1, true);
}

void ShapesPage::hideEvent(QHideEvent *event)
{
  boundaryTable->clearSelection();
  QWidget::hideEvent(event);
}

CheckGeometry::CheckGeometry(const ftr::Base &featureIn, QWidget *parent) :
  QDialog(parent), feature(featureIn)
{
  this->setWindowTitle(tr("Check Geometry"));
  
  observer = std::move(std::unique_ptr<msg::Observer>(new msg::Observer()));
  observer->name = "dlg::CheckGeometry";
  setupDispatcher();
  
  buildGui();
  
  WidgetGeometry *filter = new WidgetGeometry(this, "dlg::CheckGeometry");
  this->installEventFilter(filter);
}

CheckGeometry::~CheckGeometry(){}

void CheckGeometry::closeEvent(QCloseEvent *e)
{
  QDialog::closeEvent(e);
  observer->out(msg::Mask(msg::Request | msg::Command | msg::Done));
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
  connect(basicCheckPage, SIGNAL(basicCheckPassed()), bopCheckPage, SLOT(basicCheckPassedSlot()));
  connect(basicCheckPage, SIGNAL(basicCheckFailed()), bopCheckPage, SLOT(basicCheckFailedSlot()));
  
  toleranceCheckPage = new ToleranceCheckPage(feature, this);
  tabWidget->addTab(toleranceCheckPage, tr("Tolerance"));
  
  shapesPage = new ShapesPage(feature, this);
  tabWidget->addTab(shapesPage, tr("Shapes"));
}

void CheckGeometry::go()
{
  //we don't call go for bop page because it is so slow.
  //we make the user launch it.
  basicCheckPage->go();
  toleranceCheckPage->go();
  shapesPage->go();
}

void CheckGeometry::setupDispatcher()
{
  msg::Mask mask;
  
  mask = msg::Response | msg::Pre | msg::Remove | msg::Feature;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind
    (&CheckGeometry::featureRemovedDispatched, this, _1)));

  mask = msg::Response | msg::Feature | msg::Status;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind
    (&CheckGeometry::featureStateChangedDispatched, this, _1)));
}

void CheckGeometry::featureRemovedDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  prj::Message message = boost::get<prj::Message>(messageIn.payload);
  
  if
  (
    (message.featureIds.size() == 1)
    && (message.featureIds.front() == feature.getId())
  )
    qApp->postEvent(this, new QCloseEvent());
}

void CheckGeometry::featureStateChangedDispatched(const msg::Message &messageIn)
{
  ftr::Message fMessage = boost::get<ftr::Message>(messageIn.payload);
  if (fMessage.featureId == feature.getId())
    qApp->postEvent(this, new QCloseEvent());
}

/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2015  Thomas S. Anderson blobfish.at.gmx.com
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

#include "dagmodelgraph.h"

using namespace DAG;

VertexProperty::VertexProperty() :
  feature(),
  featureId(),
  columnMask(),
  row(0),
  sortedIndex(0),
  dagVisible(true),
  
  rectShared(new RectItem()),
  rectRaw(rectShared.get()),
  pointShared(new QGraphicsEllipseItem()), 
  pointRaw(pointShared.get()),
  visibleIconShared(new QGraphicsPixmapItem()),
  visibleIconRaw(visibleIconShared.get()),
  stateIconShared(new QGraphicsPixmapItem()),
  stateIconRaw(stateIconShared.get()),
  featureIconShared(new QGraphicsPixmapItem()),
  featureIconRaw(featureIconShared.get()),
  textShared(new QGraphicsTextItem()),
  textRaw(textShared.get())
{
  //set z values.
  rectRaw->setZValue(-1000.0);
  pointRaw->setZValue(1000.0);
  visibleIconRaw->setZValue(0.0);
  stateIconRaw->setZValue(0.0);
  featureIconRaw->setZValue(0.0);
  textRaw->setZValue(0.0);
  
  //assign data to recognize later.
  rectRaw->setData(QtData::key, QtData::rectangle);
  pointRaw->setData(QtData::key, QtData::point);
  visibleIconRaw->setData(QtData::key, QtData::visibleIcon);
  stateIconRaw->setData(QtData::key, QtData::stateIcon);
  featureIconRaw->setData(QtData::key, QtData::featureIcon);
  textRaw->setData(QtData::key, QtData::text);
}

EdgeProperty::EdgeProperty() : connector(new QGraphicsPathItem())
{
  connector->setZValue(0.0);
  
  //maybe senseless as right now. we are not searching for connectors.
  connector->setData(QtData::key, QtData::connector);
}

const VertexProperty& DAG::findRecordByVisible(const GraphLinkContainer &containerIn, QGraphicsPixmapItem *itemIn)
{
  typedef GraphLinkContainer::index<VertexProperty::ByVisibleIcon>::type List;
  const List &list = containerIn.get<VertexProperty::ByVisibleIcon>();
  List::const_iterator it = list.find(itemIn);
  assert(it != list.end());
  return *it;
}

// const GraphLinkRecord& Gui::DAG::findRecord(Vertex vertexIn, const GraphLinkContainer &containerIn)
// {
//   typedef GraphLinkContainer::index<GraphLinkRecord::ByVertex>::type List;
//   const List &list = containerIn.get<GraphLinkRecord::ByVertex>();
//   List::const_iterator it = list.find(vertexIn);
//   assert(it != list.end());
//   return *it;
// }
// 
// const GraphLinkRecord& Gui::DAG::findRecord(const App::DocumentObject* dObjectIn, const GraphLinkContainer &containerIn)
// {
//   typedef GraphLinkContainer::index<GraphLinkRecord::ByDObject>::type List;
//   const List &list = containerIn.get<GraphLinkRecord::ByDObject>();
//   List::const_iterator it = list.find(dObjectIn);
//   assert(it != list.end());
//   return *it;
// }
// 
// const GraphLinkRecord& Gui::DAG::findRecord(const ViewProviderDocumentObject* VPDObjectIn, const GraphLinkContainer &containerIn)
// {
//   typedef GraphLinkContainer::index<GraphLinkRecord::ByVPDObject>::type List;
//   const List &list = containerIn.get<GraphLinkRecord::ByVPDObject>();
//   List::const_iterator it = list.find(VPDObjectIn);
//   assert(it != list.end());
//   return *it;
// }
// 
// const GraphLinkRecord& Gui::DAG::findRecord(const RectItem* rectIn, const GraphLinkContainer &containerIn)
// {
//   typedef GraphLinkContainer::index<GraphLinkRecord::ByRectItem>::type List;
//   const List &list = containerIn.get<GraphLinkRecord::ByRectItem>();
//   List::const_iterator it = list.find(rectIn);
//   assert(it != list.end());
//   return *it;
// }
// 
// const GraphLinkRecord& Gui::DAG::findRecord(const std::string &stringIn, const GraphLinkContainer &containerIn)
// {
//   typedef GraphLinkContainer::index<GraphLinkRecord::ByUniqueName>::type List;
//   const List &list = containerIn.get<GraphLinkRecord::ByUniqueName>();
//   List::const_iterator it = list.find(stringIn);
//   assert(it != list.end());
//   return *it;
// }
// 
// void Gui::DAG::eraseRecord(const ViewProviderDocumentObject* VPDObjectIn, GraphLinkContainer &containerIn)
// {
//   typedef GraphLinkContainer::index<GraphLinkRecord::ByVPDObject>::type List;
//   const List &list = containerIn.get<GraphLinkRecord::ByVPDObject>();
//   List::iterator it = list.find(VPDObjectIn);
//   assert(it != list.end());
//   containerIn.get<GraphLinkRecord::ByVPDObject>().erase(it);
// }
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

#include <boost/graph/breadth_first_search.hpp>

#include <QTextStream>

#include <tools/idtools.h>
#include <tools/graphtools.h>
#include <feature/states.h>
#include <dagview/stow.h>

using namespace dag;


VertexProperty::VertexProperty() :
featureId(gu::createNilId()),
columnMask(),
row(0),
sortedIndex(0),
state(),
dagVisible(true),
alive(true),
hasSeerShape(false),
rectShared(new RectItem()),
pointShared(new QGraphicsEllipseItem()), 
visibleIconShared(new QGraphicsPixmapItem()),
overlayIconShared(new QGraphicsPixmapItem()),
stateIconShared(new QGraphicsPixmapItem()),
featureIconShared(new QGraphicsPixmapItem()),
textShared(new QGraphicsTextItem())
{
  //set z values.
  rectShared->setZValue(-1000.0);
  pointShared->setZValue(1000.0);
  visibleIconShared->setZValue(0.0);
  overlayIconShared->setZValue(0.0);
  stateIconShared->setZValue(0.0);
  featureIconShared->setZValue(0.0);
  textShared->setZValue(0.0);
  
  //assign data to recognize later.
  rectShared->setData(qtd::key, qtd::rectangle);
  pointShared->setData(qtd::key, qtd::point);
  visibleIconShared->setData(qtd::key, qtd::visibleIcon);
  overlayIconShared->setData(qtd::key, qtd::overlayIcon);
  stateIconShared->setData(qtd::key, qtd::stateIcon);
  featureIconShared->setData(qtd::key, qtd::featureIcon);
  textShared->setData(qtd::key, qtd::text);
  
  pointShared->setFlag(QGraphicsItem::ItemIsMovable, true);
}

EdgeProperty::EdgeProperty() : connector(new QGraphicsPathItem()), inputType()
{
  connector->setZValue(0.0);
  //maybe senseless as right now. we are not searching for connectors.
  connector->setData(qtd::key, qtd::connector);
}
  
Stow::Stow(){}
Stow::~Stow(){}

Vertex Stow::findVertex(const boost::uuids::uuid &idIn)
{
  for (auto its = boost::vertices(graph); its.first != its.second; ++its.first)
  {
    if (graph[*its.first].featureId == idIn)
    {
      assert(graph[*its.first].alive);
      return *its.first;
    }
  }
  assert(0); //no vertex with id in prj::Stow::findVertex
  std::cout << "warning: no vertex with id in prj::Stow::findVertex" << std::endl;
  return NullVertex();
}

Vertex Stow::findVertex(const RectItem *iIn)
{
  for (auto its = boost::vertices(graph); its.first != its.second; ++its.first)
  {
    if (graph[*its.first].rectShared.get() == iIn)
    {
      assert(graph[*its.first].alive);
      return *its.first;
    }
  }
  assert(0); //no vertex with id in prj::Stow::findVertex
  std::cout << "warning: no vertex with RectItem in prj::Stow::findVertex" << std::endl;
  return NullVertex();
}

Vertex Stow::findVertex(const QGraphicsEllipseItem *eIn)
{
  for (auto its = boost::vertices(graph); its.first != its.second; ++its.first)
  {
    if (graph[*its.first].pointShared.get() == eIn)
    {
      assert(graph[*its.first].alive);
      return *its.first;
    }
  }
  assert(0); //no vertex with id in prj::Stow::findVertex
  std::cout << "warning: no vertex with pointIcon in prj::Stow::findVertex" << std::endl;
  return NullVertex();
}

Vertex Stow::findVisibleVertex(const QGraphicsPixmapItem *iIn)
{
  for (auto its = boost::vertices(graph); its.first != its.second; ++its.first)
  {
    if (graph[*its.first].visibleIconShared.get() == iIn)
    {
      assert(graph[*its.first].alive);
      return *its.first;
    }
  }
  assert(0); //no vertex with id in prj::Stow::findVertex
  std::cout << "warning: no vertex with visibleIcon in prj::Stow::findVisibleVertex" << std::endl;
  return NullVertex();
}

Vertex Stow::findOverlayVertex(const QGraphicsPixmapItem *iIn)
{
  for (auto its = boost::vertices(graph); its.first != its.second; ++its.first)
  {
    if (graph[*its.first].overlayIconShared.get() == iIn)
    {
      assert(graph[*its.first].alive);
      return *its.first;
    }
  }
  assert(0); //no vertex with id in prj::Stow::findVertex
  std::cout << "warning: no vertex with visibleIcon in prj::Stow::findVisibleVertex" << std::endl;
  return NullVertex();
}

std::vector<Vertex> Stow::getAllSelected()
{
  std::vector<Vertex> out;
  for (auto its = boost::vertices(graph); its.first != its.second; ++its.first)
  {
    if (graph[*its.first].rectShared->isSelected())
      out.push_back(*its.first);
  }
  
  return out;
}

std::vector<QGraphicsItem*> Stow::getAllSceneItems(Vertex v)
{
  std::vector<QGraphicsItem*> out;
  
  out.push_back(graph[v].rectShared.get());
  out.push_back(graph[v].pointShared.get());
  out.push_back(graph[v].visibleIconShared.get());
  out.push_back(graph[v].overlayIconShared.get());
  out.push_back(graph[v].stateIconShared.get());
  out.push_back(graph[v].featureIconShared.get());
  out.push_back(graph[v].textShared.get());
  
  return out;
}

std::vector<boost::uuids::uuid> Stow::getDropAccepted(Vertex v)
{
  //start out real simple and just grab connected vertices.
  
  std::vector<Vertex> vs;
  gu::BFSLimitVisitor<Vertex> lv1(vs);
  boost::breadth_first_search(graph, v, boost::visitor(lv1));
  
  GraphReversed rGraph = boost::make_reverse_graph(graph);
  gu::BFSLimitVisitor<Vertex> lv2(vs);
  boost::breadth_first_search(rGraph, v, boost::visitor(lv2));
  
  //remove the passed in vertex. Should be in there twice.
  std::vector<Vertex>::iterator it = std::find(vs.begin(), vs.end(), v);
  while (it != vs.end())
  {
    vs.erase(it);
    it = std::find(vs.begin(), vs.end(), v);
  }
  
  std::vector<boost::uuids::uuid> out;
  for (const auto &vert : vs)
    out.push_back(graph[vert].featureId);
  
  return out;
}

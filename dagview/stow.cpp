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
#include <QPen>

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

Edge Stow::findEdge(const QGraphicsPathItem *pi) //path item
{
  for (auto its = boost::edges(graph); its.first != its.second; ++its.first)
  {
    if (graph[*its.first].connector.get() == pi)
      return *its.first;
  }
  assert(0); //couldn't find edge.
  return Edge(); //what to do here?
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

std::pair<std::vector<Vertex>, std::vector<Edge>> Stow::getDropAccepted(Vertex v)
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
  
  //filter to get edges.
  gu::SubsetFilter<Graph> vertexFilter(graph, vs);
  typedef boost::filtered_graph<Graph, boost::keep_all, gu::SubsetFilter<Graph> > FilteredGraph;
  FilteredGraph filteredGraph(graph, boost::keep_all(), vertexFilter);
  
  std::vector<Edge> es; //edges
  for (auto its = boost::edges(filteredGraph); its.first != its.second; ++its.first)
    es.push_back(*its.first);
  
  return std::make_pair(vs, es);
}

float Stow::connectionOffset(Vertex v, Edge e)
{
  //incoming edges always come from the top, so this works good for that.
  //outgoing edges can go out left, right or the bottom, so this doesn't work good for that. leaving for now.
  
  int edgeIndex = -1; // -1 equals not found.
  int indexCount = 0;
  
  int edgeCount = boost::in_degree(v, graph);
  for (auto its = boost::in_edges(v, graph); its.first != its.second; ++its.first)
  {
    if (*its.first == e)
    {
      edgeIndex = indexCount;
      break;
    }
    indexCount++;
  }
  
  if (edgeIndex == -1)
  {
    edgeCount = boost::out_degree(v, graph);
    indexCount = 0;
    for (auto its = boost::out_edges(v, graph); its.first != its.second; ++its.first)
    {
      if (*its.first == e)
      {
        edgeIndex = indexCount;
        break;
      }
      indexCount++;
    }
  }
  
  assert(edgeCount != 0);
  assert(edgeIndex < edgeCount);
  if
  (
    (edgeIndex < 0) //couldn't find edge
    || (edgeCount < 2) //1 edge, return center. 0 edges, WTF
    || (edgeIndex >= edgeCount) //WTF
  )
    return 0.0;
    
  //2 or more edges.
  float nos = static_cast<float>(edgeCount) - 1.0; //number of spaces.
  float s = 2.0 / nos; //space between edges.
  
  return -1.0 + (s * static_cast<float>(edgeIndex));
  
}

void Stow::highlightConnectorOn(Edge e, const QColor &colorIn)
{
  highlightConnectorOn(graph[e].connector.get(), colorIn);
}

void Stow::highlightConnectorOff(Edge e)
{
  highlightConnectorOff(graph[e].connector.get());
}

void Stow::highlightConnectorOn(QGraphicsPathItem *pi, const QColor &c)
{
  QPen pen(c);
  pen.setWidth(3.0);
  pi->setPen(pen);
  pi->setZValue(1.0);
}

void Stow::highlightConnectorOff(QGraphicsPathItem *pi)
{
  pi->setPen(QPen());
  pi->setZValue(0.0);
}

std::vector<Edge> Stow::getAllEdges(Vertex v)
{
  //is there really no function to get both in and out edges?
  std::vector<Edge> out;
  for (auto its = boost::out_edges(v, graph); its.first != its.second; ++its.first)
    out.push_back(*its.first);
  for (auto its = boost::in_edges(v, graph); its.first != its.second; ++its.first)
    out.push_back(*its.first);
  return out;
}

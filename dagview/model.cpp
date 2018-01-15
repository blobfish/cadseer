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

#include <iostream>
#include <cassert>

#include <boost/graph/topological_sort.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/variant.hpp>

#include <QString>
#include <QTextStream>
#include <QGraphicsTextItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsProxyWidget>
#include <QGraphicsView>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QPainter>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QMenu>
#include <QTimer>
#include <QUrl>
#include <QDesktopServices>

#include <application/application.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <project/project.h>
#include <globalutilities.h>
#include <tools/idtools.h>
#include <viewer/message.h>
#include <feature/base.h>
#include <message/dispatch.h>
#include <message/observer.h>
#include <dagview/controlleddfs.h>
#include <dagview/rectitem.h>
#include <dagview/stow.h>
#include <dagview/model.h>

using boost::uuids::uuid;

using namespace dag;

LineEdit::LineEdit(QWidget* parentIn): QLineEdit(parentIn)
{

}

void LineEdit::keyPressEvent(QKeyEvent *eventIn)
{
  if (eventIn->key() == Qt::Key_Escape)
  {
    Q_EMIT rejectedSignal();
    eventIn->accept();
    return;
  }
  if (
    (eventIn->key() == Qt::Key_Enter) ||
    (eventIn->key() == Qt::Key_Return)
  )
  {
    Q_EMIT acceptedSignal();
    eventIn->accept();
    return;
  }
  
  QLineEdit::keyPressEvent(eventIn);
}

void LineEdit::focusOutEvent(QFocusEvent *e)
{
  QLineEdit::focusOutEvent(e);
  
  Q_EMIT rejectedSignal();
}

//I dont think I should have to call invalidate
//and definitely not on the whole scene!
//if we have performance problems, this will definitely
//be something to re-visit. I am not wasting anymore time on
//this right now.
//   this->scene()->invalidate();
//   this->scene()->invalidate(this->sceneTransform().inverted().mapRect(this->boundingRect()));
//   update(boundingRect());
//note: I haven't tried this again since I turned BSP off.

namespace dag
{
  struct DragData
  {
    uuid featureId = gu::createNilId();
    std::vector<Vertex> acceptedVertices;
    std::vector<Edge> acceptedEdges;
    QGraphicsPathItem *lastHighlight = nullptr;
    bool isAcceptedEdge(const Edge &eIn)
    {
      return std::find(acceptedEdges.begin(), acceptedEdges.end(), eIn) != acceptedEdges.end();
    }
  };
}

Model::Model(QObject *parentIn) : QGraphicsScene(parentIn), stow(new Stow())
{
  observer = std::move(std::unique_ptr<msg::Observer>(new msg::Observer()));
  observer->name = "dag::Model";
  setupDispatcher();
  
  //turned off BSP as it was giving inconsistent discovery of items
  //underneath cursor.
  this->setItemIndexMethod(QGraphicsScene::NoIndex);
  setupViewConstants();
  
//   setupFilters();
//   
  currentPrehighlight = nullptr;
  
  QIcon temp(":/resources/images/dagViewVisible.svg");
  visiblePixmapEnabled = temp.pixmap(iconSize, iconSize, QIcon::Normal, QIcon::On);
  visiblePixmapDisabled = temp.pixmap(iconSize, iconSize, QIcon::Disabled, QIcon::Off);
  
  QIcon tempOverlay(":/resources/images/dagViewOverlay.svg");
  overlayPixmapEnabled = tempOverlay.pixmap(iconSize, iconSize, QIcon::Normal, QIcon::On);
  overlayPixmapDisabled = tempOverlay.pixmap(iconSize, iconSize, QIcon::Disabled, QIcon::Off);
  
  QIcon passIcon(":/resources/images/dagViewPass.svg");
  passPixmap = passIcon.pixmap(iconSize, iconSize);
  QIcon failIcon(":/resources/images/dagViewFail.svg");
  failPixmap = failIcon.pixmap(iconSize, iconSize);
  QIcon pendingIcon(":/resources/images/dagViewPending.svg");
  pendingPixmap = pendingIcon.pixmap(iconSize, iconSize);
  QIcon inactiveIcon(":/resources/images/dagViewInactive.svg");
  inactivePixmap = inactiveIcon.pixmap(iconSize, iconSize);
}

Model::~Model()
{
  //we used shared pointers in the graphics map to manage memory.
  //so we don't want qt deleting these objects. so remove from scene.
  removeAllItems();
}

void Model::setupViewConstants()
{
  //get direction. 1.0 = top to bottom.  -1.0 = bottom to top.
  direction = 1.0;
  if (direction != -1.0 && direction != 1.0)
    direction = 1.0;
  
  QFontMetrics fontMetric(this->font());
  fontHeight = fontMetric.height();
  verticalSpacing = 1.0;
  rowHeight = (fontHeight + 2.0 * verticalSpacing) * direction; //pixel space top and bottom.
  iconSize = fontHeight;
  pointSize = fontHeight * 0.6;
  pointSpacing = pointSize * 1.10;
  pointToIcon = iconSize;
  iconToIcon = iconSize * 0.25;
  iconToText = iconSize / 2.0;
  rowPadding = fontHeight;
  backgroundBrushes = {this->palette().base(), this->palette().alternateBase()};
  forgroundBrushes = 
  {
    QBrush(Qt::red),
    QBrush(Qt::darkRed),
    QBrush(Qt::green),
    QBrush(Qt::darkGreen),
    QBrush(Qt::blue),
    QBrush(Qt::darkBlue),
    QBrush(Qt::cyan),
    QBrush(Qt::darkCyan),
    QBrush(Qt::magenta),
    QBrush(Qt::darkMagenta),
//     QBrush(Qt::yellow), can't read
    QBrush(Qt::darkYellow),
    QBrush(Qt::gray),
    QBrush(Qt::darkGray),
    QBrush(Qt::lightGray)
  }; //reserve some of the these for highlight stuff.
}

void Model::featureAddedDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  prj::Message message = boost::get<prj::Message>(messageIn.payload);
  
  Vertex virginVertex = boost::add_vertex(stow->graph);
  stow->graph[virginVertex].featureId = message.feature->getId();
  stow->graph[virginVertex].state = message.feature->getState();
  stow->graph[virginVertex].hasSeerShape = message.feature->hasAnnex(ann::Type::SeerShape);
  
  if (message.feature->isVisible3D())
    stow->graph[virginVertex].visibleIconShared->setPixmap(visiblePixmapEnabled);
  else
    stow->graph[virginVertex].visibleIconShared->setPixmap(visiblePixmapDisabled);
  
  if (message.feature->isVisibleOverlay())
    stow->graph[virginVertex].overlayIconShared->setPixmap(overlayPixmapEnabled);
  else
    stow->graph[virginVertex].overlayIconShared->setPixmap(overlayPixmapDisabled);
  
  stow->graph[virginVertex].featureIconShared->setPixmap(message.feature->getIcon().pixmap(iconSize, iconSize));
  stow->graph[virginVertex].textShared->setPlainText(message.feature->getName());
  stow->graph[virginVertex].textShared->setFont(this->font());
  
  addItemsToScene(stow->getAllSceneItems(virginVertex));
  this->invalidate(); //temp.
}

void Model::featureRemovedDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  prj::Message message = boost::get<prj::Message>(messageIn.payload);
  
  Vertex vertex = stow->findVertex(message.feature->getId());
  if (vertex == NullVertex())
    return;
  removeItemsFromScene(stow->getAllSceneItems(vertex));
  //connections should already removed
  assert(boost::in_degree(vertex, stow->graph) == 0);
  assert(boost::out_degree(vertex, stow->graph) == 0);
  stow->graph[vertex].alive = false;
}


void Model::connectionAddedDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  prj::Message message = boost::get<prj::Message>(messageIn.payload);
  
  assert(message.featureIds.size() == 2);
  Vertex parentVertex = stow->findVertex(message.featureIds.front());
  Vertex childVertex = stow->findVertex(message.featureIds.back());
  if (parentVertex == NullVertex() || childVertex == NullVertex())
    return;
  
  bool results;
  Edge edge;
  
  std::tie(edge, results) = boost::edge(parentVertex, childVertex, stow->graph);
  if (results)
  {
    //connection already exists. just add input type and get out.
    stow->graph[edge].inputType += message.inputType;
    return;
  }
  
  std::tie(edge, results) = boost::add_edge(parentVertex, childVertex, stow->graph);
  assert(results);
  if (!results)
    return;
  stow->graph[edge].inputType = message.inputType;
  
  QPainterPath path;
  path.moveTo(0.0, 0.0);
  path.lineTo(0.0, 1.0);
  stow->graph[edge].connector->setPath(path);
  
  if (!stow->graph[edge].connector->scene())
    this->addItem(stow->graph[edge].connector.get());
}

void Model::connectionRemovedDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  prj::Message message = boost::get<prj::Message>(messageIn.payload);
  
  assert(message.featureIds.size() == 2);
  Vertex parentVertex = stow->findVertex(message.featureIds.front());
  Vertex childVertex = stow->findVertex(message.featureIds.back());
  if (parentVertex == NullVertex() || childVertex == NullVertex())
    return;
  
  bool results;
  Edge edge;
  std::tie(edge, results) = boost::edge(parentVertex, childVertex, stow->graph);
  assert(results);
  if (!results)
    return;
  
  if (stow->graph[edge].connector->scene())
    this->removeItem(stow->graph[edge].connector.get());
  boost::remove_edge(edge, stow->graph);
}

void Model::setupDispatcher()
{
  msg::Mask mask;
  
  mask = msg::Response | msg::Post | msg::Add | msg::Feature;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::featureAddedDispatched, this, _1)));
  
  mask = msg::Response | msg::Pre | msg::Remove | msg::Feature;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::featureRemovedDispatched, this, _1)));
  
  mask = msg::Response | msg::Post | msg::Add | msg::Connection;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::connectionAddedDispatched, this, _1)));
  
  mask = msg::Response | msg::Pre | msg::Remove | msg::Connection;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::connectionRemovedDispatched, this, _1)));
  
  mask = msg::Response | msg::Post | msg::Project | msg::Update | msg::Model;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::projectUpdatedDispatched, this, _1)));
  
  mask = msg::Response | msg::Post | msg::Preselection | msg::Add;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::preselectionAdditionDispatched, this, _1)));
  
  mask = msg::Response | msg::Pre | msg::Preselection | msg::Remove;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::preselectionSubtractionDispatched, this, _1)));
  
  mask = msg::Response | msg::Post | msg::Selection | msg::Add;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::selectionAdditionDispatched, this, _1)));
  
  mask = msg::Response | msg::Pre | msg::Selection | msg::Remove;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::selectionSubtractionDispatched, this, _1)));
  
  mask = msg::Response | msg::Pre | msg::Close | msg::Project;;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::closeProjectDispatched, this, _1)));
  
  mask = msg::Response | msg::Feature | msg::Status;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::featureStateChangedDispatched, this, _1)));
  
  mask = msg::Response | msg::Project | msg::Feature | msg::Status;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::projectFeatureStateChangedDispatched, this, _1)));
  
  mask = msg::Response | msg::Edit | msg::Feature | msg::Name;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::featureRenamedDispatched, this, _1)));
  
  mask = msg::Request | msg::DebugDumpDAGViewGraph;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::dumpDAGViewGraphDispatched, this, _1)));
  
  mask = msg::Response | msg::View | msg::Show | msg::ThreeD;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::threeDShowDispatched, this, _1)));
  
  mask = msg::Response | msg::View | msg::Hide | msg::ThreeD;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::threeDHideDispatched, this, _1)));
  
  mask = msg::Request | msg::DAG | msg::View | msg::Update;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::projectUpdatedDispatched, this, _1)));
  
  mask = msg::Response | msg::View | msg::Show | msg::Overlay;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::overlayShowDispatched, this, _1)));
  
  mask = msg::Response | msg::View | msg::Hide | msg::Overlay;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::overlayHideDispatched, this, _1)));
}

void Model::featureStateChangedDispatched(const msg::Message &messageIn)
{
  ftr::Message fMessage = boost::get<ftr::Message>(messageIn.payload);
  Vertex vertex = stow->findVertex(fMessage.featureId);
  if (vertex == NullVertex())
    return;
  
  //this is the feature state change from the actual feature.
  //so clear out the lower 3 bits and set to new state
  stow->graph[vertex].state &= ftr::State("11000");
  stow->graph[vertex].state |= (ftr::State("00111") & fMessage.state);
  
  stateUpdate(vertex);
}

void Model::projectFeatureStateChangedDispatched(const msg::Message &mIn)
{
  ftr::Message fMessage = boost::get<ftr::Message>(mIn.payload);
  Vertex vertex = stow->findVertex(fMessage.featureId);
  if (vertex == NullVertex())
    return;
  
  //this is the feature state change from the PROJECT.
  //so clear out the upper 2 bits and set to new state
  stow->graph[vertex].state &= ftr::State("00111");
  stow->graph[vertex].state |= (ftr::State("11000") & fMessage.state);
  
  stateUpdate(vertex);
}

void Model::stateUpdate(Vertex vIn)
{
  if (vIn == NullVertex())
    return;
  
  ftr::State cState = stow->graph[vIn].state;

  //from highest to lowest priority.
  if (cState.test(ftr::StateOffset::Inactive))
    stow->graph[vIn].stateIconShared->setPixmap(inactivePixmap);
  else if (cState.test(ftr::StateOffset::ModelDirty))
    stow->graph[vIn].stateIconShared->setPixmap(pendingPixmap);
  else if (cState.test(ftr::StateOffset::Failure))
    stow->graph[vIn].stateIconShared->setPixmap(failPixmap);
  else
    stow->graph[vIn].stateIconShared->setPixmap(passPixmap);

  if (cState.test(ftr::StateOffset::NonLeaf))
  {
    if (stow->graph[vIn].visibleIconShared->scene())
      removeItem(stow->graph[vIn].visibleIconShared.get());
  }
  else
  {
    if (!stow->graph[vIn].visibleIconShared->scene())
      addItem(stow->graph[vIn].visibleIconShared.get());
  }
  
  //set tool tip to current state.
  QString ts = tr("True");
  QString fs = tr("False");
  QString toolTip;
  QTextStream stream(&toolTip);
  stream <<
  "<table border=\"1\" cellpadding=\"6\">" << 
  "<tr>" <<
  "<td>" << tr("Model Dirty") << "</td><td>" << ((cState.test(ftr::StateOffset::ModelDirty)) ? ts : fs) << "</td>" <<
  "</tr><tr>" <<
  "<td>" << tr("Visual Dirty") << "</td><td>" << ((cState.test(ftr::StateOffset::VisualDirty)) ? ts : fs) << "</td>" <<
  "</tr><tr>" <<
  "<td>" << tr("Failure") << "</td><td>" << ((cState.test(ftr::StateOffset::Failure)) ? ts : fs) << "</td>" <<
  "</tr><tr>" <<
  "<td>" << tr("Inactive") << "</td><td>" << ((cState.test(ftr::StateOffset::Inactive)) ? ts : fs) << "</td>" <<
  "</tr><tr>" <<
  "<td>" << tr("Non-Leaf") << "</td><td>" << ((cState.test(ftr::StateOffset::NonLeaf)) ? ts : fs) << "</td>" <<
  "</tr>" <<
  "</table>";
  stow->graph[vIn].stateIconShared->setToolTip(toolTip);
}

void Model::featureRenamedDispatched(const msg::Message &messageIn)
{
  ftr::Message fMessage = boost::get<ftr::Message>(messageIn.payload);
  Vertex vertex = stow->findVertex(fMessage.featureId);
  if (vertex == NullVertex())
    return;
  stow->graph[vertex].textShared->setPlainText(fMessage.string);
}

void Model::preselectionAdditionDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  slc::Message sMessage = boost::get<slc::Message>(messageIn.payload);
  if (sMessage.type != slc::Type::Object)
    return;
  Vertex vertex = stow->findVertex(sMessage.featureId);
  if (vertex == NullVertex())
    return;
  assert(!currentPrehighlight); //trying to set prehighlight when something is already set.
  if (currentPrehighlight)
    return;
  currentPrehighlight = stow->graph[vertex].rectShared.get();
  currentPrehighlight->preHighlightOn();
  
  invalidate();
}

void Model::preselectionSubtractionDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  slc::Message sMessage = boost::get<slc::Message>(messageIn.payload);
  if (sMessage.type != slc::Type::Object)
    return;
  Vertex vertex = stow->findVertex(sMessage.featureId);
  if (vertex == NullVertex())
    return;
  assert(currentPrehighlight); //trying to clear prehighlight when already empty.
  if (!currentPrehighlight)
    return;
  stow->graph[vertex].rectShared->preHighlightOff();
  currentPrehighlight = nullptr;
  
  invalidate();
}

void Model::selectionAdditionDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  slc::Message sMessage = boost::get<slc::Message>(messageIn.payload);
  if (sMessage.type != slc::Type::Object)
    return;
  Vertex vertex = stow->findVertex(sMessage.featureId);
  if (vertex == NullVertex())
    return;
  stow->graph[vertex].rectShared->selectionOn();
  for (const auto &e : stow->getAllEdges(vertex))
    stow->highlightConnectorOn(e, stow->graph[vertex].textShared->defaultTextColor());
  
  lastPickValid = true;
  lastPick = stow->graph[vertex].rectShared->mapToScene(stow->graph[vertex].rectShared->rect().center());
  
  invalidate();
}

void Model::selectionSubtractionDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  slc::Message sMessage = boost::get<slc::Message>(messageIn.payload);
  if (sMessage.type != slc::Type::Object)
    return;
  Vertex vertex = stow->findVertex(sMessage.featureId);
  if (vertex == NullVertex())
    return;
  stow->graph[vertex].rectShared->selectionOff();
  for (const auto &e : stow->getAllEdges(vertex))
    stow->highlightConnectorOff(e);
  
  lastPickValid = false;
  
  invalidate();
}

void Model::closeProjectDispatched(const msg::Message&)
{
  removeAllItems();
  stow->graph.clear();
  invalidate();
}

void Model::projectUpdatedDispatched(const msg::Message &)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
//   auto dumpMask = [] (const ColumnMask& columnMaskIn)
//   {
//     //have to create a smaller subset to get through std::cout.
//     std::bitset<8> testSet;
//     for (unsigned int index = 0; index < testSet.size(); ++index)
//       testSet[index] = columnMaskIn[index];
//     std::cout << testSet.to_string() << std::endl;
//   };
  
  auto columnNumberFromMask = [] (const ColumnMask& columnMaskIn)
  {
    //if we probe for a column before it is set, Error!
    assert(columnMaskIn.any()); //toposort problem?
    //we can't use to_ulong or to_ullong. They are to small.
    //this might be slow?
    std::string buffer = columnMaskIn.to_string();
    std::size_t position = buffer.find_last_of('1');
    return (columnMaskIn.size() - position - 1);
  };
  
  TopoSortVisitor<Graph> visitor(stow->graph);
  ControlledDFS<Graph, TopoSortVisitor<Graph> > dfs(visitor);
  Path sorted = visitor.getResults();
  
  //reversed graph for calculating parent mask.
  GraphReversed rGraph = boost::make_reverse_graph(stow->graph);
  GraphReversed::adjacency_iterator parentIt, parentItEnd;
  
  std::size_t maxColumn = 0;
  std::size_t currentRow = 0;
  
  for (auto currentIt = sorted.begin(); currentIt != sorted.end(); ++currentIt)
  {
    Vertex currentVertex = *currentIt;
    stow->graph[currentVertex].sortedIndex = std::distance(sorted.begin(), currentIt);
    
    if (!stow->graph[currentVertex].dagVisible)
      continue;
    if (!stow->graph[currentVertex].alive)
      continue;
    
    std::size_t currentColumn = 0; //always default to first column
    ColumnMask spreadMask; //mask between child and all parents.
    ColumnMask targetMask; //mask of 'target' parent.
    std::tie(parentIt, parentItEnd) = boost::adjacent_vertices(currentVertex, rGraph);
    for (; parentIt != parentItEnd; ++parentIt)
    {
      bool results;
      GraphReversed::edge_descriptor currentEdge;
      std::tie(currentEdge, results) = boost::edge(currentVertex, *parentIt, rGraph);
      assert(results);
      if (rGraph[currentEdge].inputType.has(ftr::InputType::target))
        targetMask |= rGraph[*parentIt].columnMask; //should only be 1 target parent.
      
      auto parentSortedIndex = rGraph[*parentIt].sortedIndex;
      auto tempParentIt = sorted.begin() + parentSortedIndex;
      tempParentIt++;
      while (tempParentIt != currentIt)
      {
        Vertex parentVertex = *tempParentIt;
        spreadMask |= rGraph[parentVertex].columnMask;
        tempParentIt++;
      }
    }
    assert(targetMask.count() < 2); //just a little sanity check.
    
    if ((targetMask & (~spreadMask)).any())
      currentColumn = columnNumberFromMask(targetMask);
    else //target parent didn't work.
    {
      //find first usable column
      for (std::size_t nextColumn = 0; nextColumn < ColumnMask().size(); nextColumn++)
      {
        if (!spreadMask.test(nextColumn))
        {
        currentColumn = nextColumn;
        break;
        }
      }
    }
    
    maxColumn = std::max(currentColumn, maxColumn);
    ColumnMask freshMask;
    freshMask.set(currentColumn);
    
    stow->graph[currentVertex].columnMask = freshMask;
    stow->graph[currentVertex].row = currentRow;
    QBrush currentBrush(forgroundBrushes.at(currentColumn % forgroundBrushes.size()));
    
    auto rectangle = stow->graph[currentVertex].rectShared;
    rectangle->setRect(-rowPadding, 0.0, rowPadding, rowHeight); //calculate actual length later.
    rectangle->setTransform(QTransform::fromTranslate(0, rowHeight * currentRow));
    rectangle->setBackgroundBrush(backgroundBrushes[currentRow % backgroundBrushes.size()]);
    
    auto point = stow->graph[currentVertex].pointShared;
    point->setRect(0.0, 0.0, pointSize, pointSize);
    point->setTransform(QTransform::fromTranslate(pointSpacing * currentColumn,
      rowHeight * currentRow + rowHeight / 2.0 - pointSize / 2.0));
    point->setBrush(currentBrush);
    
    float cheat = 0.0;
    if (direction == -1)
      cheat = rowHeight;
    
    auto visiblePixmap = stow->graph[currentVertex].visibleIconShared;
    visiblePixmap->setTransform(QTransform::fromTranslate(0.0, rowHeight * currentRow + cheat)); //calculate x location later.
    
    auto overlayPixmap = stow->graph[currentVertex].overlayIconShared;
    overlayPixmap->setTransform(QTransform::fromTranslate(0.0, rowHeight * currentRow + cheat)); //calculate x location later.
    
    auto statePixmap = stow->graph[currentVertex].stateIconShared;
    statePixmap->setTransform(QTransform::fromTranslate(0.0, rowHeight * currentRow + cheat)); //calculate x location later.
    
    auto featurePixmap = stow->graph[currentVertex].featureIconShared;
    featurePixmap->setTransform(QTransform::fromTranslate(0.0, rowHeight * currentRow + cheat)); //calculate x location later.
    
    auto text = stow->graph[currentVertex].textShared;
    text->setDefaultTextColor(currentBrush.color());
    maxTextLength = std::max(maxTextLength, static_cast<float>(text->boundingRect().width()));
    text->setTransform(QTransform::fromTranslate
      (0.0, rowHeight * currentRow - verticalSpacing * 2.0 + cheat)); //calculate x location later.
    
    //update connector
    float currentX = pointSpacing * currentColumn + pointSize / 2.0;
    float currentY = rowHeight * currentRow + rowHeight / 2.0;
    
//     GraphReversed rGraph = boost::make_reverse_graph(graph);
//     GraphReversed::adjacency_iterator parentIt, parentItEnd;
    std::tie(parentIt, parentItEnd) = boost::adjacent_vertices(currentVertex, rGraph);
    for (; parentIt != parentItEnd; ++parentIt)
    {
      bool results;
      GraphReversed::edge_descriptor edge;
      std::tie(edge, results) = boost::edge(currentVertex, *parentIt, rGraph);
      assert(results);
      
      //create an offset along x axis to allow separation in multiple edge scenario.
      Edge forwardEdge;
      std::tie(forwardEdge, results) = boost::edge(*parentIt, currentVertex, stow->graph);
      assert(results);
      float currentXOffset = currentX + pointSize * 0.25 * stow->connectionOffset(currentVertex, forwardEdge);
      
      //we can't do this when loop through parents above because then we don't
      //know what the column is going to be.
      if (!rGraph[*parentIt].dagVisible)
        continue; //we don't make it here if source isn't visible. So don't have to worry about that.
      float dependentX = pointSpacing * static_cast<int>(columnNumberFromMask(rGraph[*parentIt].columnMask)) + pointSize / 2.0; //on center.
      float dependentY = rowHeight * rGraph[*parentIt].row + rowHeight / 2.0 + pointSize * 0.25 * stow->connectionOffset(*parentIt, forwardEdge);;
      
      QGraphicsPathItem *pathItem = rGraph[edge].connector.get();
      pathItem->setBrush(Qt::NoBrush);
      QPainterPath path;
      path.moveTo(currentXOffset, currentY);
      if (currentColumn == columnNumberFromMask(rGraph[*parentIt].columnMask))
        path.lineTo(currentXOffset, dependentY); //straight connector in y.
      else
      {
        //connector with bend.
        float radius = pointSpacing / 1.9; //no zero length line.
        
        path.lineTo(currentXOffset, dependentY + radius * direction);
      
        float yPosition;
        if (direction == -1.0)
          yPosition = dependentY - 2.0 * radius;
        else
          yPosition = dependentY;
        float width = 2.0 * radius;
        float height = width;
        if (dependentX > currentXOffset) //radius to the right.
        {
          QRectF arcRect(currentXOffset, yPosition, width, height);
          path.arcTo(arcRect, 180.0, 90.0 * -direction);
        }
        else //radius to the left.
        {
          QRectF arcRect(currentXOffset - 2.0 * radius, yPosition, width, height);
          path.arcTo(arcRect, 0.0, 90.0 * direction);
        }
        path.lineTo(dependentX, dependentY);
      }
      pathItem->setPath(path);
    }
    
    currentRow++;
  }
  
  //now that we have the graph drawn we know where to place icons and text.
  float columnSpacing = (maxColumn * pointSpacing);
  for (const auto &vertex : sorted)
  {
    float localCurrentX = columnSpacing;
    localCurrentX += pointToIcon;
    auto visiblePixmap = stow->graph[vertex].visibleIconShared;
    QTransform visibleIconTransform = QTransform::fromTranslate(localCurrentX, 0.0);
    visiblePixmap->setTransform(visiblePixmap->transform() * visibleIconTransform);
    
    localCurrentX += iconSize + iconToIcon;
    auto overlayPixmap = stow->graph[vertex].overlayIconShared;
    QTransform overlayIconTransform = QTransform::fromTranslate(localCurrentX, 0.0);
    overlayPixmap->setTransform(overlayPixmap->transform() * overlayIconTransform);
    
    localCurrentX += iconSize + iconToIcon;
    auto statePixmap = stow->graph[vertex].stateIconShared;
    QTransform stateIconTransform = QTransform::fromTranslate(localCurrentX, 0.0);
    statePixmap->setTransform(statePixmap->transform() * stateIconTransform);
    
    localCurrentX += iconSize + iconToIcon;
    auto pixmap = stow->graph[vertex].featureIconShared;
    QTransform iconTransform = QTransform::fromTranslate(localCurrentX, 0.0);
    pixmap->setTransform(pixmap->transform() * iconTransform);
    
    localCurrentX += iconSize + iconToText;
    auto text = stow->graph[vertex].textShared;
    QTransform textTransform = QTransform::fromTranslate(localCurrentX, 0.0);
    text->setTransform(text->transform() * textTransform);
    
    auto rectangle = stow->graph[vertex].rectShared;
    QRectF rect = rectangle->rect();
    rect.setWidth(localCurrentX + maxTextLength + 2.0 * rowPadding);
    rectangle->setRect(rect);
  }
  
  this->setSceneRect(this->itemsBoundingRect());
}

void Model::dumpDAGViewGraphDispatched(const msg::Message &)
{
  //something here to check preferences about writing this out.
  QString fileName = static_cast<app::Application *>(qApp)->getApplicationDirectory().path();
  fileName += QDir::separator();
  fileName += "dagView.dot";
  stow->writeGraphViz(fileName.toStdString());
  
  QDesktopServices::openUrl(QUrl(fileName));
}

void Model::threeDShowDispatched(const msg::Message &msgIn)
{
  Vertex vertex = stow->findVertex(boost::get<vwr::Message>(msgIn.payload).featureId);
  if (vertex == NullVertex())
    return;
  stow->graph[vertex].visibleIconShared->setPixmap(visiblePixmapEnabled);
}

void Model::threeDHideDispatched(const msg::Message &msgIn)
{
  Vertex vertex = stow->findVertex(boost::get<vwr::Message>(msgIn.payload).featureId);
  if (vertex == NullVertex())
    return;
  stow->graph[vertex].visibleIconShared->setPixmap(visiblePixmapDisabled);
}

void Model::overlayShowDispatched(const msg::Message &msgIn)
{
  Vertex vertex = stow->findVertex(boost::get<vwr::Message>(msgIn.payload).featureId);
  if (vertex == NullVertex())
    return;
  stow->graph[vertex].overlayIconShared->setPixmap(overlayPixmapEnabled);
}

void Model::overlayHideDispatched(const msg::Message &msgIn)
{
  Vertex vertex = stow->findVertex(boost::get<vwr::Message>(msgIn.payload).featureId);
  if (vertex == NullVertex())
    return;
  stow->graph[vertex].overlayIconShared->setPixmap(overlayPixmapDisabled);
}

void Model::removeAllItems()
{
  for (auto its = boost::vertices(stow->graph); its.first != its.second; ++its.first)
  {
    if (!stow->graph[*its.first].alive)
      continue;
    removeItemsFromScene(stow->getAllSceneItems(*its.first));
  }
  
  for (auto its = boost::edges(stow->graph); its.first != its.second; ++its.first)
  {
    if (stow->graph[*its.first].connector->scene())
      this->removeItem(stow->graph[*its.first].connector.get());
  }
}

void Model::addItemsToScene(std::vector<QGraphicsItem*> items)
{
  for (auto *item : items)
  {
    if (!item->scene())
      this->addItem(item);
  }
}

void Model::removeItemsFromScene(std::vector<QGraphicsItem*> items)
{
  for (auto *item : items)
  {
    if (item->scene())
      this->removeItem(item);
  }
}

RectItem* Model::getRectFromPosition(const QPointF& position)
{
  auto theItems = this->items(position, Qt::IntersectsItemBoundingRect, Qt::DescendingOrder);
  if (theItems.isEmpty())
    return nullptr;
  return dynamic_cast<RectItem *>(theItems.back());
}

void Model::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
  QGraphicsScene::mouseMoveEvent(event);
  
  RectItem *rect = getRectFromPosition(event->scenePos());
  if (dragData)
  {
    QGraphicsView *qgv = this->views().front();
    
    //search for edge first as it is more explicit.
    Vertex dv = stow->findVertex(dragData->featureId); //drag vertex
    auto pi = stow->graph[dv].pointShared.get(); //point item.
    QList<QGraphicsItem *> cis = collidingItems(pi);
    bool foundIntersection = false;
    for (QGraphicsItem* gi : cis)
    {
      if (gi->data(qtd::key).toInt() != qtd::connector)
        continue;
      if (pi->collidesWithItem(gi))
      {
        QGraphicsPathItem *npi = static_cast<QGraphicsPathItem *>(gi); //new path item
        Edge edge = stow->findEdge(npi);
        if (dragData->isAcceptedEdge(edge))
        {
          foundIntersection = true;
          if (dragData->lastHighlight)
            stow->highlightConnectorOff(dragData->lastHighlight);
          stow->highlightConnectorOn(npi, Qt::darkYellow);
          dragData->lastHighlight = npi;
          observer->out(msg::buildStatusMessage(QObject::tr("Drop accepted on edge").toStdString()));
          break;
        }
      }
    }
    if (!foundIntersection)
    {
      if (dragData->lastHighlight)
      {
        stow->highlightConnectorOff(dragData->lastHighlight);
        dragData->lastHighlight = nullptr;
      }
    }
    
    //now search for a vertex.
    if (!foundIntersection && rect)
    {
      Vertex rv = stow->findVertex(rect);
      if (rv != NullVertex())
      {
        const auto &av = dragData->acceptedVertices; //just makes next line readable
        if (std::find(av.begin(), av.end(), rv) != av.end())
        {
          foundIntersection = true;
          observer->out(msg::buildStatusMessage(QObject::tr("Drop accepted on vertex").toStdString()));
        }
      }
    }
    if (!foundIntersection)
    {
      qgv->setCursor(Qt::ForbiddenCursor);
      observer->out(msg::buildStatusMessage(QObject::tr("Drop rejected").toStdString()));
    }
    else
      qgv->setCursor(Qt::DragMoveCursor);
  }
  else //not in a drag operation
  {
    auto clearPrehighlight = [this]()
    {
      if (!currentPrehighlight)
        return;
      Vertex vertex = stow->findVertex(currentPrehighlight);
      if (vertex == NullVertex())
        return;
      
      msg::Message message(msg::Request | msg::Preselection | msg::Remove);
      slc::Message sMessage;
      sMessage.type = slc::Type::Object;
      sMessage.featureId = stow->graph[vertex].featureId;
      sMessage.shapeId = gu::createNilId();
      message.payload = sMessage;
      observer->out(message);
    };
    
    auto setPrehighlight = [this](RectItem *rectIn)
    {
      Vertex vertex = stow->findVertex(rectIn);
      if (vertex == NullVertex())
        return;
      
      msg::Message message(msg::Request | msg::Preselection | msg::Add);
      slc::Message sMessage;
      sMessage.type = slc::Type::Object;
      sMessage.featureId = stow->graph[vertex].featureId;
      sMessage.shapeId = gu::createNilId();
      message.payload = sMessage;
      observer->out(message);
    };
    
    if (rect == currentPrehighlight)
      return;
    else
    {
      clearPrehighlight();
      if (rect)
        setPrehighlight(rect);
    }
  }
}

void Model::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
  QGraphicsScene::mousePressEvent(event); //make sure we don't skip this.
  
  auto select = [this](const boost::uuids::uuid &featureIdIn, msg::Mask actionIn)
  {
    assert((actionIn == msg::Add) || (actionIn == msg::Remove));
    if (featureIdIn.is_nil())
      return;
    msg::Message message(msg::Request | msg::Selection | actionIn);
    slc::Message sMessage;
    sMessage.type = slc::Type::Object;
    sMessage.featureId = featureIdIn;
    sMessage.shapeId = gu::createNilId();
    message.payload = sMessage;
    observer->out(message);
  };
  
  auto getFeatureIdFromRect = [this](RectItem *rectIn)
  {
    assert(rectIn);
    Vertex vertex = stow->findVertex(rectIn);
    if (vertex == NullVertex())
      return gu::createNilId();
    return stow->graph[vertex].featureId;
  };
  
  auto goShiftSelect = [this, event, select, getFeatureIdFromRect]()
  {
    QPointF currentPickPoint = event->scenePos();
    QGraphicsLineItem intersectionLine(QLineF(lastPick, currentPickPoint));
    QList<QGraphicsItem *>selection = collidingItems(&intersectionLine);
    for (auto currentItem = selection.begin(); currentItem != selection.end(); ++currentItem)
    {
      RectItem *rect = dynamic_cast<RectItem *>(*currentItem);
      if (!rect || rect->isSelected())
        continue;
      select(getFeatureIdFromRect(rect), msg::Add);
    }
  };
  
  auto toggleSelect = [select, getFeatureIdFromRect](RectItem *rectIn)
  {
    if (rectIn->isSelected())
      select(getFeatureIdFromRect(rectIn), msg::Remove);
    else
      select(getFeatureIdFromRect(rectIn), msg::Add);
  };
//   
//   if (proxy)
//     renameAcceptedSlot();
  
  if (event->button() == Qt::LeftButton)
  {
    auto theItems = this->items(event->scenePos(), Qt::IntersectsItemBoundingRect, Qt::DescendingOrder);
    if (theItems.isEmpty())
      return;

    int currentType = theItems.front()->data(qtd::key).toInt();
    if (currentType == qtd::visibleIcon)
    {
      QGraphicsPixmapItem *currentPixmap = dynamic_cast<QGraphicsPixmapItem *>(theItems.front());
      assert(currentPixmap);
      
      Vertex vertex = stow->findVisibleVertex(currentPixmap);
      if (vertex == NullVertex())
        return;
      msg::Message msg(msg::Request | msg::View | msg::Toggle | msg::ThreeD);
      vwr::Message vMsg;
      vMsg.featureId = stow->graph[vertex].featureId;
      msg.payload = vMsg;
      observer->out(msg);
      
      return;
    }
    else if (currentType == qtd::overlayIcon)
    {
      QGraphicsPixmapItem *currentPixmap = dynamic_cast<QGraphicsPixmapItem *>(theItems.front());
      assert(currentPixmap);
      
      Vertex vertex = stow->findOverlayVertex(currentPixmap);
      if (vertex == NullVertex())
        return;
      msg::Message msg(msg::Request | msg::View | msg::Toggle | msg::Overlay);
      vwr::Message vMsg;
      vMsg.featureId = stow->graph[vertex].featureId; //temp during conversion.
      msg.payload = vMsg;
      observer->out(msg);
      
      return;
    }
    else if (currentType == qtd::point)
    {
      QGraphicsEllipseItem *ellipse = dynamic_cast<QGraphicsEllipseItem *>(theItems.front());
      assert(ellipse);
      
      Vertex vertex = stow->findVertex(ellipse);
      if (vertex == NullVertex())
        return;
      
      assert(!dragData);
      dragData = std::shared_ptr<DragData>(new DragData());
      dragData->featureId = stow->graph[vertex].featureId;
      std::tie(dragData->acceptedVertices, dragData->acceptedEdges) = stow->getDropAccepted(vertex);
      
      observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
      
      return;
    }
    
    //now going with selection.
    RectItem *rect = getRectFromPosition(event->scenePos());
    if (rect)
    {
      if((event->modifiers() & Qt::ShiftModifier) && lastPickValid)
      {
        goShiftSelect();
      }
      else
      {
        toggleSelect(rect);
      }
    }
  }
  else if (event->button() == Qt::MiddleButton)
  {
    msg::Message message(msg::Request | msg::Selection | msg::Clear);
    slc::Message sMessage;
    message.payload = sMessage;
    observer->out(message);
  }
}

void Model::mouseReleaseEvent(QGraphicsSceneMouseEvent *e)
{
  QGraphicsScene::mouseReleaseEvent(e); //make sure we don't skip this.
  
  if ((e->button() == Qt::LeftButton) && dragData)
  {
    //drag operation sets the ellipse xy location. set it back.
    Vertex dv = stow->findVertex(dragData->featureId);
    stow->graph[dv].pointShared->setX(0.0);
    stow->graph[dv].pointShared->setY(0.0);
    
    bool sentMessage = false;
    
    //make sure no edges are left highlighted.
    if (dragData->lastHighlight)
    {
      Edge edge = stow->findEdge(dragData->lastHighlight);
      stow->highlightConnectorOff(dragData->lastHighlight);
      dragData->lastHighlight = nullptr;
      prj::Message pmOut;
      pmOut.featureIds.push_back(dragData->featureId);
      pmOut.featureIds.push_back(stow->graph[boost::source(edge, stow->graph)].featureId);
      pmOut.featureIds.push_back(stow->graph[boost::target(edge, stow->graph)].featureId);
      msg::Message mOut(msg::Mask(msg::Request | msg::Project | msg::Feature | msg::Reorder), pmOut);
      observer->out(mOut);
      sentMessage = true;
    }
    else
    {
      RectItem *rect = getRectFromPosition(e->scenePos());
      if (rect)
      {
        Vertex rv = stow->findVertex(rect);
        if (rv != NullVertex())
        {
          if (std::find(dragData->acceptedVertices.begin(), dragData->acceptedVertices.end(), rv) != dragData->acceptedVertices.end())
          {
            prj::Message pmOut;
            pmOut.featureIds.push_back(dragData->featureId);
            pmOut.featureIds.push_back(stow->graph[rv].featureId);
            msg::Message mOut(msg::Mask(msg::Request | msg::Project | msg::Feature | msg::Reorder), pmOut);
            observer->out(mOut);
            sentMessage = true;
          }
        }
      }
    }
    
    if (!sentMessage)
      projectUpdatedDispatched(msg::Message());
    
    //take action.
    QGraphicsView *qgv = this->views().front();
    qgv->setCursor(Qt::ArrowCursor);
    dragData.reset();
  }
}

void Model::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
  if (event->button() == Qt::LeftButton)
    observer->out(msg::Message(msg::Request | msg::Edit | msg::Feature));
  
  QGraphicsScene::mouseDoubleClickEvent(event);
}

void Model::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
  auto theItems = this->items(event->scenePos(), Qt::IntersectsItemBoundingRect, Qt::DescendingOrder);
  if (!theItems.isEmpty())
  {
    int currentType = theItems.front()->data(qtd::key).toInt();
    if (currentType == qtd::visibleIcon)
    {
      Vertex v = stow->findVisibleVertex(dynamic_cast<QGraphicsPixmapItem*>(theItems.front()));
      observer->out(msg::Message(msg::Request | msg::View | msg::ThreeD | msg::Isolate, vwr::Message(stow->graph[v].featureId)));
      return;
    }
    else if (currentType == qtd::overlayIcon)
    {
      Vertex v = stow->findOverlayVertex(dynamic_cast<QGraphicsPixmapItem*>(theItems.front()));
      observer->out(msg::Message(msg::Request | msg::View | msg::Overlay | msg::Isolate, vwr::Message(stow->graph[v].featureId)));
      return;
    }
  }
  
  RectItem *rect = getRectFromPosition(event->scenePos());
  if (rect)
  {
    Vertex vertex = stow->findVertex(rect);
    if (vertex == NullVertex())
      return;
    if (!rect->isSelected())
    {
      msg::Message message(msg::Request | msg::Selection | msg::Add);
      slc::Message sMessage;
      sMessage.type = slc::Type::Object;
      sMessage.featureId = stow->graph[vertex].featureId;
      message.payload = sMessage;
      observer->out(message);
    }
    
    QMenu contextMenu;
    
    static QIcon overlayIcon(":/resources/images/dagViewOverlay.svg");
    QAction* toggleOverlayAction = contextMenu.addAction(overlayIcon, tr("Toggle Overlay"));
    connect(toggleOverlayAction, SIGNAL(triggered()), this, SLOT(toggleOverlaySlot()));
    
    static QIcon viewIsolateIcon(":/resources/images/dagViewIsolate.svg");
    QAction* viewIsolateAction = contextMenu.addAction(viewIsolateIcon, tr("View Isolate"));
    connect(viewIsolateAction, SIGNAL(triggered()), this, SLOT(viewIsolateSlot()));
    
    contextMenu.addSeparator();
    
    static QIcon infoFeatureIcon(":/resources/images/inspectInfo.svg");
    QAction* infoFeatureAction = contextMenu.addAction(infoFeatureIcon, tr("Info Feature"));
    connect(infoFeatureAction, SIGNAL(triggered()), this, SLOT(infoFeatureSlot()));
    
    static QIcon checkGeometryIcon(":/resources/images/inspectCheckGeometry.svg");
    QAction* checkGeometryAction = contextMenu.addAction(checkGeometryIcon, tr("Check Geometry"));
    connect(checkGeometryAction, SIGNAL(triggered()), this, SLOT(checkGeometrySlot()));
    
    static QIcon editFeatureIcon(":/resources/images/dagViewEditFeature.svg");
    QAction* editFeatureAction = contextMenu.addAction(editFeatureIcon, tr("Edit Feature"));
    connect(editFeatureAction, SIGNAL(triggered()), this, SLOT(editFeatureSlot()));
    
    static QIcon editColorIcon(":/resources/images/dagViewEditColor.svg");
    QAction* editColorAction = contextMenu.addAction(editColorIcon, tr("Edit Color"));
    connect(editColorAction, SIGNAL(triggered()), this, SLOT(editColorSlot()));
    
    static QIcon editRenameIcon(":/resources/images/dagViewEditRename.svg");
    QAction* editRenameAction = contextMenu.addAction(editRenameIcon, tr("Rename"));
    connect(editRenameAction, SIGNAL(triggered()), this, SLOT(editRenameSlot()));
    
    contextMenu.addSeparator();
    
    static QIcon leafIcon(":/resources/images/dagViewLeaf.svg");
    QAction* setCurrentLeafAction = contextMenu.addAction(leafIcon, tr("Set Current Leaf"));
    connect(setCurrentLeafAction, SIGNAL(triggered()), this, SLOT(setCurrentLeafSlot()));
    
    static QIcon removeIcon(":/resources/images/dagViewRemove.svg");
    QAction* removeFeatureAction = contextMenu.addAction(removeIcon, tr("Remove Feature"));
    connect(removeFeatureAction, SIGNAL(triggered()), this, SLOT(removeFeatureSlot()));
    
    std::vector<Vertex> selected = stow->getAllSelected();
    
    //disable actions that work for only 1 feature at a time.
    if (selected.size() != 1)
    {
      editRenameAction->setDisabled(true);
      editFeatureAction->setDisabled(true);
      setCurrentLeafAction->setDisabled(true);
    }
    
    //only show check geometry if all currently selected features have a seer shape.
    bool showCheckGeometry = true;
    for (const auto &v : selected)
    {
      if (!stow->graph[v].hasSeerShape)
      {
        showCheckGeometry = false;
        break;
      }
    }
    if (!showCheckGeometry)
      checkGeometryAction->setDisabled(true);
    
    contextMenu.exec(event->screenPos());
  }
  
  QGraphicsScene::contextMenuEvent(event);
}

void Model::setCurrentLeafSlot()
{
  auto currentSelections = stow->getAllSelected();
  assert(currentSelections.size() == 1);
  
  prj::Message prjMessageOut;
  prjMessageOut.featureIds.push_back(stow->graph[currentSelections.front()].featureId);
  msg::Message messageOut(msg::Request | msg::SetCurrentLeaf);
  messageOut.payload = prjMessageOut;
  observer->out(messageOut);
  
  if (prf::manager().rootPtr->dragger().triggerUpdateOnFinish())
    observer->out(msg::Mask(msg::Request | msg::Project | msg::Update));
}

void Model::removeFeatureSlot()
{
  msg::Message message(msg::Request | msg::Remove);
  observer->out(message);
}

void Model::toggleOverlaySlot()
{
  auto currentSelections = stow->getAllSelected();
  
  msg::Message message(msg::Request | msg::Selection | msg::Clear);
  observer->out(message);
  
  for (auto v : currentSelections)
  {
    msg::Message mOut(msg::Request | msg::View | msg::Toggle | msg::Overlay);
    vwr::Message vMsg;
    vMsg.featureId = stow->graph[v].featureId;
    mOut.payload = vMsg;
    observer->out(mOut);
  }
}

void Model::viewIsolateSlot()
{
  observer->out(msg::Message(msg::Request | msg::View | msg::ThreeD | msg::Overlay | msg::Isolate));
}

void Model::editColorSlot()
{
  observer->out(msg::Message(msg::Request | msg::Edit | msg::Feature | msg::Color));
}

void Model::editRenameSlot()
{
  assert(proxy == nullptr);
  std::vector<Vertex> selections = stow->getAllSelected();
  assert(selections.size() == 1);
  
  LineEdit *lineEdit = new LineEdit();
  auto text = stow->graph[selections.front()].textShared;
  lineEdit->setText(text->toPlainText());
  connect(lineEdit, SIGNAL(acceptedSignal()), this, SLOT(renameAcceptedSlot()));
  connect(lineEdit, SIGNAL(rejectedSignal()), this, SLOT(renameRejectedSlot()));
  
  proxy = this->addWidget(lineEdit);
  QRectF geometry = text->sceneBoundingRect();
  if (maxTextLength > 0.0)
    geometry.setWidth(maxTextLength);
  proxy->setGeometry(geometry);
  
  lineEdit->selectAll();
  QTimer::singleShot(0, lineEdit, SLOT(setFocus())); 
}

void Model::editFeatureSlot()
{
  observer->out(msg::Message(msg::Request | msg::Edit | msg::Feature));
}

void Model::infoFeatureSlot()
{
  observer->out(msg::Message(msg::Request | msg::Info));
}

void Model::checkGeometrySlot()
{
  observer->out(msg::Message(msg::Request | msg::CheckGeometry));
}

void Model::renameAcceptedSlot()
{
  assert(proxy);
  
  std::vector<Vertex> selections = stow->getAllSelected();
  assert(selections.size() == 1);
  
  LineEdit *lineEdit = dynamic_cast<LineEdit*>(proxy->widget());
  assert(lineEdit);
  QString freshName = lineEdit->text();
  if (!freshName.isEmpty())
  {
    ftr::Message fm(stow->graph[selections.front()].featureId, freshName);
    msg::Message m(msg::Request | msg::Edit | msg::Feature | msg::Name, fm);
    observer->out(m); //don't block rename makes it back here.
  }
  
  finishRename();
  
  observer->out(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

void Model::renameRejectedSlot()
{
  finishRename();
}

void Model::finishRename()
{
  assert(proxy);
  this->removeItem(proxy);
  delete proxy;
  proxy = nullptr;
  this->invalidate();
}

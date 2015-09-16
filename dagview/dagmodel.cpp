/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2015  tanderson <email>
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

#include <QString>
#include <QGraphicsTextItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsProxyWidget>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QPainter>
#include <QKeyEvent>
#include <QMenu>
#include <QTimer>

#include "../globalutilities.h"
#include "dagcontrolleddfs.h"
#include "dagmodel.h"

using namespace DAG;

// LineEdit::LineEdit(QWidget* parentIn): QLineEdit(parentIn)
// {
// 
// }
// 
// void LineEdit::keyPressEvent(QKeyEvent *eventIn)
// {
//   if (eventIn->key() == Qt::Key_Escape)
//   {
//     Q_EMIT rejectedSignal();
//     eventIn->accept();
//     return;
//   }
//   if (
//     (eventIn->key() == Qt::Key_Enter) ||
//     (eventIn->key() == Qt::Key_Return)
//   )
//   {
//     Q_EMIT acceptedSignal();
//     eventIn->accept();
//     return;
//   }
//   
//   QLineEdit::keyPressEvent(eventIn);
// }

//I dont think I should have to call invalidate
//and definitely not on the whole scene!
//if we have performance problems, this will definitely
//be something to re-visit. I am not wasting anymore time on
//this right now.
//   this->scene()->invalidate();
//   this->scene()->invalidate(this->sceneTransform().inverted().mapRect(this->boundingRect()));
//   update(boundingRect());
//note: I haven't tried this again since I turned BSP off.

Model::Model(QObject *parentIn) : QGraphicsScene(parentIn)
{
  //turned off BSP as it was giving inconsistent discovery of items
  //underneath cursor.
  this->setItemIndexMethod(QGraphicsScene::NoIndex);
  setupViewConstants();
  
//   setupFilters();
//   
  currentPrehighlight = nullptr;
//   
//   ParameterGrp::handle group = App::GetApplication().GetUserParameter().
//           GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("DAGView");
//     selectionMode = static_cast<SelectionMode>(group->GetInt("SelectionMode", 0));
//     group->SetInt("SelectionMode", static_cast<int>(selectionMode)); //ensure entry exists.
//     
  QIcon temp(":/resources/images/dagViewVisible.svg");
  visiblePixmapEnabled = temp.pixmap(iconSize, iconSize, QIcon::Normal, QIcon::On);
  visiblePixmapDisabled = temp.pixmap(iconSize, iconSize, QIcon::Disabled, QIcon::Off);
  
  QIcon passIcon(":/resources/images/dagViewPass.svg");
  passPixmap = passIcon.pixmap(iconSize, iconSize);
  QIcon failIcon(":/resources/images/dagViewFail.svg");
  failPixmap = failIcon.pixmap(iconSize, iconSize);
  QIcon pendingIcon(":/resources/images/dagViewPending.svg");
  pendingPixmap = pendingIcon.pixmap(iconSize, iconSize);
//   
//   renameAction = new QAction(this);
//   renameAction->setText(tr("Rename"));
//   renameAction->setStatusTip(tr("Rename object"));
//   renameAction->setShortcut(Qt::Key_F2);
//   connect(renameAction, SIGNAL(triggered()), this, SLOT(onRenameSlot()));
//   
//   editingFinishedAction = new QAction(this);
//   editingFinishedAction->setText(tr("Finish editing"));
//   editingFinishedAction->setStatusTip(tr("Finish editing object"));
//   connect(this->editingFinishedAction, SIGNAL(triggered()),
//           this, SLOT(editingFinishedSlot()));
}

Model::~Model()
{
  //we used shared pointers in the graphics map to manage memory.
  //so we don't want qt deleting these objects. so remove from scene.
  removeAllItems();
}

void Model::setupViewConstants()
{
  //get direction
  direction = 1.0;
  if (direction != -1.0 && direction != 1.0)
    direction = 1.0;
  
  QFontMetrics fontMetric(this->font());
  fontHeight = fontMetric.height();
  verticalSpacing = 1.0;
  rowHeight = (fontHeight + 2.0 * verticalSpacing) * direction; //pixel space top and bottom.
  iconSize = fontHeight;
  pointSize = fontHeight / 2.0;
  pointSpacing = pointSize;
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

void Model::featureAddedSlot(std::shared_ptr<Feature::Base> featureIn)
{
  Vertex virginVertex = boost::add_vertex(graph);
  graph[virginVertex].feature = featureIn;
  graph[virginVertex].featureId = featureIn->getId();
  
  //some of these are temp.
  graph[virginVertex].visibleIconRaw->setPixmap(visiblePixmapEnabled);
  graph[virginVertex].stateIconRaw->setPixmap(passPixmap);
  graph[virginVertex].featureIconRaw->setPixmap(featureIn->getIcon().pixmap(iconSize, iconSize));
  graph[virginVertex].textRaw->setPlainText(featureIn->getName());
  graph[virginVertex].textRaw->setFont(this->font());
  
  graphLink.insert(graph[virginVertex]);
  
  VertexIdRecord record;
  record.featureId = featureIn->getId();
  record.vertex = virginVertex;
  vertexIdContainer.insert(record);
  
  featureIn->connectState(boost::bind(&Model::stateChangedSlot, this, _1, _2));
  
  addVertexItemsToScene(virginVertex);
  
  this->invalidate(); //temp.
}

void Model::connectionAddedSlot(const boost::uuids::uuid &parentIdIn, const boost::uuids::uuid &childIdIn, Feature::InputTypes typeIn)
{
  Vertex parentVertex = findRecord(vertexIdContainer, parentIdIn).vertex;
  Vertex childVertex = findRecord(vertexIdContainer, childIdIn).vertex;
  
  bool results;
  Edge edge;
  std::tie(edge, results) = boost::add_edge(parentVertex, childVertex, graph);
  assert(results);
  graph[edge].inputType = typeIn;
  
  QPainterPath path;
  path.moveTo(0.0, 0.0);
  path.lineTo(0.0, 100.0);
  graph[edge].connector->setPath(path);
  
  addEdgeItemsToScene(edge);
}

void Model::stateChangedSlot(const boost::uuids::uuid &featureIdIn, std::size_t stateIn)
{
  Vertex vertex = findRecord(vertexIdContainer, featureIdIn).vertex;
  bool currentState = (graph[vertex].feature.lock()->getState().test(stateIn));
  
  if (stateIn == Feature::StateOffset::ModelDirty)
  {
    if (currentState)
      graph[vertex].stateIconRaw->setPixmap(pendingPixmap);
  }
  
  if (stateIn == Feature::StateOffset::Hidden3D)
  {
    if (currentState)
      graph[vertex].visibleIconRaw->setPixmap(visiblePixmapDisabled);
    else
      graph[vertex].visibleIconRaw->setPixmap(visiblePixmapEnabled);
  }
  
  if (stateIn == Feature::StateOffset::Failure)
  {
    if (currentState)
      graph[vertex].stateIconRaw->setPixmap(failPixmap);
    else
      graph[vertex].stateIconRaw->setPixmap(passPixmap);
      
  }
//   std::cout <<
//     "state changed. Feature id is: " << featureIdIn <<
//     "      state offset is: " << Feature::StateOffset::toString(stateIn) <<
//     "      state value is: " << ((currentState) ? "true" : "false") <<  std::endl;
}

// void Model::selectionChanged(const SelectionChanges& msg)
// {
//   //note that treeview uses set selection which sends a message with just a document name
//   //and no object name. Have to explore further.
//   
//   auto getAllEdges = [this](const Vertex &vertexIn)
//   {
//     //is there really no function to get both in and out edges?
//     std::vector<Edge> out;
//     
//     OutEdgeIterator outIt, outItEnd;
//     for (boost::tie(outIt, outItEnd) = boost::out_edges(vertexIn, *theGraph); outIt != outItEnd; ++outIt)
//       out.push_back(*outIt);
//     
//     InEdgeIterator inIt, inItEnd;
//     for (boost::tie(inIt, inItEnd) = boost::in_edges(vertexIn, *theGraph); inIt != inItEnd; ++inIt)
//       out.push_back(*inIt);
//     
//     return out;
//   };
//   
//   auto highlightConnectorOn = [this, getAllEdges](const Vertex &vertexIn)
//   {
//     QColor color = (*theGraph)[vertexIn].text->defaultTextColor();
//     QPen pen(color);
//     pen.setWidth(3.0);
//     auto edges = getAllEdges(vertexIn);
//     for (auto edge : edges)
//     {
//       (*theGraph)[edge].connector->setPen(pen);
//       (*theGraph)[edge].connector->setZValue(1.0);
//     }
//   };
//   
//   auto highlightConnectorOff = [this, getAllEdges](const Vertex &vertexIn)
//   {
//     auto edges = getAllEdges(vertexIn);
//     for (auto edge : edges)
//     {
//       (*theGraph)[edge].connector->setPen(QPen());
//       (*theGraph)[edge].connector->setZValue(0.0);
//     }
//   };
//   
//   //lamda for clearing selections.
//   auto clearSelection = [this, highlightConnectorOff]()
//   {
//     BGL_FORALL_VERTICES(currentVertex, *theGraph, Graph)
//     {
//       RectItem *rect = (*theGraph)[currentVertex].rectangle.get();
//       assert(rect);
//       rect->selectionOff();
//       highlightConnectorOff(currentVertex);
//     }
//   };
//   
//   //lamda for getting rectangle.
//   auto getRectangle = [this](const char *in)
//   {
//     assert(in);
//     std::string name(in);
//     assert(!name.empty());
//     const GraphLinkRecord &record = findRecord(name, *graphLink);
//     RectItem *rect = (*theGraph)[record.vertex].rectangle.get();
//     assert(rect);
//     return rect;
//   };
//   
//   if (msg.Type == SelectionChanges::AddSelection)
//   {
//     if (msg.pObjectName)
//     {
//       RectItem *rect = getRectangle(msg.pObjectName);
//       rect->selectionOn();
//       highlightConnectorOn(findRecord(std::string(msg.pObjectName), *graphLink).vertex);
//     }
//   }
//   else if(msg.Type == SelectionChanges::RmvSelection)
//   {
//     if (msg.pObjectName)
//     {
//       RectItem *rect = getRectangle(msg.pObjectName);
//       rect->selectionOff();
//       highlightConnectorOff(findRecord(std::string(msg.pObjectName), *graphLink).vertex);
//     }
//   }
//   else if(msg.Type == SelectionChanges::SetSelection)
//   {
//     clearSelection();
//     
//     auto selections = Gui::Selection().getSelection(msg.pDocName);
//     for (const auto &selection : selections)
//     {
//       assert(selection.FeatName);
//       RectItem *rect = getRectangle(selection.FeatName);
//       rect->selectionOn();
//       highlightConnectorOn(findRecord(selection.FeatName, *graphLink).vertex);
//     }
//   }
//   else if(msg.Type == SelectionChanges::ClrSelection)
//   {
//     clearSelection();
//   }
//   
//   this->invalidate();
// }

// void Model::awake()
// {
//   if (graphDirty)
//   {
//     updateSlot();
//     this->invalidate();
//   }
//   updateStates();
// }

void Model::projectUpdatedSlot()
{
  outputGraphviz<Graph>(graph, "/home/tanderson/temp/dagview.dot");
  
  
//   auto dumpMask = [] (const ColumnMask& columnMaskIn)
//   {
//     //have to create a smaller subset to get through std::cout.
//     std::bitset<8> testSet;
//     for (unsigned int index = 0; index < testSet.size(); ++index)
//       testSet[index] = columnMaskIn[index];
//     std::cout << testSet.to_string() << std::endl;
//   };
  
  //this obsoletes the passed in sortedIds.
  TopoSortVisitor<Graph> visitor(graph);
  ControlledDFS<Graph, TopoSortVisitor<Graph> > dfs(visitor);
  Path sorted = visitor.getResults();
  
  std::size_t currentRow = 0;
  std::size_t currentColumn = 0;
  std::size_t maxColumn = 0;
  float maxTextLength = 0;
  ColumnMask futureChildMask; //keeps track of columns that are awaiting children.
  for (auto currentIt = sorted.begin(); currentIt != sorted.end(); ++currentIt)
  {
    Vertex currentVertex = *currentIt;
    graph[currentVertex].sortedIndex = std::distance(sorted.begin(), currentIt);
    
    if (!graph[currentVertex].dagVisible)
      continue;
    
    //find appropriate column.
    currentColumn = 0; //always default to first column
    ColumnMask spreadMask;
    
    //this isn't complete. now when finding alter parent the spreadMask
    //is ignored. This probably won't work. revist after some more features
    //like linked copy and union.
    if (graph[currentVertex].feature.lock()->getDescriptor() == Feature::Descriptor::Alter)
    {
      //build spreadMask. spreadMask reflects occupied columns between
      //child and farthest parent. 'Farthest' is relative to topo sort.
      std::size_t maxSpread = 0;
      
      GraphReversed rGraph = boost::make_reverse_graph(graph);
      GraphReversed::adjacency_iterator parentIt, parentItEnd;
      std::tie(parentIt, parentItEnd) = boost::adjacent_vertices(currentVertex, rGraph);
      for (; parentIt != parentItEnd; ++parentIt)
      {
        bool results;
        GraphReversed::edge_descriptor currentEdge;
        std::tie(currentEdge, results) = boost::edge(currentVertex, *parentIt, rGraph);
        assert(results);
        if (rGraph[currentEdge].inputType == Feature::InputTypes::target)
          currentColumn = static_cast<std::size_t>(std::log2(rGraph[*parentIt].columnMask.to_ulong()));
        
        auto parentSortedIndex = rGraph[*parentIt].sortedIndex;
        auto currentSortedIndex = rGraph[*currentIt].sortedIndex;
        //spread is the distance between the current parent and the current entry
        //in the topo sorted vector. 1 means adjacency, 2 means 1 feature between etc..
        auto spread = currentSortedIndex - parentSortedIndex;
        
        if ((spread < 2) || (spread < maxSpread))
          continue;
        
        maxSpread = spread;
        spreadMask.reset();
        auto tempParentIt = sorted.begin() + parentSortedIndex;
        tempParentIt++;
        while (tempParentIt != currentIt)
        {
          Vertex parentVertex = *tempParentIt;
          spreadMask |= rGraph[parentVertex].columnMask;
          tempParentIt++;
//           dumpMask(spreadMask);
        }
      }
    }
    else
    {
      //use masks to determin column and set.
      ColumnMask testMask = spreadMask | futureChildMask;
      for (std::size_t index = 0; index < testMask.size(); ++index)
      {
        if (! testMask.test(index))
        {
          currentColumn = index;
          break;
        }
      }
    }
    
    maxColumn = std::max(currentColumn, maxColumn);
    ColumnMask freshMask;
    freshMask.set(currentColumn);
    
    //now loop through children and decide if we need
    //to reserve this column for future children.
    VertexAdjacencyIterator it, itEnd;
    std::tie(it, itEnd) = boost::adjacent_vertices(currentVertex, graph);
    for (; it != itEnd; ++it)
    {
      bool results;
      Edge edge;
      Feature::InputTypes type;
      std::tie(edge, results) = boost::edge(currentVertex, *it, graph);
      if (results)
        type = graph[edge].inputType;
      if
      (
        (graph[*it].feature.lock()->getDescriptor() == Feature::Descriptor::Alter) &&
        (type == Feature::InputTypes::target)
      )
        futureChildMask |= freshMask;
      else
        futureChildMask &= ~freshMask;
    }
    
    
    //update futureChildMask to reserve column for future.
    //quit half way through implementing the following.
//     GraphicsEdgeByParentIterator startChild, endChild;
//     std::tie(startChild, endChild) = findParents(graphicsEdgeMap, currentRecord.featureId);
//     if (startChild != endChild)
//       futureChildMask |= freshMask;
    
    graph[currentVertex].columnMask = freshMask;
    graph[currentVertex].row = currentRow;
    QBrush currentBrush(forgroundBrushes.at(currentColumn % forgroundBrushes.size()));
    
    auto *rectangle = graph[currentVertex].rectRaw;
    rectangle->setRect(-rowPadding, 0.0, rowPadding, rowHeight); //calculate actual length later.
    rectangle->setTransform(QTransform::fromTranslate(0, rowHeight * currentRow));
    rectangle->setBackgroundBrush(backgroundBrushes[currentRow % backgroundBrushes.size()]);
    
    auto *point = graph[currentVertex].pointRaw;
    point->setRect(0.0, 0.0, pointSize, pointSize);
    point->setTransform(QTransform::fromTranslate(pointSpacing * currentColumn,
      rowHeight * currentRow + rowHeight / 2.0 - pointSize / 2.0));
    point->setBrush(currentBrush);
    
    float cheat = 0.0;
    if (direction == -1)
      cheat = rowHeight;
    
    auto *visiblePixmap = graph[currentVertex].visibleIconRaw;
    visiblePixmap->setTransform(QTransform::fromTranslate(0.0, rowHeight * currentRow + cheat)); //calculate x location later.
    
    auto *statePixmap = graph[currentVertex].stateIconRaw;
    statePixmap->setTransform(QTransform::fromTranslate(0.0, rowHeight * currentRow + cheat)); //calculate x location later.
    
    auto *featurePixmap = graph[currentVertex].featureIconRaw;
    featurePixmap->setTransform(QTransform::fromTranslate(0.0, rowHeight * currentRow + cheat)); //calculate x location later.
    
    auto *text = graph[currentVertex].textRaw;
    text->setDefaultTextColor(currentBrush.color());
    maxTextLength = std::max(maxTextLength, static_cast<float>(text->boundingRect().width()));
    text->setTransform(QTransform::fromTranslate
      (0.0, rowHeight * currentRow - verticalSpacing * 2.0 + cheat)); //calculate x location later.
    
    //update connector
    float currentX = pointSpacing * currentColumn + pointSize / 2.0;
    float currentY = rowHeight * currentRow + rowHeight / 2.0;
    
    GraphReversed rGraph = boost::make_reverse_graph(graph);
    GraphReversed::adjacency_iterator parentIt, parentItEnd;
    std::tie(parentIt, parentItEnd) = boost::adjacent_vertices(currentVertex, rGraph);
    for (; parentIt != parentItEnd; ++parentIt)
    {
      //we can't do this when loop through parents above because then we don't
      //know what the column is going to be.
      if (!rGraph[*parentIt].dagVisible)
        continue; //we don't make it here if source isn't visible. So don't have to worry about that.
      float dependentX = pointSpacing * static_cast<int>(std::log2(rGraph[*parentIt].columnMask.to_ulong())) + pointSize / 2.0; //on center.
      float dependentY = rowHeight * rGraph[*parentIt].row + rowHeight / 2.0;
      
      bool results;
      GraphReversed::edge_descriptor edge;
      std::tie(edge, results) = boost::edge(currentVertex, *parentIt, rGraph);
      assert(results);
      QGraphicsPathItem *pathItem = rGraph[edge].connector.get();
      pathItem->setBrush(Qt::NoBrush);
      QPainterPath path;
      path.moveTo(currentX, currentY);
      if (currentColumn == static_cast<std::size_t>(std::log2(rGraph[*parentIt].columnMask.to_ulong())))
        path.lineTo(currentX, dependentY); //straight connector in y.
      else
      {
        //connector with bend.
        float radius = pointSpacing / 1.9; //no zero length line.
        
        path.lineTo(currentX, dependentY + radius * direction);
      
        float yPosition;
        if (direction == -1.0)
          yPosition = dependentY - 2.0 * radius;
        else
          yPosition = dependentY;
        float width = 2.0 * radius;
        float height = width;
        if (dependentX > currentX) //radius to the right.
        {
          QRectF arcRect(currentX, yPosition, width, height);
          path.arcTo(arcRect, 180.0, 90.0 * -direction);
        }
        else //radius to the left.
        {
          QRectF arcRect(currentX - 2.0 * radius, yPosition, width, height);
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
    auto *visiblePixmap = graph[vertex].visibleIconRaw;
    QTransform visibleIconTransform = QTransform::fromTranslate(localCurrentX, 0.0);
    visiblePixmap->setTransform(visiblePixmap->transform() * visibleIconTransform);
    
    localCurrentX += iconSize + iconToIcon;
    auto *statePixmap = graph[vertex].stateIconRaw;
    QTransform stateIconTransform = QTransform::fromTranslate(localCurrentX, 0.0);
    statePixmap->setTransform(statePixmap->transform() * stateIconTransform);
    
    localCurrentX += iconSize + iconToIcon;
    auto *pixmap = graph[vertex].featureIconRaw;
    QTransform iconTransform = QTransform::fromTranslate(localCurrentX, 0.0);
    pixmap->setTransform(pixmap->transform() * iconTransform);
    
    localCurrentX += iconSize + iconToText;
    auto *text = graph[vertex].textRaw;
    QTransform textTransform = QTransform::fromTranslate(localCurrentX, 0.0);
    text->setTransform(text->transform() * textTransform);
    
    auto *rectangle = graph[vertex].rectRaw;
    QRectF rect = rectangle->rect();
    rect.setWidth(localCurrentX + maxTextLength + 2.0 * rowPadding);
    rectangle->setRect(rect);
  }
}

void Model::removeAllItems()
{
  BGL_FORALL_VERTICES(currentVertex, graph, Graph)
  {
    removeVertexItemsFromScene(currentVertex);
  }
  
  BGL_FORALL_EDGES(currentEdge, graph, Graph)
  {
    removeEdgeItemsFromScene(currentEdge);
  }
}

void Model::addVertexItemsToScene(Vertex vertexIn)
{
  //these are either all in or all out. so just test rectangle.
  if (graph[vertexIn].rectRaw->scene()) //already in the scene.
    return;
  this->addItem(graph[vertexIn].rectRaw);
  this->addItem(graph[vertexIn].pointRaw);
  this->addItem(graph[vertexIn].visibleIconRaw);
  this->addItem(graph[vertexIn].stateIconRaw);
  this->addItem(graph[vertexIn].featureIconRaw);
  this->addItem(graph[vertexIn].textRaw);
}

void Model::removeVertexItemsFromScene(Vertex vertexIn)
{
  //these are either all in or all out. so just test rectangle.
  if (!(graph[vertexIn].rectRaw->scene())) //not in the scene.
    return;
  
  this->removeItem(graph[vertexIn].rectRaw);
  this->removeItem(graph[vertexIn].pointRaw);
  this->removeItem(graph[vertexIn].visibleIconRaw);
  this->removeItem(graph[vertexIn].stateIconRaw);
  this->removeItem(graph[vertexIn].featureIconRaw);
  this->removeItem(graph[vertexIn].textRaw);
}

void Model::addEdgeItemsToScene(Edge edgeIn)
{
  if (graph[edgeIn].connector->scene())
    return;
  this->addItem(graph[edgeIn].connector.get());
}

void Model::removeEdgeItemsFromScene(Edge edgeIn)
{
  if (!graph[edgeIn].connector->scene())
    return;
  this->removeItem(graph[edgeIn].connector.get());
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
  auto clearPrehighlight = [this]()
  {
    if (currentPrehighlight)
    {
      currentPrehighlight->preHighlightOff();
      currentPrehighlight = nullptr;
    }
  };
  
  RectItem *rect = getRectFromPosition(event->scenePos());
  if (!rect)
  {
    clearPrehighlight();
    return;
  }
  
  if (rect == currentPrehighlight)
    return;
  
  clearPrehighlight();
  rect->preHighlightOn();
  currentPrehighlight = rect;
  invalidate();
  
  QGraphicsScene::mouseMoveEvent(event);
}

void Model::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
//   auto goShiftSelect = [this, event]()
//   {
//     QPointF currentPickPoint = event->scenePos();
//     QGraphicsLineItem intersectionLine(QLineF(lastPick, currentPickPoint));
//     QList<QGraphicsItem *>selection = collidingItems(&intersectionLine);
//     for (auto currentItem = selection.begin(); currentItem != selection.end(); ++currentItem)
//     {
//       RectItem *rect = dynamic_cast<RectItem *>(*currentItem);
//       if (!rect) continue;
//       const GraphLinkRecord &selectionRecord = findRecord(rect, *graphLink);
//       Gui::Selection().addSelection(selectionRecord.DObject->getDocument()->getName(),
//                                     selectionRecord.DObject->getNameInDocument());
//     }
//   };
//   
//   auto toggleSelect = [](const App::DocumentObject *dObjectIn, RectItem *rectIn)
//   {
//     if (rectIn->isSelected())
//       Gui::Selection().rmvSelection(dObjectIn->getDocument()->getName(), dObjectIn->getNameInDocument());
//     else
//       Gui::Selection().addSelection(dObjectIn->getDocument()->getName(), dObjectIn->getNameInDocument());
//   };
//   
//   if (proxy)
//     renameAcceptedSlot();
  
  if (event->button() == Qt::LeftButton)
  {
    auto theItems = this->items(event->scenePos(), Qt::IntersectsItemBoundingRect, Qt::DescendingOrder);
    if (theItems.isEmpty())
      return; //TODO clear selection.
    for (auto it = theItems.constBegin(); it != theItems.constEnd(); ++it)
    {
      int currentType = (*it)->data(QtData::key).toInt();
      if (currentType == QtData::visibleIcon)
      {
        QGraphicsPixmapItem *currentPixmap = dynamic_cast<QGraphicsPixmapItem *>(*it);
        //this is kind of ugly. we are index the actual vertex property. so can't vertex
        //without and extra search into vertex id container. ?????
        Vertex vertex = findRecord(vertexIdContainer, findRecordByVisible(graphLink, currentPixmap).featureId).vertex;
        graph[vertex].feature.lock()->toggle3D();
      }
    }
//     RectItem *rect = getRectFromPosition(event->scenePos());
//     if (rect)
//     {
//         const GraphLinkRecord &record = findRecord(rect, *graphLink);
//         
//         //don't like that I am doing this again here after getRectFromPosition call.
//         QGraphicsItem *item = itemAt(event->scenePos());
//         QGraphicsPixmapItem *pixmapItem = dynamic_cast<QGraphicsPixmapItem *>(item);
//         if (pixmapItem && (pixmapItem == (*theGraph)[record.vertex].visibleIcon.get()))
//         {
//           //get all selections, but for now just the current pick.
//           if ((*theGraph)[record.vertex].lastVisibleState == VisibilityState::Off)
//             const_cast<ViewProviderDocumentObject *>(record.VPDObject)->show(); //const hack
//           else
//             const_cast<ViewProviderDocumentObject *>(record.VPDObject)->hide(); //const hack
//             
//           return;
//         }
//         
//         const App::DocumentObject *dObject = record.DObject;
//         if (selectionMode == SelectionMode::Single)
//         {
//           if (event->modifiers() & Qt::ControlModifier)
//           {
//             toggleSelect(dObject, rect);
//           }
//           else if((event->modifiers() & Qt::ShiftModifier) && lastPickValid)
//           {
//             goShiftSelect();
//           }
//           else
//           {
//             Gui::Selection().clearSelection(dObject->getDocument()->getName());
//             Gui::Selection().addSelection(dObject->getDocument()->getName(), dObject->getNameInDocument());
//           }
//         }
//         if (selectionMode == SelectionMode::Multiple)
//         {
//           if((event->modifiers() & Qt::ShiftModifier) && lastPickValid)
//           {
//             goShiftSelect();
//           }
//           else
//           {
//             toggleSelect(dObject, rect);
//           }
//         }
//         lastPickValid = true;
//         lastPick = event->scenePos();
//     }
//     else
//     {
//       lastPickValid = false;
//       Gui::Selection().clearSelection(); //get document name?
//     }
  }
  
  QGraphicsScene::mousePressEvent(event);
}

// void Model::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
// {
//   if (event->button() == Qt::LeftButton)
//   {
//     auto selections = getAllSelected();
//     if(selections.size() != 1)
//       return;
//     const GraphLinkRecord &record = findRecord(selections.front(), *graphLink);
//     Gui::Document* doc = Gui::Application::Instance->getDocument(record.DObject->getDocument());
//     MDIView *view = doc->getActiveView();
//     if (view)
//       getMainWindow()->setActiveWindow(view);
//     const_cast<ViewProviderDocumentObject*>(record.VPDObject)->doubleClicked();
//   }
//   
//   QGraphicsScene::mouseDoubleClickEvent(event);
// }


// std::vector<Gui::DAG::Vertex> Model::getAllSelected()
// {
//   std::vector<Gui::DAG::Vertex> out;
//   
//   BGL_FORALL_VERTICES(currentVertex, *theGraph, Graph)
//   {
//     if ((*theGraph)[currentVertex].rectangle->isSelected())
//       out.push_back(currentVertex);
//   }
//   
//   return out;
// }

// void Model::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
// {
//   RectItem *rect = getRectFromPosition(event->scenePos());
//   if (rect)
//   {
//     const GraphLinkRecord &record = findRecord(rect, *graphLink);
//     
//     //don't like that I am doing this again here after getRectFromPosition call.
//     QGraphicsItem *item = itemAt(event->scenePos());
//     QGraphicsPixmapItem *pixmapItem = dynamic_cast<QGraphicsPixmapItem *>(item);
//     if (pixmapItem && (pixmapItem == (*theGraph)[record.vertex].visibleIcon.get()))
//     {
//       visiblyIsolate(record.vertex);
//       return;
//     }
//     
//     if (!rect->isSelected())
//     {
//       Gui::Selection().clearSelection(record.DObject->getDocument()->getName());
//       Gui::Selection().addSelection(record.DObject->getDocument()->getName(), record.DObject->getNameInDocument());
//       lastPickValid = true;
//       lastPick = event->scenePos();
//     }
//     
//     MenuItem view;
//     Gui::Application::Instance->setupContextMenu("Tree", &view);
//     QMenu contextMenu;
//     MenuManager::getInstance()->setupContextMenu(&view, contextMenu);
//     
//     //actions for only one selection.
//     std::vector<Gui::DAG::Vertex> selections = getAllSelected();
//     if (selections.size() == 1)
//     {
//       contextMenu.addAction(renameAction);
//       //when we have only one selection then we know it is rect from above.
//       if (!rect->isEditing())
//         const_cast<Gui::ViewProviderDocumentObject*>(record.VPDObject)->setupContextMenu
//           (&contextMenu, this, SLOT(editingStartSlot())); //const hack.
//       else
//         contextMenu.addAction(editingFinishedAction);
//     }
//     
//     if (contextMenu.actions().count() > 0)
//         contextMenu.exec(event->screenPos());
//   }
//   
//   QGraphicsScene::contextMenuEvent(event);
// }

// void Model::onRenameSlot()
// {
//   assert(proxy == nullptr);
//   std::vector<Gui::DAG::Vertex> selections = getAllSelected();
//   assert(selections.size() == 1);
//   
//   LineEdit *lineEdit = new LineEdit();
//   auto *text = (*theGraph)[selections.front()].text.get();
//   lineEdit->setText(text->toPlainText());
//   connect(lineEdit, SIGNAL(acceptedSignal()), this, SLOT(renameAcceptedSlot()));
//   connect(lineEdit, SIGNAL(rejectedSignal()), this, SLOT(renameRejectedSlot()));
//   
//   proxy = this->addWidget(lineEdit);
//   proxy->setGeometry(text->sceneBoundingRect());
//   
//   lineEdit->selectAll();
//   QTimer::singleShot(0, lineEdit, SLOT(setFocus())); 
// }

// void Model::renameAcceptedSlot()
// {
//   assert(proxy);
//   
//   std::vector<Gui::DAG::Vertex> selections = getAllSelected();
//   assert(selections.size() == 1);
//   const GraphLinkRecord &record = findRecord(selections.front(), *graphLink);
//   
//   LineEdit *lineEdit = dynamic_cast<LineEdit*>(proxy->widget());
//   assert(lineEdit);
//   const_cast<App::DocumentObject*>(record.DObject)->Label.setValue(lineEdit->text().toUtf8().constData()); //const hack
//   
//   finishRename();
// }

// void Model::renameRejectedSlot()
// {
//   finishRename();
// }

// void Model::finishRename()
// {
//   assert(proxy);
//   this->removeItem(proxy);
//   proxy->deleteLater();
//   proxy = nullptr;
//   this->invalidate();
// }

// void Model::editingStartSlot()
// {
//   QAction* action = qobject_cast<QAction*>(sender());
//   if (action)
//   {
//     int edit = action->data().toInt();
//     auto selections = getAllSelected();
//     assert(selections.size() == 1);
//     const GraphLinkRecord &record = findRecord(selections.front(), *graphLink);
//     Gui::Document* doc = Gui::Application::Instance->getDocument(record.DObject->getDocument());
//     MDIView *view = doc->getActiveView();
//     if (view)
//       getMainWindow()->setActiveWindow(view);
//     doc->setEdit(const_cast<ViewProviderDocumentObject*>(record.VPDObject), edit);
//   }
// }

// void Model::editingFinishedSlot()
// {
//   auto selections = getAllSelected();
//   assert(selections.size() == 1);
//   const GraphLinkRecord &record = findRecord(selections.front(), *graphLink);
//   Gui::Document* doc = Gui::Application::Instance->getDocument(record.DObject->getDocument());
//   doc->commitCommand();
//   doc->resetEdit();
//   doc->getDocument()->recompute();
// }

// void Model::visiblyIsolate(Gui::DAG::Vertex sourceIn)
// {
//   auto buildSkipTypes = []()
//   {
//     std::vector<Base::Type> out;
//     Base::Type type;
//     type = Base::Type::fromName("App::DocumentObjectGroup");
//     if (type != Base::Type::badType()) out.push_back(type);
//     type = Base::Type::fromName("App::Part");
//     if (type != Base::Type::badType()) out.push_back(type);
//     type = Base::Type::fromName("PartDesign::Body");
//     if (type != Base::Type::badType()) out.push_back(type);
//     
//     return out;
//   };
//   
//   auto testSkipType = [](const App::DocumentObject *dObject, const std::vector<Base::Type> &types)
//   {
//     for (const auto &currentType : types)
//     {
//       if (dObject->isDerivedFrom(currentType))
//         return true;
//     }
//     return false;
//   };
//   
//   indexVerticesEdges();
//   Path connectedVertices;
//   ConnectionVisitor visitor(connectedVertices);
//   boost::breadth_first_search(*theGraph, sourceIn, boost::visitor(visitor));
//   boost::breadth_first_search(boost::make_reverse_graph(*theGraph), sourceIn, boost::visitor(visitor));
//   
//   //note source vertex is added twice to Path. Once for each search.
//   static std::vector<Base::Type> skipTypes = buildSkipTypes();
//   for (const auto &currentVertex : connectedVertices)
//   {
//     const GraphLinkRecord &record = findRecord(currentVertex, *graphLink);
//     if (testSkipType(record.DObject, skipTypes))
//       continue;
//     const_cast<ViewProviderDocumentObject *>(record.VPDObject)->hide(); //const hack
//   }
//   
//   const GraphLinkRecord &sourceRecord = findRecord(sourceIn, *graphLink);
//   if (!testSkipType(sourceRecord.DObject, skipTypes))
//     const_cast<ViewProviderDocumentObject *>(sourceRecord.VPDObject)->show(); //const hack
// }

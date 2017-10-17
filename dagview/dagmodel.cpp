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

#include <QString>
#include <QTextStream>
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
#include <QFocusEvent>
#include <QMenu>
#include <QTimer>
#include <QUrl>
#include <QDesktopServices>

#include <application/application.h>
#include <project/project.h>
#include <globalutilities.h>
#include <tools/idtools.h>
#include <message/dispatch.h>
#include <message/observer.h>
#include <dagview/dagcontrolleddfs.h>
#include <dagview/dagmodel.h>

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

Model::Model(QObject *parentIn) : QGraphicsScene(parentIn)
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

void Model::indexVerticesEdges()
{
  std::size_t index = 0;
  
  //index vertices.
  BGL_FORALL_VERTICES(currentVertex, graph, Graph)
  {
    boost::put(boost::vertex_index, graph, currentVertex, index);
    index++;
  }

  //index edges.
  index = 0;
  BGL_FORALL_EDGES(currentEdge, graph, Graph)
  {
    boost::put(boost::edge_index, graph, currentEdge, index);
    index++;
  }
}

void Model::featureAddedDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  prj::Message message = boost::get<prj::Message>(messageIn.payload);
  
  Vertex virginVertex = boost::add_vertex(graph);
  graph[virginVertex].feature = message.feature;
  graph[virginVertex].featureId = message.feature->getId();
  
  if (message.feature->isVisible3D())
    graph[virginVertex].visibleIconRaw->setPixmap(visiblePixmapEnabled);
  else
    graph[virginVertex].visibleIconRaw->setPixmap(visiblePixmapDisabled);
  
  //this is pretty much a duplicate of the state changed 'slot'
  if (message.feature->isInactive())
    graph[virginVertex].stateIconRaw->setPixmap(inactivePixmap);
  else if (message.feature->isModelDirty())
    graph[virginVertex].stateIconRaw->setPixmap(pendingPixmap);
  else if (message.feature->isFailure())
    graph[virginVertex].stateIconRaw->setPixmap(failPixmap);
  else
    graph[virginVertex].stateIconRaw->setPixmap(passPixmap);
  
  graph[virginVertex].featureIconRaw->setPixmap(message.feature->getIcon().pixmap(iconSize, iconSize));
  graph[virginVertex].textRaw->setPlainText(message.feature->getName());
  graph[virginVertex].textRaw->setFont(this->font());
  
  graphLink.insert(graph[virginVertex]);
  
  VertexIdRecord record;
  record.featureId = message.feature->getId();
  record.vertex = virginVertex;
  vertexIdContainer.insert(record);
  
  addVertexItemsToScene(virginVertex);
  
  this->invalidate(); //temp.
}

void Model::featureRemovedDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  prj::Message message = boost::get<prj::Message>(messageIn.payload);
  
  Vertex vertex = findRecord(vertexIdContainer, message.feature->getId()).vertex;
  eraseRecord(graphLink, message.feature->getId());
  eraseRecord(vertexIdContainer, message.feature->getId());
  removeVertexItemsFromScene(vertex);
  assert(boost::in_degree(vertex, graph) == 0);
  assert(boost::out_degree(vertex, graph) == 0);
  boost::remove_vertex(vertex, graph);
}


void Model::connectionAddedDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  prj::Message message = boost::get<prj::Message>(messageIn.payload);
  
  Vertex parentVertex = findRecord(vertexIdContainer, message.featureId).vertex;
  Vertex childVertex = findRecord(vertexIdContainer, message.featureId2).vertex;
  
  bool results;
  Edge edge;
  std::tie(edge, results) = boost::add_edge(parentVertex, childVertex, graph);
  assert(results);
  graph[edge].inputType = message.inputType;
  
  QPainterPath path;
  path.moveTo(0.0, 0.0);
  path.lineTo(0.0, 100.0);
  graph[edge].connector->setPath(path);
  
  addEdgeItemsToScene(edge);
}

void Model::connectionRemovedDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  prj::Message message = boost::get<prj::Message>(messageIn.payload);
  
  Vertex parentVertex = findRecord(vertexIdContainer, message.featureId).vertex;
  Vertex childVertex = findRecord(vertexIdContainer, message.featureId2).vertex;
  
  bool results;
  Edge edge;
  std::tie(edge, results) = boost::edge(parentVertex, childVertex, graph);
  assert(results);
  
  removeEdgeItemsFromScene(edge);
  boost::remove_edge(edge, graph);
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
  
  mask = msg::Response | msg::Post | msg::UpdateModel;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::projectUpdatedDispatched, this, _1)));
  
  mask = msg::Response | msg::Post | msg::Preselection | msg::Add;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::preselectionAdditionDispatched, this, _1)));
  
  mask = msg::Response | msg::Pre | msg::Preselection | msg::Remove;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::preselectionSubtractionDispatched, this, _1)));
  
  mask = msg::Response | msg::Post | msg::Selection | msg::Add;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::selectionAdditionDispatched, this, _1)));
  
  mask = msg::Response | msg::Pre | msg::Selection | msg::Remove;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::selectionSubtractionDispatched, this, _1)));
  
  mask = msg::Response | msg::Pre | msg::CloseProject;;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::closeProjectDispatched, this, _1)));
  
  mask = msg::Response | msg::Feature | msg::Status;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::featureStateChangedDispatched, this, _1)));
  
  mask = msg::Response | msg::Edit | msg::Feature | msg::Name;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::featureRenamedDispatched, this, _1)));
  
  mask = msg::Request | msg::DebugDumpDAGViewGraph;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::dumpDAGViewGraphDispatched, this, _1)));
}

void Model::featureStateChangedDispatched(const msg::Message &messageIn)
{
  ftr::Message fMessage = boost::get<ftr::Message>(messageIn.payload);

  Vertex vertex = findRecord(vertexIdContainer, fMessage.featureId).vertex;
  assert(!graph[vertex].feature.expired());
  std::shared_ptr<ftr::Base> feature = graph[vertex].feature.lock();
  
  auto updateStateIcon = [&]()
  {
    //from highest to lowest priority.
    if (feature->isInactive())
    {
      graph[vertex].stateIconRaw->setPixmap(inactivePixmap);
      return;
    }
    
    if (feature->isModelDirty())
    {
      graph[vertex].stateIconRaw->setPixmap(pendingPixmap);
      return;
    }
    
    if (feature->isFailure())
      graph[vertex].stateIconRaw->setPixmap(failPixmap);
    else
      graph[vertex].stateIconRaw->setPixmap(passPixmap);
  };
  
  if
  (
    (fMessage.stateOffset == ftr::StateOffset::ModelDirty) ||
    (fMessage.stateOffset == ftr::StateOffset::Failure) ||
    (fMessage.stateOffset == ftr::StateOffset::Inactive)
  )
    updateStateIcon();
  
  ftr::State featureState = fMessage.state;
  bool currentChangedState = fMessage.freshValue;
  
  if (fMessage.stateOffset == ftr::StateOffset::Hidden3D)
  {
    if (currentChangedState)
      graph[vertex].visibleIconRaw->setPixmap(visiblePixmapDisabled);
    else
      graph[vertex].visibleIconRaw->setPixmap(visiblePixmapEnabled);
  }
  
  if (fMessage.stateOffset == ftr::StateOffset::NonLeaf)
  {
    if (currentChangedState)
    {
      if (graph[vertex].visibleIconRaw->scene())
        removeItem(graph[vertex].visibleIconRaw);
    }
    else
    {
      if (!graph[vertex].visibleIconRaw->scene())
        addItem(graph[vertex].visibleIconRaw);
    }
  }
  
  //set tool tip to current state.
  QString ts = tr("True");
  QString fs = tr("False");
  QString toolTip;
  QTextStream stream(&toolTip);
  stream <<
  "<table border=\"1\" cellpadding=\"6\">" << 
  "<tr>" <<
  "<td>" << tr("Model Dirty") << "</td><td>" << ((featureState.test(ftr::StateOffset::ModelDirty)) ? ts : fs) << "</td>" <<
  "</tr><tr>" <<
  "<td>" << tr("Visual Dirty") << "</td><td>" << ((featureState.test(ftr::StateOffset::VisualDirty)) ? ts : fs) << "</td>" <<
  "</tr>" <<
  "</tr><tr>" <<
  "<td>" << tr("Hidden 3D") << "</td><td>" << ((featureState.test(ftr::StateOffset::Hidden3D)) ? ts : fs) << "</td>" <<
  "</tr>" <<
  "</tr><tr>" <<
  "<td>" << tr("Hidden Overlay") << "</td><td>" << ((featureState.test(ftr::StateOffset::HiddenOverlay)) ? ts : fs) << "</td>" <<
  "</tr>" <<
  "</tr>" <<
  "</tr><tr>" <<
  "<td>" << tr("Failure") << "</td><td>" << ((featureState.test(ftr::StateOffset::Failure)) ? ts : fs) << "</td>" <<
  "</tr>" <<
  "</tr>" <<
  "</tr><tr>" <<
  "<td>" << tr("Inactive") << "</td><td>" << ((featureState.test(ftr::StateOffset::Inactive)) ? ts : fs) << "</td>" <<
  "</tr>" <<
  "</tr>" <<
  "</tr><tr>" <<
  "<td>" << tr("Non-Leaf") << "</td><td>" << ((featureState.test(ftr::StateOffset::NonLeaf)) ? ts : fs) << "</td>" <<
  "</tr>" <<
  "</table>";
  graph[vertex].stateIconRaw->setToolTip(toolTip);
}

void Model::featureRenamedDispatched(const msg::Message &messageIn)
{
  ftr::Message fMessage = boost::get<ftr::Message>(messageIn.payload);
  Vertex vertex = findRecord(vertexIdContainer, fMessage.featureId).vertex;
  graph[vertex].textRaw->setPlainText(fMessage.string);
}

void Model::preselectionAdditionDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  slc::Message sMessage = boost::get<slc::Message>(messageIn.payload);
  if (sMessage.type != slc::Type::Object)
    return;
  Vertex vertex = findRecord(vertexIdContainer, sMessage.featureId).vertex;
  assert(!currentPrehighlight); //trying to set prehighlight when something is already set.
  currentPrehighlight = graph[vertex].rectRaw;
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
  Vertex vertex = findRecord(vertexIdContainer, sMessage.featureId).vertex;
  assert(currentPrehighlight); //trying to clear prehighlight when already empty.
  graph[vertex].rectRaw->preHighlightOff();
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
  Vertex vertex = findRecord(vertexIdContainer, sMessage.featureId).vertex;
  graph[vertex].rectRaw->selectionOn();
  
  lastPickValid = true;
  lastPick = graph[vertex].rectRaw->mapToScene(graph[vertex].rectRaw->rect().center());
  
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
  Vertex vertex = findRecord(vertexIdContainer, sMessage.featureId).vertex;
  graph[vertex].rectRaw->selectionOff();
  
  lastPickValid = false;
  
  invalidate();
}

void Model::closeProjectDispatched(const msg::Message&)
{
  removeAllItems();
  
  //remove all edges.
  //collect vertices to remove
  std::vector<Vertex> vs;
  BGL_FORALL_VERTICES(currentVertex, graph, Graph)
  {
    boost::clear_vertex(currentVertex, graph);
    vs.push_back(currentVertex);
  }

  //remove all vertices
  for (auto &vertex : vs)
    boost::remove_vertex(vertex, graph);
  
  ::dag::clear(graphLink);
  ::dag::clear(vertexIdContainer);
  
  invalidate();
}

void Model::projectUpdatedDispatched(const msg::Message &)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  indexVerticesEdges();
  
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
  
  TopoSortVisitor<Graph> visitor(graph);
  ControlledDFS<Graph, TopoSortVisitor<Graph> > dfs(visitor);
  Path sorted = visitor.getResults();
  
  //reversed graph for calculating parent mask.
  GraphReversed rGraph = boost::make_reverse_graph(graph);
  GraphReversed::adjacency_iterator parentIt, parentItEnd;
  
  std::size_t maxColumn = 0;
  std::size_t currentRow = 0;
  
  for (auto currentIt = sorted.begin(); currentIt != sorted.end(); ++currentIt)
  {
    Vertex currentVertex = *currentIt;
    graph[currentVertex].sortedIndex = std::distance(sorted.begin(), currentIt);
    
    if (!graph[currentVertex].dagVisible)
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
    
//     GraphReversed rGraph = boost::make_reverse_graph(graph);
//     GraphReversed::adjacency_iterator parentIt, parentItEnd;
    std::tie(parentIt, parentItEnd) = boost::adjacent_vertices(currentVertex, rGraph);
    for (; parentIt != parentItEnd; ++parentIt)
    {
      //we can't do this when loop through parents above because then we don't
      //know what the column is going to be.
      if (!rGraph[*parentIt].dagVisible)
        continue; //we don't make it here if source isn't visible. So don't have to worry about that.
      float dependentX = pointSpacing * static_cast<int>(columnNumberFromMask(rGraph[*parentIt].columnMask)) + pointSize / 2.0; //on center.
      float dependentY = rowHeight * rGraph[*parentIt].row + rowHeight / 2.0;
      
      bool results;
      GraphReversed::edge_descriptor edge;
      std::tie(edge, results) = boost::edge(currentVertex, *parentIt, rGraph);
      assert(results);
      QGraphicsPathItem *pathItem = rGraph[edge].connector.get();
      pathItem->setBrush(Qt::NoBrush);
      QPainterPath path;
      path.moveTo(currentX, currentY);
      if (currentColumn == columnNumberFromMask(rGraph[*parentIt].columnMask))
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

void Model::dumpDAGViewGraphDispatched(const msg::Message &)
{
  indexVerticesEdges();
  //something here to check preferences about writing this out.
  QString fileName = static_cast<app::Application *>(qApp)->getApplicationDirectory().path();
  fileName += QDir::separator();
  fileName += "dagView.dot";
  outputGraphviz<Graph>(graph, fileName.toStdString());
  
  QDesktopServices::openUrl(QUrl(fileName));
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
  //check if already in scene.
  if (!graph[vertexIn].rectRaw->scene())
    this->addItem(graph[vertexIn].rectRaw);
  if (!graph[vertexIn].pointRaw->scene())
    this->addItem(graph[vertexIn].pointRaw);
  if (!graph[vertexIn].visibleIconRaw->scene())
    this->addItem(graph[vertexIn].visibleIconRaw);
  if (!graph[vertexIn].stateIconRaw->scene())
    this->addItem(graph[vertexIn].stateIconRaw);
  if (!graph[vertexIn].featureIconRaw->scene())
    this->addItem(graph[vertexIn].featureIconRaw);
  if (!graph[vertexIn].textRaw->scene())
    this->addItem(graph[vertexIn].textRaw);
}

void Model::removeVertexItemsFromScene(Vertex vertexIn)
{
  //check if in scene.
  if (graph[vertexIn].rectRaw->scene())
    this->removeItem(graph[vertexIn].rectRaw);
  if (graph[vertexIn].pointRaw->scene())
    this->removeItem(graph[vertexIn].pointRaw);
  if (graph[vertexIn].visibleIconRaw->scene())
    this->removeItem(graph[vertexIn].visibleIconRaw);
  if (graph[vertexIn].stateIconRaw->scene())
    this->removeItem(graph[vertexIn].stateIconRaw);
  if (graph[vertexIn].featureIconRaw->scene())
    this->removeItem(graph[vertexIn].featureIconRaw);
  if (graph[vertexIn].textRaw->scene())
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
    if (!currentPrehighlight)
      return;
    const VertexProperty& record = findRecord(graphLink, currentPrehighlight);
    
    msg::Message message(msg::Request | msg::Preselection | msg::Remove);
    slc::Message sMessage;
    sMessage.type = slc::Type::Object;
    sMessage.featureId = record.featureId;
    sMessage.featureType = record.feature.lock()->getType();
    sMessage.shapeId = gu::createNilId();
    message.payload = sMessage;
    observer->out(message);
  };
  
  auto setPrehighlight = [this](RectItem *rectIn)
  {
    const VertexProperty& record = findRecord(graphLink, rectIn);
    
    msg::Message message(msg::Request | msg::Preselection | msg::Add);
    slc::Message sMessage;
    sMessage.type = slc::Type::Object;
    sMessage.featureId = record.featureId;
    sMessage.featureType = record.feature.lock()->getType();
    sMessage.shapeId = gu::createNilId();
    message.payload = sMessage;
    observer->out(message);
  };
  
  RectItem *rect = getRectFromPosition(event->scenePos());
  if (rect == currentPrehighlight)
    return;
  clearPrehighlight();
  if (!rect)
    return;
  
  setPrehighlight(rect);
  
  QGraphicsScene::mouseMoveEvent(event);
}

void Model::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
  auto select = [this](const uuid &featureIdIn, msg::Mask actionIn)
  {
    assert((actionIn == msg::Add) || (actionIn == msg::Remove));
    msg::Message message(msg::Request | msg::Selection | actionIn);
    slc::Message sMessage;
    sMessage.type = slc::Type::Object;
    sMessage.featureId = featureIdIn;
    sMessage.featureType = findRecord(graphLink, featureIdIn).feature.lock()->getType();
    sMessage.shapeId = gu::createNilId();
    message.payload = sMessage;
    observer->out(message);
  };
  
  auto getFeatureIdFromRect = [this](RectItem *rectIn)
  {
    assert(rectIn);
    const VertexProperty &selectionRecord = findRecord(graphLink, rectIn);
    return selectionRecord.featureId;
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
  
  auto toggleSelect = [this, select, getFeatureIdFromRect](RectItem *rectIn)
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
      //this is kind of ugly. we are indexing the actual vertex PROPERTY. so we
      //can't get the vertex without and extra search into vertex id container. ?????
      Vertex vertex = findRecord(vertexIdContainer, findRecordByVisible(graphLink, currentPixmap).featureId).vertex;
      graph[vertex].feature.lock()->toggle3D();
      return;
    }
    
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
  
  if (event->button() == Qt::MiddleButton)
  {
    msg::Message message(msg::Request | msg::Selection | msg::Clear);
    slc::Message sMessage;
    message.payload = sMessage;
    observer->out(message);
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

std::vector<Vertex> Model::getAllSelected()
{
  std::vector<Vertex> out;
  
  BGL_FORALL_VERTICES(currentVertex, graph, Graph)
  {
    if (graph[currentVertex].rectRaw->isSelected())
      out.push_back(currentVertex);
  }
  
  return out;
}

void Model::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
  RectItem *rect = getRectFromPosition(event->scenePos());
  if (rect)
  {
    const VertexProperty &record = findRecord(graphLink, rect);
    if (!rect->isSelected())
    {
      msg::Message message(msg::Request | msg::Selection | msg::Add);
      slc::Message sMessage;
      sMessage.type = slc::Type::Object;
      sMessage.featureId = record.featureId;
      sMessage.featureType = record.feature.lock()->getType();
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
    
    //disable actions that work for only 1 feature at a time.
    if (getAllSelected().size() != 1)
    {
      editRenameAction->setDisabled(true);
      editFeatureAction->setDisabled(true);
      setCurrentLeafAction->setDisabled(true);
    }
    
    contextMenu.exec(event->screenPos());
  }
  
  QGraphicsScene::contextMenuEvent(event);
}

void Model::setCurrentLeafSlot()
{
  auto currentSelections = getAllSelected();
  assert(currentSelections.size() == 1);
  
  //temp for testing
  prj::Message prjMessageOut;
  prjMessageOut.featureId = graph[currentSelections.front()].featureId;
  msg::Message messageOut(msg::Request | msg::SetCurrentLeaf);
  messageOut.payload = prjMessageOut;
  observer->out(messageOut);
}

void Model::removeFeatureSlot()
{
  msg::Message message(msg::Request | msg::Remove);
  observer->out(message);
}

void Model::toggleOverlaySlot()
{
  auto currentSelections = getAllSelected();
  
  msg::Message message(msg::Request | msg::Selection | msg::Clear);
  observer->out(message);
  
  for (auto v : currentSelections)
    graph[v].feature.lock()->toggleOverlay();
}

void Model::viewIsolateSlot()
{
  observer->out(msg::Message(msg::Request | msg::ViewIsolate));
}

void Model::editColorSlot()
{
  observer->out(msg::Message(msg::Request | msg::Edit | msg::Feature | msg::Color));
}

void Model::editRenameSlot()
{
  assert(proxy == nullptr);
  std::vector<Vertex> selections = getAllSelected();
  assert(selections.size() == 1);
  
  LineEdit *lineEdit = new LineEdit();
  auto *text = graph[selections.front()].textRaw;
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

void Model::renameAcceptedSlot()
{
  assert(proxy);
  
  std::vector<Vertex> selections = getAllSelected();
  assert(selections.size() == 1);
  
  LineEdit *lineEdit = dynamic_cast<LineEdit*>(proxy->widget());
  assert(lineEdit);
  QString freshName = lineEdit->text();
  if (!freshName.isEmpty())
  {
    std::shared_ptr<ftr::Base> feature = graph[selections.front()].feature.lock();
    QString oldName = feature->getName();
    feature->setName(freshName);
    
    //setting the name doesn't make the feature dirty and thus doesn't
    //serialize it. Here we force a serialization so rename is in sync
    //with git, but doesn't trigger an unneeded update.
    feature->serialWrite(QDir(QString::fromStdString
      (static_cast<app::Application*>(qApp)->getProject()->getSaveDirectory())));
    
    std::ostringstream gitStream;
    gitStream << "Rename feature id: " << gu::idToString(feature->getId())
    << "    From: " << oldName.toStdString()
    << "    To: " << freshName.toStdString();
    observer->out(msg::buildGitMessage(gitStream.str()));
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

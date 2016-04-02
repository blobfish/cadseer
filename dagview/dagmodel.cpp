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
#include <QMenu>
#include <QTimer>

#include <application/application.h>
#include <globalutilities.h>
#include <message/dispatch.h>
#include <message/observer.h>
#include <dagview/dagcontrolleddfs.h>
#include <dagview/dagmodel.h>

using namespace dag;

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
  observer = std::move(std::unique_ptr<msg::Observer>(new msg::Observer()));
  setupDispatcher();
  
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
  QIcon inactiveIcon(":/resources/images/dagViewInactive.svg");
  inactivePixmap = inactiveIcon.pixmap(iconSize, iconSize);
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
  
  //some of these are temp.
  graph[virginVertex].visibleIconRaw->setPixmap(visiblePixmapEnabled);
  graph[virginVertex].stateIconRaw->setPixmap(passPixmap);
  graph[virginVertex].featureIconRaw->setPixmap(message.feature->getIcon().pixmap(iconSize, iconSize));
  graph[virginVertex].textRaw->setPlainText(message.feature->getName());
  graph[virginVertex].textRaw->setFont(this->font());
  
  graphLink.insert(graph[virginVertex]);
  
  VertexIdRecord record;
  record.featureId = message.feature->getId();
  record.vertex = virginVertex;
  vertexIdContainer.insert(record);
  
  graph[virginVertex].connection = message.feature->connectState(boost::bind(&Model::stateChangedSlot, this, _1, _2));
  
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
  graph[vertex].connection.disconnect();
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
  assert(graph[edge].inputType == message.inputType);
  
  removeEdgeItemsFromScene(edge);
  boost::remove_edge(edge, graph);
}

void Model::stateChangedSlot(const boost::uuids::uuid &featureIdIn, std::size_t stateOffsetChanged)
{
  Vertex vertex = findRecord(vertexIdContainer, featureIdIn).vertex;
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
    (stateOffsetChanged == ftr::StateOffset::ModelDirty) ||
    (stateOffsetChanged == ftr::StateOffset::Failure) ||
    (stateOffsetChanged == ftr::StateOffset::Inactive)
  )
    updateStateIcon();
  
  ftr::State featureState = feature->getState();
  bool currentChangedState = featureState.test(stateOffsetChanged);
  
  if (stateOffsetChanged == ftr::StateOffset::Hidden3D)
  {
    if (currentChangedState)
      graph[vertex].visibleIconRaw->setPixmap(visiblePixmapDisabled);
    else
      graph[vertex].visibleIconRaw->setPixmap(visiblePixmapEnabled);
  }
  
  if (stateOffsetChanged == ftr::StateOffset::NonLeaf)
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
//   std::cout <<
//     "state changed. Feature id is: " << featureIdIn <<
//     "      state offset is: " << ftr::StateOffset::toString(stateIn) <<
//     "      state value is: " << ((currentState) ? "true" : "false") <<  std::endl;

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

void Model::setupDispatcher()
{
  msg::Mask mask;
  
  mask = msg::Response | msg::Post | msg::AddFeature;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::featureAddedDispatched, this, _1)));
  
  mask = msg::Response | msg::Pre | msg::RemoveFeature;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::featureRemovedDispatched, this, _1)));
  
  mask = msg::Response | msg::Post | msg::AddConnection;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::connectionAddedDispatched, this, _1)));
  
  mask = msg::Response | msg::Pre | msg::RemoveConnection;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::connectionRemovedDispatched, this, _1)));
  
  mask = msg::Response | msg::Post | msg::UpdateModel;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::projectUpdatedDispatched, this, _1)));
  
  mask = msg::Response | msg::Post | msg::Preselection | msg::Addition;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::preselectionAdditionDispatched, this, _1)));
  
  mask = msg::Response | msg::Pre | msg::Preselection | msg::Subtraction;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::preselectionSubtractionDispatched, this, _1)));
  
  mask = msg::Response | msg::Post | msg::Selection | msg::Addition;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::selectionAdditionDispatched, this, _1)));
  
  mask = msg::Response | msg::Pre | msg::Selection | msg::Subtraction;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::selectionSubtractionDispatched, this, _1)));
  
  mask = msg::Response | msg::Pre | msg::CloseProject;;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Model::closeProjectDispatched, this, _1)));
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
    graph[currentVertex].connection.disconnect();
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

void Model::projectUpdatedDispatched(const msg::Message &)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  indexVerticesEdges();
  //something here to check preferences about writing this out.
  QString fileName = static_cast<app::Application *>(qApp)->getApplicationDirectory().path();
  fileName += QDir::separator();
  fileName += "dagView.dot";
  outputGraphviz<Graph>(graph, fileName.toStdString());
  
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
  float maxTextLength = 0;
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
      if (rGraph[currentEdge].inputType == ftr::InputTypes::target)
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
    
    msg::Message message;
    message.mask = msg::Request | msg::Preselection | msg::Subtraction;
    slc::Message sMessage;
    sMessage.type = slc::Type::Object;
    sMessage.featureId = record.featureId;
    sMessage.shapeId = boost::uuids::nil_generator()();
    message.payload = sMessage;
    observer->messageOutSignal(message);
  };
  
  auto setPrehighlight = [this](RectItem *rectIn)
  {
    const VertexProperty& record = findRecord(graphLink, rectIn);
    
    msg::Message message;
    message.mask = msg::Request | msg::Preselection | msg::Addition;
    slc::Message sMessage;
    sMessage.type = slc::Type::Object;
    sMessage.featureId = record.featureId;
    sMessage.shapeId = boost::uuids::nil_generator()();
    message.payload = sMessage;
    observer->messageOutSignal(message);
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
    assert((actionIn == msg::Addition) || (actionIn == msg::Subtraction));
    msg::Message message;
    message.mask = msg::Request | msg::Selection | actionIn;
    slc::Message sMessage;
    sMessage.type = slc::Type::Object;
    sMessage.featureId = featureIdIn;
    sMessage.shapeId = boost::uuids::nil_generator()();
    message.payload = sMessage;
    observer->messageOutSignal(message);
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
      select(getFeatureIdFromRect(rect), msg::Addition);
    }
  };
  
  auto toggleSelect = [this, select, getFeatureIdFromRect](RectItem *rectIn)
  {
    if (rectIn->isSelected())
      select(getFeatureIdFromRect(rectIn), msg::Subtraction);
    else
      select(getFeatureIdFromRect(rectIn), msg::Addition);
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
    msg::Message message;
    message.mask = msg::Request | msg::Selection | msg::Clear;
    slc::Message sMessage;
    message.payload = sMessage;
    observer->messageOutSignal(message);
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
      msg::Message message;
      message.mask = msg::Request | msg::Selection | msg::Addition;
      slc::Message sMessage;
      sMessage.type = slc::Type::Object;
      sMessage.featureId = record.featureId;
      message.payload = sMessage;
      observer->messageOutSignal(message);
    }
    
    QMenu contextMenu;
    
    if (getAllSelected().size() == 1)
    {
      static QIcon leafIcon(":/resources/images/dagViewLeaf.svg");
      QAction* setCurrentLeafAction = contextMenu.addAction(leafIcon, tr("Set Current Leaf"));
      connect(setCurrentLeafAction, SIGNAL(triggered()), this, SLOT(setCurrentLeafSlot()));
    }
    
    static QIcon removeIcon(":/resources/images/dagViewRemove.svg");
    QAction* removeFeatureAction = contextMenu.addAction(removeIcon, tr("Remove Feature"));
    connect(removeFeatureAction, SIGNAL(triggered()), this, SLOT(removeFeatureSlot()));
    
    static QIcon overlayIcon(":/resources/images/dagViewOverlay.svg");
    QAction* toggleOverlayAction = contextMenu.addAction(overlayIcon, tr("Toggle Overlay"));
    connect(toggleOverlayAction, SIGNAL(triggered()), this, SLOT(toggleOverlaySlot()));
    
    contextMenu.exec(event->screenPos());
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
  msg::Message messageOut;
  messageOut.mask = msg::Request | msg::SetCurrentLeaf;
  messageOut.payload = prjMessageOut;
  observer->messageOutSignal(messageOut);
}

void Model::removeFeatureSlot()
{
  msg::Message message(msg::Request | msg::Remove);
  observer->messageOutSignal(message);
}

void Model::toggleOverlaySlot()
{
  auto currentSelections = getAllSelected();
  
  msg::Message message;
  message.mask = msg::Request | msg::Selection | msg::Clear;
  observer->messageOutSignal(message);
  
  for (auto v : currentSelections)
    graph[v].feature.lock()->toggleOverlay();
}

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

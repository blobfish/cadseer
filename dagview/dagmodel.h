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

#ifndef DAGMODEL_H
#define DAGMODEL_H

#include <memory>
#include <vector>
#include <bitset>

#include <boost/uuid/uuid.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/composite_key.hpp>

#include <QGraphicsScene>
#include <QBrush>
#include <QLineEdit>

#include "dagmodelgraph.h"
// #include "DAGFilter.h"

class QGraphicsSceneHoverEvent;
class QGraphicsProxyWidget;

namespace DAG
{
//   class LineEdit : public QLineEdit
//   {
//   Q_OBJECT
//   public:
//     LineEdit(QWidget *parentIn = 0);
//   Q_SIGNALS:
//     void acceptedSignal();
//     void rejectedSignal();
//   protected:
//   virtual void keyPressEvent(QKeyEvent*);
//   };
  
  class Model : public QGraphicsScene
  {
    Q_OBJECT
  public:
    Model(QObject *parentIn);
    virtual ~Model() override;
    
    void featureAddedSlot(std::shared_ptr<Feature::Base>); //!<received from the project.
    void connectionAddedSlot(const boost::uuids::uuid&, const boost::uuids::uuid&, Feature::InputTypes);
    void projectUpdatedSlot(); //!<received from the project.
    
  protected:
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    virtual void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
//     virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
//     virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
    
//   private Q_SLOTS:
//     void updateSlot();
//     void onRenameSlot();
//     void renameAcceptedSlot();
//     void renameRejectedSlot();
//     void editingStartSlot();
//     void editingFinishedSlot();
    
  private:
    Model(){}
    void stateChangedSlot(const boost::uuids::uuid &featureIdIn, std::size_t stateIn); //!< received from each feature.
    
    Graph graph;
    GraphLinkContainer graphLink;
    VertexIdContainer vertexIdContainer;
    
    void removeAllItems();
    void addVertexItemsToScene(Vertex);
    void addEdgeItemsToScene(Edge);
    void removeVertexItemsFromScene(Vertex);
    void removeEdgeItemsFromScene(Edge);
//     
    RectItem* getRectFromPosition(const QPointF &position); //!< can be nullptr
//     
  //! @name View Constants for spacing
  //@{
    float fontHeight;                           //!< height of the current qApp default font.
    float direction;                            //!< controls top to bottom or bottom to top direction.
    float verticalSpacing;                      //!< pixels between top and bottom of text to background rectangle.
    float rowHeight;                            //!< height of background rectangle.
    float iconSize;                             //!< size of icon to match font.
    float pointSize;                            //!< size of the connection point.
    float pointSpacing;                         //!< spacing between pofloat columns.
    float pointToIcon;                          //!< spacing from last column points to first icon.
    float iconToIcon;                           //!< spacing between icons.
    float iconToText;                           //!< spacing between last icon and text.
    float rowPadding;                           //!< spaces added to rectangle bacground width ends.
    std::vector<QBrush> backgroundBrushes;      //!< brushes to paint background rectangles.
    std::vector<QBrush> forgroundBrushes;       //!< brushes to paint points, connectors, text.
    void setupViewConstants();
  //@}
//     
    RectItem *currentPrehighlight;
//     
//     enum class SelectionMode
//     {
//       Single,
//       Multiple
//     };
//     SelectionMode selectionMode;
//     std::vector<Vertex> getAllSelected();
//     void visiblyIsolate(Vertex sourceIn); //!< hide any connected feature and turn on sourceIn.
//     
//     QPointF lastPick;
//     bool lastPickValid = false;
//     
    QPixmap visiblePixmapEnabled;
    QPixmap visiblePixmapDisabled;
    QPixmap passPixmap;
    QPixmap failPixmap;
    QPixmap pendingPixmap;
//     
//     QAction *renameAction;
//     QAction *editingFinishedAction;
//     QGraphicsProxyWidget *proxy = nullptr;
//     void finishRename();
//     
//     //filters
//     void setupFilters();
//     typedef std::vector<std::shared_ptr<FilterBase> > FilterContainer;
//     FilterContainer filters;
  };
}

#endif // DAGMODEL_H

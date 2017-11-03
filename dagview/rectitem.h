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

#ifndef DAG_RECTITEM_H
#define DAG_RECTITEM_H

#include <QGraphicsRectItem>
#include <QBrush>

namespace dag
{
  /*all right I give up! the parenting combined with the zvalues is fubar!
    * you can't control any kind of layering between children of separate parents
    */
  class RectItem : public QGraphicsRectItem
  {
  public:
    RectItem(QGraphicsItem* parent = 0);
    void setBackgroundBrush(const QBrush &brushIn){backgroundBrush = brushIn;}
    void setEditingBrush(const QBrush &brushIn){editBrush = brushIn;}
    void preHighlightOn(){preSelected = true;}
    void preHighlightOff(){preSelected = false;}
    void selectionOn(){selected = true;}
    void selectionOff(){selected = false;}
    bool isSelected(){return selected;}
    bool isPreSelected(){return preSelected;}
    void editingStart(){editing = true;}
    void editingFinished(){editing = false;}
    bool isEditing(){return editing;}
  protected:
    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);
  private:
    QBrush backgroundBrush; //!< brush used for background. not used yet.
    QBrush editBrush; //!< brush used when object is in edit mode.
    //start with booleans, may expand to state.
    bool selected;
    bool preSelected;
    bool editing;
  };
}

#endif // DAG_RECTITEM_H

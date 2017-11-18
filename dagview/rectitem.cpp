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

#include <QPainter>
#include <QApplication>
#include <QStyleOptionViewItem>

#include <dagview/rectitem.h>

using namespace dag;

RectItem::RectItem(QGraphicsItem* parent) : QGraphicsRectItem(parent)
{
  selected = false;
  preSelected = false;
  editing = false;
}

void RectItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
  painter->save();
  
  QStyleOptionViewItem styleOption;
  
  styleOption.backgroundBrush = backgroundBrush;
  if (editing)
    styleOption.backgroundBrush = editBrush;
  else
  {
    styleOption.state |= QStyle::State_Enabled;
    if (selected)
      styleOption.state |= QStyle::State_Selected;
    if (preSelected)
    {
      if (!selected)
      {
        styleOption.state |= QStyle::State_Selected;
        QPalette palette = styleOption.palette;
        QColor tempColor = palette.color(QPalette::Active, QPalette::Highlight);
        tempColor.setAlphaF(0.15);
        palette.setColor(QPalette::Inactive, QPalette::Highlight, tempColor);
        styleOption.palette = palette;
      }
      styleOption.state |= QStyle::State_MouseOver;
    }
  }
  styleOption.rect = this->rect().toRect();
  
  QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &styleOption, painter);
  
  painter->restore();
}

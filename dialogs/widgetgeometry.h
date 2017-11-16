/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef DLG_WIDGETGEOMETRY_H
#define DLG_WIDGETGEOMETRY_H

#include <QObject>

class QWidget;

namespace dlg
{
  /*! Event filter to save restore widget geometry.*/
  class WidgetGeometry : public QObject
  {
    Q_OBJECT
  public:
    WidgetGeometry(QObject *parent, const QString &uniqueNameIn);
  protected:
    virtual bool eventFilter(QObject*, QEvent*) override;
    void saveSettings(QWidget*);
    void restoreSettings(QWidget*);
    QString uniqueName;
    
  };
}

#endif // DLG_WIDGETGEOMETRY_H

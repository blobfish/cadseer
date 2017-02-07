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

#ifndef DLG_EXPRESSIONEDIT_H
#define DLG_EXPRESSIONEDIT_H

#include <QWidget>
#include <QLabel>

class QResizeEvent;
class QLineEdit;
class QAction;

namespace dlg
{
  //! just a widget to show green, yellow, red on a traffic signal.
  class TrafficLabel : public QLabel
  {
    Q_OBJECT
  public:
    explicit TrafficLabel(QWidget *parent);
  public Q_SLOTS:
    void setTrafficRedSlot();
    void setTrafficYellowSlot();
    void setTrafficGreenSlot();
    void setLinkSlot();
  Q_SIGNALS:
    void requestUnlinkSignal();
  protected:
    void updatePixmaps();
    QPixmap buildPixmap(const QString&);
    virtual void resizeEvent(QResizeEvent *) override;
    
    int iconSize = 0;
    QPixmap trafficRed;
    QPixmap trafficYellow;
    QPixmap trafficGreen;
    QPixmap link;
    QAction *unlinkAction;
  };
  
  //! a widget with an edit line and traffic signal hooked up to expressions.
  class ExpressionEdit : public QWidget
  {
    Q_OBJECT
  public:
    explicit ExpressionEdit(QWidget *, Qt::WindowFlags f = 0);
    
    QLineEdit *lineEdit;
    TrafficLabel *trafficLabel;
    
  public Q_SLOTS:
    void goToolTipSlot(const QString&);
    
  protected:
    void setupGui();
  };
  
  //! a filter to do drag and drop for expression edit.
  class ExpressionEditFilter : public QObject
  {
    Q_OBJECT
  public:
    explicit ExpressionEditFilter(QObject *parent) : QObject(parent){}
    
  protected:
    virtual bool eventFilter(QObject *obj, QEvent *event) override;
    
  Q_SIGNALS:
    void requestLinkSignal(const QString&);
  };
}

#endif // DLG_EXPRESSIONEDIT_H

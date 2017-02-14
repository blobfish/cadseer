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

#ifndef APP_INCREMENTWIDGET_H
#define APP_INCREMENTWIDGET_H

#include <QWidgetAction>

class QLineEdit;

namespace dlg{class ExpressionEdit;}

namespace app
{
  class IncrementWidgetAction : public QWidgetAction
  {
    Q_OBJECT
  public:
    IncrementWidgetAction(QObject *parent, const QString &title1In, const QString &title2In);
    dlg::ExpressionEdit *lineEdit1;
    dlg::ExpressionEdit *lineEdit2;
  protected:
    virtual QWidget* createWidget(QWidget* parent) override;
    QString title1;
    QString title2;
    
  private Q_SLOTS:
    void textEditedSlot1(const QString&);
    void textEditedSlot2(const QString&);
    void editingFinishedSlot1();
    void editingFinishedSlot2();
    void returnPressedSlot1();
    void returnPressedSlot2();
    
  private:
    void textEditedCommon(const QString&, dlg::ExpressionEdit *);
    double editingFinishedCommon(dlg::ExpressionEdit *);
  };
  
  //! a filter to select text when editlines are picked.
  class HighlightOnFocusFilter : public QObject
  {
    Q_OBJECT
  public:
    explicit HighlightOnFocusFilter(QObject *parent) : QObject(parent){}
    
  protected:
    virtual bool eventFilter(QObject *obj, QEvent *event) override;
  };
}

#endif // APP_INCREMENTWIDGET_H

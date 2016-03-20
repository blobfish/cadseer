/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016  <copyright holder> <email>
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

namespace app
{
  class IncrementWidgetAction : public QWidgetAction
  {
  public:
    IncrementWidgetAction(QObject *parent, const QString &title1In, const QString &title2In);
    QLineEdit *lineEdit1;
    QLineEdit *lineEdit2;
  protected:
    virtual QWidget* createWidget(QWidget* parent) override;
    QString title1;
    QString title2;
  };
}

#endif // APP_INCREMENTWIDGET_H

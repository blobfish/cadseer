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

#include <QApplication>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QFontMetrics>

#include <application/incrementwidget.h>

using namespace app;

IncrementWidgetAction::IncrementWidgetAction(QObject* parent, const QString& title1In, const QString& title2In):
  QWidgetAction(parent), title1(title1In), title2(title2In)
{

}

QWidget* IncrementWidgetAction::createWidget(QWidget* parent)
{
  QWidget *out = new QWidget(parent);
  
  QHBoxLayout *hLayout = new QHBoxLayout();
  
  QVBoxLayout *vLayout1 = new QVBoxLayout();
  vLayout1->setSpacing(0);
  QLabel *label1 = new QLabel(title1, out);
  vLayout1->addWidget(label1);
  lineEdit1 = new QLineEdit(out);
  int width1 = QFontMetrics(QApplication::font()).boundingRect(title1).width();
  lineEdit1->setMaximumWidth(width1);
  vLayout1->addWidget(lineEdit1);
  vLayout1->setContentsMargins(0, 0, 0, 0);
  hLayout->addLayout(vLayout1);
  
  QVBoxLayout *vLayout2 = new QVBoxLayout();
  vLayout2->setSpacing(0);
  QLabel *label2 = new QLabel(title2, out);
  vLayout2->addWidget(label2);
  lineEdit2 = new QLineEdit(out);
  int width2 = QFontMetrics(QApplication::font()).boundingRect(title2).width();
  lineEdit2->setMaximumWidth(width2);
  vLayout2->addWidget(lineEdit2);
  vLayout2->setContentsMargins(0, 0, 0, 0);
  hLayout->addLayout(vLayout2);
  
  hLayout->addStretch();
  //this gets rid of the extra space from this widget and it's layout.
  //this needs to be done after widgets added.
  hLayout->setContentsMargins(0, 0, 0, 0);
  
  out->setLayout(hLayout);
  return out;
}

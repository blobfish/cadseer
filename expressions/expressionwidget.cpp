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

#include <QTabWidget>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QTextStream>
#include <QPushButton>

#include <expressions/expressionmanager.h>
#include <expressions/stringtranslator.h>
#include <expressions/scopedexpressionwidget.h>
#include <expressions/expressionwidget.h>

using namespace expr;

ExpressionWidget::ExpressionWidget(ExpressionManager &eManagerIn, QWidget* parent, Qt::WindowFlags f): QWidget(parent, f),
  eManager(eManagerIn)
{
  setupGui();
  this->setWindowTitle(tr("Expressions"));
  eManager.beginTransaction();
}

ExpressionWidget::~ExpressionWidget()
{
}

// void ExpressionWidget::accept()
// {
//   eManager.commitTransaction();
// }
// 
// void ExpressionWidget::reject()
// {
//   eManager.rejectTransaction();
// }
// 
// void ExpressionWidget::applySlot()
// {
//   eManager.commitTransaction();
//   eManager.beginTransaction();
// }

void ExpressionWidget::setupGui()
{
  QVBoxLayout *mainLayout = new QVBoxLayout();
  this->setLayout(mainLayout);
  
  ScopedExpressionWidget *subTab1 = new ScopedExpressionWidget(eManager, this);
  mainLayout->addWidget(subTab1);
}

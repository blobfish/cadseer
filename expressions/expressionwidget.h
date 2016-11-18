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

#ifndef EXPRESSIONWIDGET_H
#define EXPRESSIONWIDGET_H

#include <memory>

#include <boost/shared_ptr.hpp>

#include <QWidget>

class QTabWidget;

namespace expr
{
  class ExpressionManager;

/*! @brief Main widget for this test program
 */
class ExpressionWidget : public QWidget
{
  Q_OBJECT
public:
  explicit ExpressionWidget(ExpressionManager &eManagerIn, QWidget* parent = 0, Qt::WindowFlags f = 0);
  virtual ~ExpressionWidget();
private:
  //! Build up the GUI.
  void setupGui();
  //! Temp for testing, as this should live somewhere else and be passed directly to scoped widget.
  ExpressionManager &eManager;
};
}

#endif // EXPRESSIONWIDGET_H

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

#ifndef BOXDIALOG_H
#define BOXDIALOG_H

#include <QDialog>
#include <QLineEdit>

class BoxDialog : public QDialog
{
  Q_OBJECT
public:
  explicit BoxDialog(QWidget *parent = 0);
  
  QLineEdit *lengthEdit;
  QLineEdit *widthEdit;
  QLineEdit *heightEdit;
  
  void setLength(const double &lengthIn);
  void setWidth(const double &widthIn);
  void setHeight(const double &heightIn);
  void setParameters(const double &lengthIn, const double &widthIn, const double &heightIn);
  
private:
  void buildGui();
  QString cleanFormat(const double &valueIn);
};

#endif // BOXDIALOG_H

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

// #include "dagmodel.h"
#include "dagview.h"

using namespace DAG;

View::View(QWidget* parentIn): QGraphicsView(parentIn)
{
  this->setRenderHint(QPainter::Antialiasing, true);
  this->setRenderHint(QPainter::TextAntialiasing, true);
}

View::~View()
{
}

// #include "dagview/moc_dagview.cxx"

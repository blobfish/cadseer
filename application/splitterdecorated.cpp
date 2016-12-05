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

#include <QSettings>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <application/application.h>
#include <application/splitterdecorated.h>

SplitterDecorated::SplitterDecorated(QWidget *parent) : QSplitter(parent)
{

}

SplitterDecorated::~SplitterDecorated()
{
  saveSettings();
}

void SplitterDecorated::restoreSettings(const QString& nameIn)
{
  setObjectName(nameIn);
  QSettings &settings = static_cast<app::Application*>(qApp)->getUserSettings();
  settings.beginGroup(objectName());
  restoreState(settings.value("splitterSizes").toByteArray());
  settings.endGroup();
}

void SplitterDecorated::saveSettings()
{
  QSettings &settings = static_cast<app::Application*>(qApp)->getUserSettings();
  
  settings.beginGroup(objectName());
  settings.setValue("splitterSizes", saveState());
  settings.endGroup();
}

//come on Qt! just add any kind visual cue
//http://stackoverflow.com/questions/2545577/qsplitter-becoming-undistinguishable-between-qwidget-and-qtabwidget
QSplitterHandle* SplitterDecorated::createHandle()
{
  QSplitterHandle *out = QSplitter::createHandle(); //assuming parenting is done.
  
  Q_ASSERT(out != NULL);

  const int gripLength = 15; 
  const int gripWidth = 1;
  const int grips = 3;

  setHandleWidth(7);
  Qt::Orientation orientation = QSplitter::orientation();
  QHBoxLayout* layout = new QHBoxLayout(out);
  layout->setSpacing(0);
  layout->setMargin(0);

  if (orientation == Qt::Horizontal)
  {
      for (int i=0;i<grips;++i)
      {
	  QFrame* line = new QFrame(out);
	  line->setMinimumSize(gripWidth, gripLength);
	  line->setMaximumSize(gripWidth, gripLength);
	  line->setFrameShape(QFrame::VLine);
	  line->setFrameShadow(QFrame::Sunken);
	  layout->addWidget(line);
      }
  }
  else
  {
      //this will center the vertical grip
      //add a horizontal spacer
      layout->addStretch();
      //create the vertical grip
      QVBoxLayout* vbox = new QVBoxLayout;
      for (int i=0;i<grips;++i)
      {
	  QFrame* line = new QFrame(out);
	  line->setMinimumSize(gripLength, gripWidth);
	  line->setMaximumSize(gripLength, gripWidth);
	  line->setFrameShape(QFrame::HLine);
	  line->setFrameShadow(QFrame::Sunken);
	  vbox->addWidget(line);
      }
      layout->addLayout(vbox);
      //add another horizontal spacer
      layout->addStretch();
  }
  
  return out;
}

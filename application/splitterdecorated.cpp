/*
 * Copyright 2016 <copyright holder> <email>
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

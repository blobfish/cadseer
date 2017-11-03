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

#include <cassert>

#include <boost/filesystem.hpp>

#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTimer>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QDragLeaveEvent>
#include <QButtonGroup>
#include <QCheckBox>

#include <application/application.h>
#include <application/mainwindow.h>
#include <project/project.h>
#include <message/observer.h>
#include <feature/strip.h>
#include <dialogs/widgetgeometry.h>
#include <dialogs/selectionbutton.h>
#include <dialogs/strip.h>

using namespace dlg;

using boost::uuids::uuid;

TrashCan::TrashCan(QWidget *parent)
  : QLabel(parent)
{
  setMinimumSize(128, 128);
  setFrameStyle(QFrame::Sunken | QFrame::StyledPanel);
  setAlignment(Qt::AlignCenter);
  setAcceptDrops(true);
  setAutoFillBackground(true);
  
  setPixmap(QPixmap(":resources/images/dagViewRemove.svg").scaled(128, 128, Qt::KeepAspectRatio));
}

void TrashCan::dragEnterEvent(QDragEnterEvent *event)
{
  setBackgroundRole(QPalette::Highlight);
  event->acceptProposedAction();
}

void TrashCan::dragMoveEvent(QDragMoveEvent *event)
{
  event->acceptProposedAction();
}

void TrashCan::dropEvent(QDropEvent *event)
{
  setBackgroundRole(QPalette::Dark);
  event->acceptProposedAction();
}

void TrashCan::dragLeaveEvent(QDragLeaveEvent *event)
{
  setBackgroundRole(QPalette::Dark);
  event->accept();
}



Strip::Strip(ftr::Strip *stripIn, QWidget *parent) : QDialog(parent)
{
  observer = std::unique_ptr<msg::Observer>(new msg::Observer());
  observer->name = "dlg::Strip";
  
  strip = stripIn;
  
  buildGui();
  initGui();
  
  WidgetGeometry *filter = new WidgetGeometry(this, "dlg::Strip");
  this->installEventFilter(filter);
}

Strip::~Strip()
{
}

void Strip::reject()
{
  isAccepted = false;
  finishDialog();
  QDialog::reject();
}

void Strip::accept()
{
  isAccepted = true;
  finishDialog();
  QDialog::accept();
}

void Strip::finishDialog()
{
  if (isAccepted)
  {
    strip->setModelDirty(); // several values are not parameters
    
    if (acCheckBox->isChecked())
      strip->setAutoCalc(true);
    else
      strip->setAutoCalc(false);
    
    strip->stations.clear();
    for (int i = 0; i < stationsList->count(); ++i)
      strip->stations.push_back(stationsList->item(i)->text());
    
    //upate graph connections
    prj::Project *p = static_cast<app::Application *>(qApp)->getProject();
    
    auto updateTag = [&](QLabel* label, const std::string &sIn)
    {
      p->removeParentTag(strip->getId(), sIn);
      uuid labelId = gu::stringToId(label->text().toStdString());
      if(!labelId.is_nil() && p->hasFeature(labelId))
        p->connect(labelId, strip->getId(), {sIn});
    };
    
    updateTag(partIdLabel, ftr::Strip::part);
    updateTag(blankIdLabel, ftr::Strip::blank);
    updateTag(nestIdLabel, ftr::Strip::nest);
  }
  
  static_cast<app::Application *>(qApp)->messageSlot(msg::Mask(msg::Request | msg::Command | msg::Done));
}

void Strip::initGui()
{
  if (strip->isAutoCalc())
    acCheckBox->setChecked(true);
  else
    acCheckBox->setChecked(false);
  
  for (const auto &s : strip->stations)
    stationsList->addItem(s);
  
  QString nilText = QString::fromStdString(gu::idToString(gu::createNilId()));
  partIdLabel->setText(nilText);
  blankIdLabel->setText(nilText);
  nestIdLabel->setText(nilText);
  
  prj::Project *p = static_cast<app::Application *>(qApp)->getProject();
  ftr::UpdatePayload::UpdateMap pMap = p->getParentMap(strip->getId());
  for (const auto &it : pMap)
  {
    if (it.first == ftr::Strip::part)
      setPartId(it.second->getId());
    if (it.first == ftr::Strip::blank)
      setBlankId(it.second->getId());
    if (it.first == ftr::Strip::nest)
      setNestId(it.second->getId());
  }
}

void Strip::buildGui()
{
  tabWidget = new QTabWidget(this);
  tabWidget->addTab(buildInputPage(), tr("Input"));
  tabWidget->addTab(buildStationPage(), tr("Stations"));
  
  QDialogButtonBox *buttons = new QDialogButtonBox
    (QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
  QHBoxLayout *buttonLayout = new QHBoxLayout();
  buttonLayout->addStretch();
  buttonLayout->addWidget(buttons);
  connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
  
  QVBoxLayout *vl = new QVBoxLayout();
  vl->addWidget(tabWidget);
  vl->addWidget(buttons);
  this->setLayout(vl);
}

QWidget* Strip::buildInputPage()
{
  QWidget *out = new QWidget(tabWidget);
  
  QHBoxLayout *acLayout = new QHBoxLayout();
  acCheckBox = new QCheckBox(tr("Auto Calculation"), out);
  acLayout->addWidget(acCheckBox);
  acLayout->addStretch();
  
  QGridLayout *gl = new QGridLayout();
  bGroup = new QButtonGroup(out);
  QString nilString(QString::fromStdString(gu::idToString(gu::createNilId())));
  QPixmap pmap = QPixmap(":resources/images/cursor.svg").scaled(32, 32, Qt::KeepAspectRatio);
  
  QLabel *partLabel = new QLabel(tr("Part:"), out);
  partButton = new SelectionButton(pmap, QString(), out); //switch to icon
  partButton->isSingleSelection = true;
  partButton->mask = ~slc::All | slc::ObjectsEnabled | slc::ObjectsSelectable;
  partIdLabel = new QLabel(nilString, out);
  gl->addWidget(partLabel, 0, 0, Qt::AlignVCenter | Qt::AlignRight);
  gl->addWidget(partButton, 0, 1, Qt::AlignVCenter | Qt::AlignCenter);
  gl->addWidget(partIdLabel, 0, 2, Qt::AlignVCenter | Qt::AlignLeft);
  connect(partButton, &SelectionButton::dirty, this, &Strip::updatePartIdSlot);
  connect(partButton, &SelectionButton::advance, this, &Strip::advanceSlot);
  bGroup->addButton(partButton);
  
  QLabel *blankLabel = new QLabel(tr("Blank:"), out);
  blankButton = new SelectionButton(pmap, QString(), out);
  blankButton->isSingleSelection = true;
  blankButton->mask = ~slc::All | slc::ObjectsEnabled | slc::ObjectsSelectable;
  blankIdLabel = new QLabel(nilString, out);
  gl->addWidget(blankLabel, 1, 0, Qt::AlignVCenter | Qt::AlignRight);
  gl->addWidget(blankButton, 1, 1, Qt::AlignVCenter | Qt::AlignCenter);
  gl->addWidget(blankIdLabel, 1, 2, Qt::AlignVCenter | Qt::AlignLeft);
  connect(blankButton, &SelectionButton::dirty, this, &Strip::updateBlankIdSlot);
  connect(blankButton, &SelectionButton::advance, this, &Strip::advanceSlot);
  bGroup->addButton(blankButton);
  
  QLabel *nestLabel = new QLabel(tr("Nest:"), out);
  nestButton = new SelectionButton(pmap, QString(), out);
  nestButton->isSingleSelection = true;
  nestButton->mask = ~slc::All | slc::ObjectsEnabled | slc::ObjectsSelectable;
  nestIdLabel = new QLabel(nilString, out);
  gl->addWidget(nestLabel, 2, 0, Qt::AlignVCenter | Qt::AlignRight);
  gl->addWidget(nestButton, 2, 1, Qt::AlignVCenter | Qt::AlignCenter);
  gl->addWidget(nestIdLabel, 2, 2, Qt::AlignVCenter | Qt::AlignLeft);
  connect(nestButton, &SelectionButton::dirty, this, &Strip::updateNestIdSlot);
  connect(nestButton, &SelectionButton::advance, this, &Strip::advanceSlot);
  bGroup->addButton(nestButton);
  
  QHBoxLayout *hl = new QHBoxLayout();
  hl->addLayout(gl);
  hl->addStretch();
  
  QVBoxLayout *vl = new QVBoxLayout();
  vl->addLayout(acLayout);
  vl->addSpacing(10);
  vl->addLayout(hl);
  vl->addStretch();
  
  out->setLayout(vl);
  
  return out;
}

QWidget* Strip::buildStationPage()
{
  QWidget *out = new QWidget(tabWidget);
  
  QHBoxLayout *hl = new QHBoxLayout();
  out->setLayout(hl);
  
  QVBoxLayout *vl = new QVBoxLayout();
  hl->addLayout(vl);
  
  stationsList = new QListWidget(out);
  stationsList->setSelectionMode(QAbstractItemView::SingleSelection);
  stationsList->setDragEnabled(true);
  stationsList->setAcceptDrops(true);
  stationsList->setDropIndicatorShown(true);
  stationsList->setDragDropMode(QAbstractItemView::DragDrop);
  stationsList->setDefaultDropAction(Qt::MoveAction);
  vl->addWidget(stationsList);
  
  TrashCan *trash = new TrashCan(out);
  vl->addWidget(trash);
  
  QListWidget *protoList = new QListWidget(out);
  protoList->addItem(tr("Aerial Cam"));
  protoList->addItem(tr("Blank"));
  protoList->addItem(tr("Cam"));
  protoList->addItem(tr("Coin"));
  protoList->addItem(tr("Cutoff"));
  protoList->addItem(tr("Draw"));
  protoList->addItem(tr("Flange"));
  protoList->addItem(tr("Form"));
  protoList->addItem(tr("Idle"));
  protoList->addItem(tr("Pierce"));
  protoList->addItem(tr("Restrike"));
  protoList->addItem(tr("Tip"));
  protoList->addItem(tr("Trim"));
  hl->addWidget(protoList);
  protoList->setSelectionMode(QAbstractItemView::SingleSelection);
  protoList->setDragEnabled(true);
  protoList->setDragDropMode(QAbstractItemView::DragOnly);
  
  
  return out;
}

void Strip::setPartId(const uuid &idIn)
{
  if (idIn.is_nil())
    return;
  
  slc::Message m;
  m.type = slc::Type::Object;
  m.featureId = idIn;
  
  partButton->setMessages(m);
}

void Strip::setBlankId(const uuid &idIn)
{
  if (idIn.is_nil())
    return;
  
  slc::Message m;
  m.type = slc::Type::Object;
  m.featureId = idIn;
  
  blankButton->setMessages(m);
}

void Strip::setNestId(const uuid &idIn)
{
  if (idIn.is_nil())
    return;
  
  slc::Message m;
  m.type = slc::Type::Object;
  m.featureId = idIn;
  
  nestButton->setMessages(m);
}

void Strip::updatePartIdSlot()
{
  const slc::Messages &ms = partButton->getMessages();
  if (ms.empty())
    partIdLabel->setText(QString::fromStdString(gu::idToString(gu::createNilId())));
  else
    partIdLabel->setText(QString::fromStdString(gu::idToString(ms.front().featureId)));
}

void Strip::updateBlankIdSlot()
{
  const slc::Messages &ms = blankButton->getMessages();
  if (ms.empty())
    blankIdLabel->setText(QString::fromStdString(gu::idToString(gu::createNilId())));
  else
    blankIdLabel->setText(QString::fromStdString(gu::idToString(ms.front().featureId)));
}

void Strip::updateNestIdSlot()
{
  const slc::Messages &ms = nestButton->getMessages();
  if (ms.empty())
    nestIdLabel->setText(QString::fromStdString(gu::idToString(gu::createNilId())));
  else
    nestIdLabel->setText(QString::fromStdString(gu::idToString(ms.front().featureId)));
}

void Strip::advanceSlot()
{
  QAbstractButton *cb = bGroup->checkedButton();
  if (!cb)
    return;
  if (cb == partButton)
    blankButton->setChecked(true);
  else if (cb == blankButton)
    nestButton->setChecked(true);
  else if (cb == nestButton)
    partButton->setChecked(true);
  else
  {
    assert(0); //no button checked.
    throw std::runtime_error("no button in dlg::Strip::advanceSlot");
  }
}

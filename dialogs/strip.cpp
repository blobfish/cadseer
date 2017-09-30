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
#include <QLineEdit>
#include <QComboBox>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTimer>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QDragLeaveEvent>

#include <application/application.h>
#include <application/mainwindow.h>
#include <project/project.h>
#include <viewer/viewerwidget.h>
#include <message/observer.h>
#include <feature/strip.h>
#include <dialogs/strip.h>

using namespace dlg;



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
  clear();
  event->accept();
}



Strip::Strip(ftr::Strip *stripIn, QWidget *parent) : QDialog(parent)
{
  observer = std::unique_ptr<msg::Observer>(new msg::Observer());
  observer->name = "dlg::Strip";
  
  strip = stripIn;
  
  buildGui();
  initGui();
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
    strip->stripData.partName = pNameEdit->text();
    strip->stripData.partNumber = pNumberEdit->text();
    strip->stripData.partRevision = pRevisionEdit->text();
    strip->stripData.materialType = pmTypeEdit->text();
    strip->stripData.materialThickness = pmThicknessEdit->text().toDouble();
    strip->stripData.quoteNumber = sQuoteNumberEdit->text().toInt();
    strip->stripData.customerName = sCustomerNameEdit->text();
    strip->stripData.partSetup = sPartSetupCombo->currentText();
    strip->stripData.processType = sProcessTypeCombo->currentText();
    strip->stripData.annualVolume = sAnnualVolumeEdit->text().toInt();
    
    strip->stripData.stations.clear();
    for (int i = 0; i < stationsList->count(); ++i)
      strip->stripData.stations.push_back(stationsList->item(i)->text());
  }
  
  static_cast<app::Application *>(qApp)->messageSlot(msg::Mask(msg::Request | msg::Command | msg::Done));
}

void Strip::initGui()
{
  pNameEdit->setText(strip->stripData.partName);
  pNumberEdit->setText(strip->stripData.partNumber);
  pRevisionEdit->setText(strip->stripData.partRevision);
  pmTypeEdit->setText(strip->stripData.materialType);
  pmThicknessEdit->setText(QString::number(strip->stripData.materialThickness));
  sQuoteNumberEdit->setText(QString::number(strip->stripData.quoteNumber));
  sCustomerNameEdit->setText(strip->stripData.customerName);
  sPartSetupCombo->setCurrentText(strip->stripData.partSetup);
  sProcessTypeCombo->setCurrentText(strip->stripData.processType);
  sAnnualVolumeEdit->setText(QString::number(strip->stripData.annualVolume));
  
  for (const auto &s : strip->stripData.stations)
    stationsList->addItem(s);
  
  loadLabelPixmapSlot();
}

void Strip::loadLabelPixmapSlot()
{
  pLabel->setText(strip->stripData.picturePath + " not found");
  QPixmap pMap(strip->stripData.picturePath);
  if (!pMap.isNull())
  {
    QPixmap scaled = pMap.scaledToHeight(pMap.height() / 2.0);
    pLabel->setPixmap(scaled);
  }
}

void Strip::buildGui()
{
  tabWidget = new QTabWidget(this);
  tabWidget->addTab(buildInputPage(), tr("Input"));
  tabWidget->addTab(buildPartPage(), tr("Part"));
  tabWidget->addTab(buildStripPage(), tr("Strip"));
  tabWidget->addTab(buildStationPage(), tr("Station"));
  tabWidget->addTab(buildPicturePage(), tr("Picture"));
  
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
  
  return out;
}

QWidget* Strip::buildPartPage()
{
  QWidget *out = new QWidget(tabWidget);
  
  QGridLayout *gl = new QGridLayout();
  
  QLabel *pNameLabel = new QLabel(tr("Part Name:"), out);
  pNameEdit = new QLineEdit(out);
  gl->addWidget(pNameLabel, 0, 0, Qt::AlignVCenter | Qt::AlignRight);
  gl->addWidget(pNameEdit, 0, 1, Qt::AlignVCenter | Qt::AlignLeft);
  
  QLabel *pNumberLabel = new QLabel(tr("Part Number:"), out);
  pNumberEdit = new QLineEdit(out);
  gl->addWidget(pNumberLabel, 1, 0, Qt::AlignVCenter | Qt::AlignRight);
  gl->addWidget(pNumberEdit, 1, 1, Qt::AlignVCenter | Qt::AlignLeft);
  
  QLabel *pRevisionLabel = new QLabel(tr("Part Revision:"), out);
  pRevisionEdit = new QLineEdit(out);
  gl->addWidget(pRevisionLabel, 2, 0, Qt::AlignVCenter | Qt::AlignRight);
  gl->addWidget(pRevisionEdit, 2, 1, Qt::AlignVCenter | Qt::AlignLeft);
  
  QLabel *pmTypeLabel = new QLabel(tr("Material Type:"), out);
  pmTypeEdit = new QLineEdit(out);
  gl->addWidget(pmTypeLabel, 3, 0, Qt::AlignVCenter | Qt::AlignRight);
  gl->addWidget(pmTypeEdit, 3, 1, Qt::AlignVCenter | Qt::AlignLeft);
  
  QLabel *pmThicknessLabel = new QLabel(tr("Material Thickness:"), out);
  pmThicknessEdit = new QLineEdit(out);
  gl->addWidget(pmThicknessLabel, 4, 0, Qt::AlignVCenter | Qt::AlignRight);
  gl->addWidget(pmThicknessEdit, 4, 1, Qt::AlignVCenter | Qt::AlignLeft);
  
  QHBoxLayout *hl = new QHBoxLayout();
  hl->addLayout(gl);
  hl->addStretch();
  
  QVBoxLayout *vl = new QVBoxLayout();
  vl->addLayout(hl);
  vl->addStretch();
  
  out->setLayout(vl);
  
  return out;
}

QWidget* Strip::buildStripPage()
{
  QWidget *out = new QWidget(tabWidget);
  
  QGridLayout *gl = new QGridLayout();
  
  QLabel *sQuoteNumberLabel = new QLabel(tr("Quote Number:"), out);
  sQuoteNumberEdit = new QLineEdit(out);
  gl->addWidget(sQuoteNumberLabel, 0, 0, Qt::AlignVCenter | Qt::AlignRight);
  gl->addWidget(sQuoteNumberEdit, 0, 1, Qt::AlignVCenter | Qt::AlignLeft);
  
  QLabel *sCustomerNameLabel = new QLabel(tr("Customer Name:"), out);
  sCustomerNameEdit = new QLineEdit(out);
  gl->addWidget(sCustomerNameLabel, 1, 0, Qt::AlignVCenter | Qt::AlignRight);
  gl->addWidget(sCustomerNameEdit, 1, 1, Qt::AlignVCenter | Qt::AlignLeft);
  
  QLabel *sPartSetupLabel = new QLabel(tr("Part Setup:"), out);
  sPartSetupCombo = new QComboBox(out);
  sPartSetupCombo->setEditable(true);
  sPartSetupCombo->setInsertPolicy(QComboBox::NoInsert);
  sPartSetupCombo->addItem(tr("One Out"));
  sPartSetupCombo->addItem(tr("Two Out"));
  sPartSetupCombo->addItem(tr("Sym Opposite"));
  gl->addWidget(sPartSetupLabel, 2, 0, Qt::AlignVCenter | Qt::AlignRight);
  gl->addWidget(sPartSetupCombo, 2, 1, Qt::AlignVCenter | Qt::AlignLeft);
  
  QLabel *sProcessTypeLabel = new QLabel(tr("Process Type:"), out);
  sProcessTypeCombo = new QComboBox(out);
  sProcessTypeCombo->setEditable(true);
  sProcessTypeCombo->setInsertPolicy(QComboBox::NoInsert);
  sProcessTypeCombo->addItem(tr("Prog"));
  sProcessTypeCombo->addItem(tr("Prog Partial"));
  sProcessTypeCombo->addItem(tr("Mech Transfer"));
  sProcessTypeCombo->addItem(tr("Hand Transfer"));
  gl->addWidget(sProcessTypeLabel, 3, 0, Qt::AlignVCenter | Qt::AlignRight);
  gl->addWidget(sProcessTypeCombo, 3, 1, Qt::AlignVCenter | Qt::AlignLeft);
  
  QLabel *sAnnualVolumeLabel = new QLabel(tr("Annual Volume:"), out);
  sAnnualVolumeEdit = new QLineEdit(out);
  gl->addWidget(sAnnualVolumeLabel, 4, 0, Qt::AlignVCenter | Qt::AlignRight);
  gl->addWidget(sAnnualVolumeEdit, 4, 1, Qt::AlignVCenter | Qt::AlignLeft);
  
  QHBoxLayout *hl = new QHBoxLayout();
  hl->addLayout(gl);
  hl->addStretch();
  
  QVBoxLayout *vl = new QVBoxLayout();
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

QWidget* Strip::buildPicturePage()
{
  QWidget *out = new QWidget(tabWidget);
  
  QVBoxLayout *vl = new QVBoxLayout();
  out->setLayout(vl);
  
  QHBoxLayout *bl = new QHBoxLayout();
  vl->addLayout(bl);
  QPushButton *ppb = new QPushButton(tr("Take Picture"), out);
  bl->addWidget(ppb);
  bl->addStretch();
  connect(ppb, &QPushButton::clicked, this, &Strip::takePictureSlot);
  
  pLabel = new QLabel(out);
  pLabel->setScaledContents(true);
  vl->addWidget(pLabel);
  
  return out;
}

void Strip::takePictureSlot()
{
  /* the osg screen capture handler is designed to automatically
   * add indexes to the file names. We have to work around that here.
   * it will add '_0' to the filename we give it before the dot and extension
   */
  
  namespace bfs = boost::filesystem;
  
  app::Application *app = static_cast<app::Application *>(qApp);
  bfs::path pp = (app->getProject()->getSaveDirectory());
  assert(bfs::exists(pp));
  bfs::path fp = pp /= gu::idToString(strip->getId());
  std::string ext = "png";
  bfs::path fullPath = fp.string() + "_0." + ext;
  
  strip->stripData.picturePath = QString::fromStdString(fullPath.string());
  
  app->getMainWindow()->getViewer()->screenCapture(fp.string(), ext);
  
  QTimer::singleShot(1000, this, SLOT(loadLabelPixmapSlot()));
}

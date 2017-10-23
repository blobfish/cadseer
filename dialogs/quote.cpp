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
#include <QDialogButtonBox>
#include <QPushButton>
#include <QButtonGroup>
#include <QTimer>
#include <QFileDialog>

#include <application/application.h>
#include <application/mainwindow.h>
#include <project/project.h>
#include <viewer/viewerwidget.h>
#include <message/observer.h>
#include <feature/quote.h>
#include <dialogs/widgetgeometry.h>
#include <dialogs/selectionbutton.h>
#include <dialogs/quote.h>

using namespace dlg;

using boost::uuids::uuid;

Quote::Quote(ftr::Quote *quoteIn, QWidget *parent) : QDialog(parent)
{
  observer = std::unique_ptr<msg::Observer>(new msg::Observer());
  observer->name = "dlg::Quote";
  
  quote = quoteIn;
  
  buildGui();
  initGui();
  
  WidgetGeometry *filter = new WidgetGeometry(this, "dlg::Quote");
  this->installEventFilter(filter);
}

Quote::~Quote()
{
}

void Quote::reject()
{
  isAccepted = false;
  finishDialog();
  QDialog::reject();
}

void Quote::accept()
{
  isAccepted = true;
  finishDialog();
  QDialog::accept();
}

void Quote::finishDialog()
{
  if (isAccepted)
  {
    quote->setModelDirty();
    
    quote->quoteData.partName = pNameEdit->text();
    quote->quoteData.partNumber = pNumberEdit->text();
    quote->quoteData.partRevision = pRevisionEdit->text();
    quote->quoteData.materialType = pmTypeEdit->text();
    quote->quoteData.materialThickness = pmThicknessEdit->text().toDouble();
    quote->quoteData.quoteNumber = sQuoteNumberEdit->text().toInt();
    quote->quoteData.customerName = sCustomerNameEdit->text();
    quote->quoteData.partSetup = sPartSetupCombo->currentText();
    quote->quoteData.processType = sProcessTypeCombo->currentText();
    quote->quoteData.annualVolume = sAnnualVolumeEdit->text().toInt();
    
    quote->tFile.setValue(boost::filesystem::path(tFileEdit->text().toStdString()));
    quote->oFile.setValue(boost::filesystem::path(oFileEdit->text().toStdString()));
    
    //upate graph connections
    prj::Project *p = static_cast<app::Application *>(qApp)->getProject();
    
    auto updateTag = [&](QLabel* label, const std::string &sIn)
    {
      p->removeParentTag(quote->getId(), sIn);
      uuid labelId = gu::stringToId(label->text().toStdString());
      if(!labelId.is_nil() && p->hasFeature(labelId))
        p->connect(labelId, quote->getId(), {sIn});
    };
    
    updateTag(stripIdLabel, ftr::Quote::strip);
    updateTag(diesetIdLabel, ftr::Quote::dieSet);
  }
  
  static_cast<app::Application *>(qApp)->messageSlot(msg::Mask(msg::Request | msg::Command | msg::Done));
}

void Quote::initGui()
{
  pNameEdit->setText(quote->quoteData.partName);
  pNumberEdit->setText(quote->quoteData.partNumber);
  pRevisionEdit->setText(quote->quoteData.partRevision);
  pmTypeEdit->setText(quote->quoteData.materialType);
  pmThicknessEdit->setText(QString::number(quote->quoteData.materialThickness));
  sQuoteNumberEdit->setText(QString::number(quote->quoteData.quoteNumber));
  sCustomerNameEdit->setText(quote->quoteData.customerName);
  sPartSetupCombo->setCurrentText(quote->quoteData.partSetup);
  sProcessTypeCombo->setCurrentText(quote->quoteData.processType);
  sAnnualVolumeEdit->setText(QString::number(quote->quoteData.annualVolume));
  
  loadLabelPixmapSlot();
  
  tFileEdit->setText(QString::fromStdString(static_cast<boost::filesystem::path>(quote->tFile).string()));
  oFileEdit->setText(QString::fromStdString(static_cast<boost::filesystem::path>(quote->oFile).string()));
  
  prj::Project *p = static_cast<app::Application *>(qApp)->getProject();
  ftr::UpdatePayload::UpdateMap pMap = p->getParentMap(quote->getId());
  for (const auto &it : pMap)
  {
    if (it.first == ftr::Quote::strip)
      stripIdLabel->setText(QString::fromStdString(gu::idToString(it.second->getId())));
    if (it.first == ftr::Quote::dieSet)
      diesetIdLabel->setText(QString::fromStdString(gu::idToString(it.second->getId())));
  }
}

void Quote::loadLabelPixmapSlot()
{
  pLabel->setText(QString::fromStdString(quote->pFile.string() + " not found"));
  QPixmap pMap(quote->pFile.c_str());
  if (!pMap.isNull())
  {
    QPixmap scaled = pMap.scaledToHeight(pMap.height() / 2.0);
    pLabel->setPixmap(scaled);
  }
}

void Quote::buildGui()
{
  tabWidget = new QTabWidget(this);
  tabWidget->addTab(buildInputPage(), tr("Input"));
  tabWidget->addTab(buildPartPage(), tr("Part"));
  tabWidget->addTab(buildQuotePage(), tr("Quote"));
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

QWidget* Quote::buildInputPage()
{
  QWidget *out = new QWidget(tabWidget);
  
  QGridLayout *gl = new QGridLayout();
  bGroup = new QButtonGroup(out);
  QString nilString(QString::fromStdString(gu::idToString(gu::createNilId())));
  QPixmap pmap = QPixmap(":resources/images/cursor.svg").scaled(32, 32, Qt::KeepAspectRatio);
  
  QLabel *stripLabel = new QLabel(tr("Strip:"), out);
  stripButton = new SelectionButton(pmap, QString(), out); //switch to icon
  stripButton->isSingleSelection = true;
  stripButton->mask = ~slc::All | slc::ObjectsEnabled | slc::ObjectsSelectable;
  stripIdLabel = new QLabel(nilString, out);
  gl->addWidget(stripLabel, 0, 0, Qt::AlignVCenter | Qt::AlignRight);
  gl->addWidget(stripButton, 0, 1, Qt::AlignVCenter | Qt::AlignCenter);
  gl->addWidget(stripIdLabel, 0, 2, Qt::AlignVCenter | Qt::AlignLeft);
  connect(stripButton, &SelectionButton::dirty, this, &Quote::updateStripIdSlot);
  connect(stripButton, &SelectionButton::advance, this, &Quote::advanceSlot);
  bGroup->addButton(stripButton);
  
  QLabel *diesetLabel = new QLabel(tr("Die Set:"), out);
  diesetButton = new SelectionButton(pmap, QString(), out);
  diesetButton->isSingleSelection = true;
  diesetButton->mask = ~slc::All | slc::ObjectsEnabled | slc::ObjectsSelectable;
  diesetIdLabel = new QLabel(nilString, out);
  gl->addWidget(diesetLabel, 1, 0, Qt::AlignVCenter | Qt::AlignRight);
  gl->addWidget(diesetButton, 1, 1, Qt::AlignVCenter | Qt::AlignCenter);
  gl->addWidget(diesetIdLabel, 1, 2, Qt::AlignVCenter | Qt::AlignLeft);
  connect(diesetButton, &SelectionButton::dirty, this, &Quote::updateDiesetIdSlot);
  connect(diesetButton, &SelectionButton::advance, this, &Quote::advanceSlot);
  bGroup->addButton(diesetButton);
  
  //bft = browse for template.
  QLabel *bftLabel = new QLabel(tr("Template Sheet:"), out);
  QPushButton *bftButton = new QPushButton(tr("Browse"), out);
  tFileEdit = new QLineEdit(out);
  gl->addWidget(bftLabel, 2, 0, Qt::AlignVCenter | Qt::AlignRight);
  gl->addWidget(bftButton, 2, 1, Qt::AlignVCenter | Qt::AlignCenter);
  gl->addWidget(tFileEdit, 2, 2);
  connect(bftButton, &QPushButton::clicked, this, &Quote::browseForTemplateSlot);
  
  //bfo = browse for output.
  QLabel *bfoLabel = new QLabel(tr("Output Sheet:"), out);
  QPushButton *bfoButton = new QPushButton(tr("Browse"), out);
  oFileEdit = new QLineEdit(out);
  gl->addWidget(bfoLabel, 3, 0, Qt::AlignVCenter | Qt::AlignRight);
  gl->addWidget(bfoButton, 3, 1, Qt::AlignVCenter | Qt::AlignCenter);
  gl->addWidget(oFileEdit, 3, 2);
  connect(bfoButton, &QPushButton::clicked, this, &Quote::browseForOutputSlot);
  
  QHBoxLayout *hl = new QHBoxLayout();
  hl->addLayout(gl);
  hl->addStretch();
  
  QVBoxLayout *vl = new QVBoxLayout();
  vl->addLayout(hl);
  vl->addStretch();
  
  out->setLayout(vl);
  
  return out;
}

QWidget* Quote::buildPartPage()
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

QWidget* Quote::buildQuotePage()
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

QWidget* Quote::buildPicturePage()
{
  QWidget *out = new QWidget(tabWidget);
  
  QVBoxLayout *vl = new QVBoxLayout();
  out->setLayout(vl);
  
  QHBoxLayout *bl = new QHBoxLayout();
  vl->addLayout(bl);
  QPushButton *ppb = new QPushButton(tr("Take Picture"), out);
  bl->addWidget(ppb);
  bl->addStretch();
  connect(ppb, &QPushButton::clicked, this, &Quote::takePictureSlot);
  
  pLabel = new QLabel(out);
  pLabel->setScaledContents(true);
  vl->addWidget(pLabel);
  
  return out;
}

void Quote::takePictureSlot()
{
  /* the osg screen capture handler is designed to automatically
   * add indexes to the file names. We have to work around that here.
   * it will add '_0' to the filename we give it before the dot and extension
   */
  
  namespace bfs = boost::filesystem;
  
  app::Application *app = static_cast<app::Application *>(qApp);
  bfs::path pp = (app->getProject()->getSaveDirectory());
  assert(bfs::exists(pp));
  bfs::path fp = pp /= gu::idToString(quote->getId());
  std::string ext = "png";
  bfs::path fullPath = fp.string() + "_0." + ext;
  
  quote->pFile = fullPath;
  
  app->getMainWindow()->getViewer()->screenCapture(fp.string(), ext);
  
  QTimer::singleShot(1000, this, SLOT(loadLabelPixmapSlot()));
}

void Quote::setStripId(const uuid &idIn)
{
  if (idIn.is_nil())
    return;
  
  slc::Message m;
  m.type = slc::Type::Object;
  m.featureId = idIn;
  
  stripButton->setMessages(m);
}

void Quote::setDieSetId(const uuid &idIn)
{
  if (idIn.is_nil())
    return;
  
  slc::Message m;
  m.type = slc::Type::Object;
  m.featureId = idIn;
  
  diesetButton->setMessages(m);
}

void Quote::updateStripIdSlot()
{
  const slc::Messages &ms = stripButton->getMessages();
  if (ms.empty())
    stripIdLabel->setText(QString::fromStdString(gu::idToString(gu::createNilId())));
  else
    stripIdLabel->setText(QString::fromStdString(gu::idToString(ms.front().featureId)));
}

void Quote::updateDiesetIdSlot()
{
  const slc::Messages &ms = diesetButton->getMessages();
  if (ms.empty())
    diesetIdLabel->setText(QString::fromStdString(gu::idToString(gu::createNilId())));
  else
    diesetIdLabel->setText(QString::fromStdString(gu::idToString(ms.front().featureId)));
}

void Quote::advanceSlot()
{
  QAbstractButton *cb = bGroup->checkedButton();
  if (!cb)
    return;
  if (cb == stripButton)
    diesetButton->setChecked(true);
  else if (cb == diesetButton)
    stripButton->setChecked(true);
  else
  {
    assert(0); //no button checked.
    throw std::runtime_error("no button in dlg::Quote::advanceSlot");
  }
}

void Quote::browseForTemplateSlot()
{
  namespace bfs = boost::filesystem;
  bfs::path t = tFileEdit->text().toStdString();
  if (!bfs::exists(t)) //todo use a parameter from preferences.
    t = static_cast<app::Application*>(qApp)->getApplicationDirectory().canonicalPath().toStdString();
  
  QString fileName = QFileDialog::getOpenFileName
  (
    this,
    tr("Browse For Template"),
    QString::fromStdString(t.string()),
    tr("SpreadSheet (*.ods)")
  );
  
  if (!fileName.isEmpty())
    tFileEdit->setText(fileName);
}

void Quote::browseForOutputSlot()
{
  namespace bfs = boost::filesystem;
  bfs::path t = oFileEdit->text().toStdString();
  if (!bfs::exists(t.parent_path()))
  {
    t = static_cast<app::Application*>(qApp)->getProject()->getSaveDirectory();
    t /= quote->getName().toStdString() + ".ods";
  }
  
  QString fileName = QFileDialog::getSaveFileName
  (
    this,
    tr("Browse For Output"),
    QString::fromStdString(t.string()),
    tr("SpreadSheet (*.ods)")
  );
  
  if (!fileName.isEmpty())
    oFileEdit->setText(fileName);
}
    

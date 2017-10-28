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
#include <iostream>
#include <stdexcept>

#include <boost/filesystem.hpp>

#include <QDoubleValidator>
#include <QMessageBox>
#include <QFileDialog>
#include <QTimer>
#include <QHBoxLayout>

#include <application/application.h>
#include <application/splitterdecorated.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <dialogs/widgetgeometry.h>
#include <dialogs/preferences.h>
#include <ui_preferences.h> //in build directory

using namespace dlg;

Preferences::Preferences(prf::Manager *managerIn, QWidget *parent) : QDialog(parent), ui(new Ui::dialog), manager(managerIn)
{
  this->setWindowFlags(this->windowFlags() | Qt::WindowContextHelpButtonHint);
  ui->setupUi(this);
  
  setupFeatureSplitter();
  
  initialize();
  
  connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
  connect(ui->basePathButton, SIGNAL(clicked()), this, SLOT(basePathBrowseSlot()));
  connect(ui->quoteTSheetButton, SIGNAL(clicked()), this, SLOT(quoteTemplateBrowseSlot()));
  
  dlg::WidgetGeometry *filter = new dlg::WidgetGeometry(this, "prf::PreferencesDialog");
  this->installEventFilter(filter);
  
  ui->tabWidget->setCurrentIndex(0);
  ui->featureListWidget->setCurrentRow(0);
}

Preferences::~Preferences()
{
  delete ui;
}

void Preferences::setupFeatureSplitter()
{
  fsSplitter = new SplitterDecorated(ui->featuresTab);
  QHBoxLayout *mainLayout = new QHBoxLayout();
  mainLayout->addWidget(fsSplitter);
  ui->featuresTab->setLayout(mainLayout);
  
  QWidget *w1 = new QWidget(fsSplitter);
  w1->setLayout(ui->featuresLayout01);
  fsSplitter->addWidget(w1);
  
  QWidget *w2 = new QWidget(fsSplitter);
  w2->setLayout(ui->featuresLayout02);
  fsSplitter->addWidget(w2);
  
  fsSplitter->restoreSettings("prf:dlg:featuresSplitter");
}

void Preferences::initialize()
{
  QDoubleValidator *positiveDouble = new QDoubleValidator(this);
  positiveDouble->setNotation(QDoubleValidator::StandardNotation);
  positiveDouble->setBottom(0.0001); //this doesn't work. validate on accept!
  positiveDouble->setDecimals(4);
  
  ui->linearDeflectionEdit->setValidator(positiveDouble);
  ui->angularDeflectionEdit->setValidator(positiveDouble);
  ui->linearDeflectionEdit->setText(QString().setNum(manager->rootPtr->visual().mesh().linearDeflection()));
  ui->angularDeflectionEdit->setText(QString().setNum(manager->rootPtr->visual().mesh().angularDeflection()));
  if (manager->rootPtr->visual().display().showHiddenLines())
    ui->hiddenLineCombo->setCurrentIndex(0);
  else
    ui->hiddenLineCombo->setCurrentIndex(1);
  
  ui->linearIncrementEdit->setText(QString().setNum(manager->rootPtr->dragger().linearIncrement()));
  ui->angularIncrementEdit->setText(QString().setNum(manager->rootPtr->dragger().angularIncrement()));
  if (manager->rootPtr->dragger().triggerUpdateOnFinish())
    ui->updateOnFinishCombo->setCurrentIndex(0);
  else
    ui->updateOnFinishCombo->setCurrentIndex(1);
  
  ui->characterSizeEdit->setText(QString().setNum(manager->rootPtr->interactiveParameter().characterSize()));
  ui->arrowWidthEdit->setText(QString().setNum(manager->rootPtr->interactiveParameter().arrowWidth()));
  ui->arrowHeightEdit->setText(QString().setNum(manager->rootPtr->interactiveParameter().arrowHeight()));
  
  ui->basePathEdit->setText(QString::fromStdString(manager->rootPtr->project().basePath()));
  ui->gitNameEdit->setText(QString::fromStdString(manager->rootPtr->project().gitName()));
  ui->gitEmailEdit->setText(QString::fromStdString(manager->rootPtr->project().gitEmail()));
  
  ui->gestureTimeEdit->setValidator(positiveDouble);
  ui->gestureTimeEdit->setText(QString().setNum(manager->rootPtr->gesture().animationSeconds()));
  ui->gestureIconRadiusEdit->setText(QString().setNum(manager->rootPtr->gesture().iconRadius()));
  ui->gestureIncludeAngleEdit->setText(QString().setNum(manager->rootPtr->gesture().includeAngle()));
  ui->gestureSpreadFactorEdit->setText(QString().setNum(manager->rootPtr->gesture().spreadFactor()));
  ui->gestureSprayFactorEdit->setText(QString().setNum(manager->rootPtr->gesture().sprayFactor()));
  
  ui->blendRadiusEdit->setText(QString().setNum(manager->rootPtr->features().blend().get().radius()));
  ui->boxLengthEdit->setText(QString().setNum(manager->rootPtr->features().box().get().length()));
  ui->boxWidthEdit->setText(QString().setNum(manager->rootPtr->features().box().get().width()));
  ui->boxHeightEdit->setText(QString().setNum(manager->rootPtr->features().box().get().height()));
  ui->chamferDistanceEdit->setText(QString().setNum(manager->rootPtr->features().chamfer().get().distance()));
  ui->coneRadius1Edit->setText(QString().setNum(manager->rootPtr->features().cone().get().radius1()));
  ui->coneRadius2Edit->setText(QString().setNum(manager->rootPtr->features().cone().get().radius2()));
  ui->coneHeightEdit->setText(QString().setNum(manager->rootPtr->features().cone().get().height()));
  ui->cylinderRadiusEdit->setText(QString().setNum(manager->rootPtr->features().cylinder().get().radius()));
  ui->cylinderHeightEdit->setText(QString().setNum(manager->rootPtr->features().cylinder().get().height()));
  ui->datumPlaneOffsetEdit->setText(QString().setNum(manager->rootPtr->features().datumPlane().get().offset()));
  ui->draftAngleEdit->setText(QString().setNum(manager->rootPtr->features().draft().get().angle()));
  ui->hollowOffsetEdit->setText(QString().setNum(manager->rootPtr->features().hollow().get().offset()));
  ui->oblongLengthEdit->setText(QString().setNum(manager->rootPtr->features().oblong().get().length()));
  ui->oblongWidthEdit->setText(QString().setNum(manager->rootPtr->features().oblong().get().width()));
  ui->oblongHeightEdit->setText(QString().setNum(manager->rootPtr->features().oblong().get().height()));
  ui->sphereRadiusEdit->setText(QString().setNum(manager->rootPtr->features().sphere().get().radius()));
  ui->diesetLengthPaddingEdit->setText(QString().setNum(manager->rootPtr->features().dieset().get().lengthPadding()));
  ui->diesetWidthPaddingEdit->setText(QString().setNum(manager->rootPtr->features().dieset().get().widthPadding()));
  ui->nestGapEdit->setText(QString().setNum(manager->rootPtr->features().nest().get().gap()));
  ui->quoteTSheetEdit->setText(QString::fromStdString(manager->rootPtr->features().quote().get().templateSheet()));
  ui->squashGEdit->setText(QString().setNum(manager->rootPtr->features().squash().get().granularity()));
  ui->stripGapEdit->setText(QString().setNum(manager->rootPtr->features().strip().get().gap()));
}

void Preferences::accept()
{
  try
  {
    updateVisual();
    updateDragger();
    updateProject();
    updateGesture();
    updateFeature();
    
    manager->saveConfig();
    QDialog::accept();
  }
  catch(std::exception &error)
  {
    QMessageBox::warning(this, tr("Error"), QString::fromStdString(error.what()));
  }
}

void Preferences::updateVisual()
{
  bool dummy;
  double tempLinearDeflection = ui->linearDeflectionEdit->text().toDouble(&dummy);
  double tempAngularDeflection = ui->angularDeflectionEdit->text().toDouble(&dummy);
  
  if (tempLinearDeflection < 0.0001)
  {
    //TODO message to log about value.
    tempLinearDeflection = manager->rootPtr->visual().mesh().linearDeflection();
  }
  if (tempAngularDeflection < 0.0001)
  {
    //TODO message to log about value.
    tempAngularDeflection = manager->rootPtr->visual().mesh().angularDeflection();
  }
  
  if (tempLinearDeflection != manager->rootPtr->visual().mesh().linearDeflection())
  {
    manager->rootPtr->visual().mesh().linearDeflection() = tempLinearDeflection;
    visualDirty = true;
  }
  if (tempAngularDeflection != manager->rootPtr->visual().mesh().angularDeflection())
  {
    manager->rootPtr->visual().mesh().angularDeflection() = tempAngularDeflection;
    visualDirty = true;
  }
  
  bool hiddenLines = ui->hiddenLineCombo->currentIndex() == 0;
  if (hiddenLines != manager->rootPtr->visual().display().showHiddenLines())
  {
    hiddenLinesDirty = true;
    manager->rootPtr->visual().display().showHiddenLines() = hiddenLines;
    //some kind of notification.
  }
}

void Preferences::updateDragger()
{
  bool dummy;
  double tempLinearIncrement = ui->linearIncrementEdit->text().toDouble(&dummy);
  double tempAngularIncrement = ui->angularIncrementEdit->text().toDouble(&dummy);
  //trigger on update.
  double tempCharacterSize = ui->characterSizeEdit->text().toDouble(&dummy);
  double tempArrowWidth = ui->arrowWidthEdit->text().toDouble(&dummy);
  double tempArrowHeight = ui->arrowHeightEdit->text().toDouble(&dummy);
  
  if
  (
    (tempLinearIncrement > 0.0) &&
    (tempLinearIncrement != manager->rootPtr->dragger().linearIncrement())
  )
    manager->rootPtr->dragger().linearIncrement() = tempLinearIncrement;
    
  if
  (
    (tempAngularIncrement > 0.0) &&
    (tempAngularIncrement != manager->rootPtr->dragger().angularIncrement())
  )
    manager->rootPtr->dragger().angularIncrement() = tempAngularIncrement;
    
  if (ui->updateOnFinishCombo->currentIndex() == 0)
    manager->rootPtr->dragger().triggerUpdateOnFinish() = true;
  else
    manager->rootPtr->dragger().triggerUpdateOnFinish() = false;
    
  if
  (
    (tempCharacterSize > 0.0) &&
    (tempCharacterSize != manager->rootPtr->interactiveParameter().characterSize())
  )
    manager->rootPtr->interactiveParameter().characterSize() = tempCharacterSize;
    
  if
  (
    (tempArrowWidth > 0.0) &&
    (tempArrowWidth != manager->rootPtr->interactiveParameter().arrowWidth())
  )
    manager->rootPtr->interactiveParameter().arrowWidth() = tempArrowWidth;
    
  if
  (
    (tempArrowHeight > 0.0) &&
    (tempArrowHeight != manager->rootPtr->interactiveParameter().arrowHeight())
  )
    manager->rootPtr->interactiveParameter().arrowHeight() = tempArrowHeight;
}

void Preferences::updateProject()
{
  QString pathString = ui->basePathEdit->text();
  QDir baseDir(pathString);
  if (!baseDir.mkpath(pathString))
  {
    QString defaultPath = QDir::homePath();
    defaultPath += QString(QDir::separator()) + "CadseerProjects";
    QDir defaultDir(defaultPath);
    assert(defaultDir.mkpath(defaultPath));
    ui->basePathEdit->setText(defaultDir.absolutePath());
    
    QString message(tr("Couldn't create base path. Default base path has been set"));
    throw std::runtime_error(message.toStdString());
  }
  
  manager->rootPtr->project().basePath() = ui->basePathEdit->text().toStdString();
  manager->rootPtr->project().gitName() = ui->gitNameEdit->text().toStdString();
  manager->rootPtr->project().gitEmail() = ui->gitEmailEdit->text().toStdString();
}

void Preferences::updateGesture()
{
  double temp = ui->gestureTimeEdit->text().toDouble();
  if (temp < 0.01)
    temp = 0.01;
  manager->rootPtr->gesture().animationSeconds() = temp;
  
  int iconRadius = ui->gestureIconRadiusEdit->text().toInt();
  iconRadius = std::max(iconRadius, 8);
  iconRadius = std::min(iconRadius, 128);
  manager->rootPtr->gesture().iconRadius() = iconRadius;
  
  int includeAngle = ui->gestureIncludeAngleEdit->text().toInt();
  includeAngle = std::max(includeAngle, 15);
  includeAngle = std::min(includeAngle, 360);
  manager->rootPtr->gesture().includeAngle() = includeAngle;
  
  double spreadFactor = ui->gestureSpreadFactorEdit->text().toDouble();
  spreadFactor = std::max(spreadFactor, 0.01);
  spreadFactor = std::min(spreadFactor, 10.0);
  manager->rootPtr->gesture().spreadFactor() = spreadFactor;
  
  double sprayFactor = ui->gestureSprayFactorEdit->text().toDouble();
  sprayFactor = std::max(sprayFactor, 0.01);
  sprayFactor = std::min(sprayFactor, 20.0);
  manager->rootPtr->gesture().sprayFactor() = sprayFactor;
}

void Preferences::updateFeature()
{
  double temp;
  
  temp = ui->blendRadiusEdit->text().toDouble();
  if (temp <= 0.0)
    temp = prf::Blend::radius_default_value();
  manager->rootPtr->features().blend().get().radius() = temp;
  
  temp = ui->boxLengthEdit->text().toDouble();
  if (temp <= 0.0)
    temp = prf::Box::length_default_value();
  manager->rootPtr->features().box().get().length() = temp;
  
  temp = ui->boxWidthEdit->text().toDouble();
  if (temp <= 0.0)
    temp = prf::Box::width_default_value();
  manager->rootPtr->features().box().get().width() = temp;
  
  temp = ui->boxHeightEdit->text().toDouble();
  if (temp <= 0.0)
    temp = prf::Box::height_default_value();
  manager->rootPtr->features().box().get().height() = temp;
  
  temp = ui->chamferDistanceEdit->text().toDouble();
  if (temp <= 0.0)
    temp = prf::Chamfer::distance_default_value();
  manager->rootPtr->features().chamfer().get().distance() = temp;
  
  temp = ui->coneRadius1Edit->text().toDouble();
  if (temp <= 0.0)
    temp = prf::Cone::radius1_default_value();
  manager->rootPtr->features().cone().get().radius1() = temp;
  
  temp = ui->coneRadius2Edit->text().toDouble();
  if (temp <= 0.0)
    temp = prf::Cone::radius2_default_value();
  manager->rootPtr->features().cone().get().radius2() = temp;
  
  temp = ui->coneHeightEdit->text().toDouble();
  if (temp <= 0.0)
    temp = prf::Cone::height_default_value();
  manager->rootPtr->features().cone().get().height() = temp;
  
  temp = ui->cylinderRadiusEdit->text().toDouble();
  if (temp <= 0.0)
    temp = prf::Cylinder::radius_default_value();
  manager->rootPtr->features().cylinder().get().radius() = temp;
  
  temp = ui->cylinderHeightEdit->text().toDouble();
  if (temp <= 0.0)
    temp = prf::Cylinder::height_default_value();
  manager->rootPtr->features().cylinder().get().height() = temp;
  
  temp = ui->datumPlaneOffsetEdit->text().toDouble();
  if (temp <= 0.0)
    temp = prf::DatumPlane::offset_default_value();
  manager->rootPtr->features().datumPlane().get().offset() = temp;
  
  temp = ui->draftAngleEdit->text().toDouble();
  if (temp <= 0.0)
    temp = prf::Draft::angle_default_value();
  manager->rootPtr->features().draft().get().angle() = temp;
  
  temp = ui->hollowOffsetEdit->text().toDouble();
  if (temp <= 0.0)
    temp = prf::Hollow::offset_default_value();
  manager->rootPtr->features().hollow().get().offset() = temp;
  
  temp = ui->sphereRadiusEdit->text().toDouble();
  if (temp <= 0.0)
    temp = prf::Sphere::radius_default_value();
  manager->rootPtr->features().sphere().get().radius() = temp;
  
  temp = ui->oblongLengthEdit->text().toDouble();
  if (temp <= 0.0)
    temp = prf::Oblong::length_default_value();
  manager->rootPtr->features().oblong().get().length() = temp;
  
  temp = ui->oblongWidthEdit->text().toDouble();
  if (temp <= 0.0)
    temp = prf::Oblong::width_default_value();
  manager->rootPtr->features().oblong().get().width() = temp;
  
  temp = ui->oblongHeightEdit->text().toDouble();
  if (temp <= 0.0)
    temp = prf::Oblong::height_default_value();
  manager->rootPtr->features().oblong().get().height() = temp;
  
  temp = ui->diesetLengthPaddingEdit->text().toDouble();
  if (temp < 0.0)
    temp = prf::Dieset::lengthPadding_default_value();
  manager->rootPtr->features().dieset().get().lengthPadding() = temp;
  
  temp = ui->diesetWidthPaddingEdit->text().toDouble();
  if (temp < 0.0)
    temp = prf::Dieset::widthPadding_default_value();
  manager->rootPtr->features().dieset().get().widthPadding() = temp;
  
  temp = ui->nestGapEdit->text().toDouble();
  if (temp < 0.0)
    temp = prf::Nest::gap_default_value();
  manager->rootPtr->features().nest().get().gap() = temp;
  
  manager->rootPtr->features().quote().get().templateSheet() = ui->quoteTSheetEdit->text().toStdString();
  
  temp = ui->squashGEdit->text().toDouble();
  if (temp < 0.0)
    temp = prf::Squash::granularity_default_value();
  manager->rootPtr->features().squash().get().granularity() = temp;
  
  temp = ui->stripGapEdit->text().toDouble();
  if (temp < 0.0)
    temp = prf::Strip::gap_default_value();
  manager->rootPtr->features().strip().get().gap() = temp;
}

void Preferences::basePathBrowseSlot()
{
  QString browseStart = ui->basePathEdit->text();
  QDir browseStartDir(browseStart);
  if (!browseStartDir.exists() || browseStart.isEmpty())
    browseStart = QString::fromStdString(manager->rootPtr->project().lastDirectory().get());
  QString freshDirectory = QFileDialog::getExistingDirectory
  (
    this,
    tr("Browse to projects base directory"),
    browseStart
  );
  if (freshDirectory.isEmpty())
    return;
  
  boost::filesystem::path p = freshDirectory.toStdString();
  manager->rootPtr->project().lastDirectory() = p.string();
  
  ui->basePathEdit->setText(freshDirectory);
}

void Preferences::quoteTemplateBrowseSlot()
{
  namespace bfs = boost::filesystem;
  bfs::path t = ui->quoteTSheetEdit->text().toStdString();
  if (!bfs::exists(t)) //todo use a parameter from preferences.
    t = manager->rootPtr->project().lastDirectory().get();
  
  QString fileName = QFileDialog::getOpenFileName
  (
    this,
    tr("Browse For Template"),
    QString::fromStdString(t.string()),
    tr("SpreadSheet (*.ods)")
  );
  
  if (fileName.isEmpty())
    return;
  
  boost::filesystem::path p = fileName.toStdString();
  manager->rootPtr->project().lastDirectory() = p.remove_filename().string();
  
  ui->quoteTSheetEdit->setText(fileName);
}

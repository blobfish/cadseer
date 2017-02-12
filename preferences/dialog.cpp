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

#include <QDoubleValidator>
#include <QMessageBox>
#include <QFileDialog>

#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <preferences/dialog.h>
#include <ui_dialog.h> //in build directory

using namespace prf;

Dialog::Dialog(Manager *managerIn, QWidget *parent) : QDialog(parent), ui(new Ui::dialog), manager(managerIn)
{
  this->setWindowFlags(this->windowFlags() | Qt::WindowContextHelpButtonHint);
  ui->setupUi(this);
  
  initialize();
  
  connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
  connect(ui->basePathButton, SIGNAL(clicked()), this, SLOT(basePathBrowseSlot()));
}

Dialog::~Dialog()
{
  delete ui;
}

void Dialog::initialize()
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
}

void Dialog::accept()
{
  try
  {
    updateVisual();
    updateDragger();
    updateProject();
    updateGesture();
    
    manager->saveConfig();
    QDialog::accept();
  }
  catch(std::exception &error)
  {
    QMessageBox::warning(this, tr("Error"), QString::fromStdString(error.what()));
  }
}

void Dialog::updateVisual()
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

void Dialog::updateDragger()
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

void Dialog::updateProject()
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

void Dialog::updateGesture()
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

void Dialog::basePathBrowseSlot()
{
  QString browseStart = ui->basePathEdit->text();
  QDir browseStartDir(browseStart);
  if (!browseStartDir.exists() || browseStart.isEmpty())
    browseStart = QDir::homePath();
  QString freshDirectory = QFileDialog::getExistingDirectory(this, tr("Browse to projects base directory"), browseStart);
  if (!freshDirectory.isEmpty())
    ui->basePathEdit->setText(freshDirectory);
}

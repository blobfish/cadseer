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

#include <QDoubleValidator>

#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <preferences/dialog.h>
#include <ui_dialog.h> //in build directory

using namespace prf;

Dialog::Dialog(Manager *managerIn, QWidget *parent) : QDialog(parent), ui(new Ui::dialog), manager(managerIn)
{
  ui->setupUi(this);
  initialize();
  
  connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
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
  
  ui->linearIncrementEdit->setText(QString().setNum(manager->rootPtr->dragger().linearIncrement()));
  ui->angularIncrementEdit->setText(QString().setNum(manager->rootPtr->dragger().angularIncrement()));
  if (manager->rootPtr->dragger().triggerUpdateOnFinish())
    ui->updateOnFinishCombo->setCurrentIndex(0);
  else
    ui->updateOnFinishCombo->setCurrentIndex(1);
  
  ui->characterSizeEdit->setText(QString().setNum(manager->rootPtr->interactiveParameter().characterSize()));
  ui->arrowWidthEdit->setText(QString().setNum(manager->rootPtr->interactiveParameter().arrowWidth()));
  ui->arrowHeightEdit->setText(QString().setNum(manager->rootPtr->interactiveParameter().arrowHeight()));
}

void Dialog::accept()
{
  updateDeflections();
  updateDragger();
  
  manager->saveConfig();
  QDialog::accept();
}

void Dialog::updateDeflections()
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

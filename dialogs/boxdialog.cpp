/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2015  tanderson <email>
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

#include <limits>

#include <QVBoxLayout>
#include <QGridLayout>
#include <QDoubleValidator>
#include <QLabel>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QLocale>

#include "boxdialog.h"

BoxDialog::BoxDialog(QWidget* parent): QDialog(parent)
{
  buildGui();
}

void BoxDialog::buildGui()
{
  QVBoxLayout *mainLayout = new QVBoxLayout();
  QGridLayout *gridLayout = new QGridLayout();
  gridLayout->setColumnStretch(1, 1);
  
  QDoubleValidator *validator = new QDoubleValidator(this);
  validator->setBottom(0.0000001);
  validator->setTop(std::numeric_limits<double>::max());
  validator->setDecimals(std::numeric_limits<double>::max_digits10);
  
  QLabel *lengthLabel = new QLabel(tr("Length: "), this);
  gridLayout->addWidget(lengthLabel, 0, 0, Qt::AlignRight);
  lengthEdit = new QLineEdit(this);
  lengthEdit->setValidator(validator);
  gridLayout->addWidget(lengthEdit, 0, 1);
  
  QLabel *widthLabel = new QLabel(tr("Width: "), this);
  gridLayout->addWidget(widthLabel, 1, 0, Qt::AlignRight);
  widthEdit = new QLineEdit(this);
  widthEdit->setValidator(validator);
  gridLayout->addWidget(widthEdit, 1, 1);
  
  QLabel *heightLabel = new QLabel(tr("Height: "), this);
  gridLayout->addWidget(heightLabel, 2, 0, Qt::AlignRight);
  heightEdit = new QLineEdit(this);
  heightEdit->setValidator(validator);
  gridLayout->addWidget(heightEdit, 2, 1);
  
  mainLayout->addLayout(gridLayout);
  
  QDialogButtonBox *buttons = new QDialogButtonBox
  (
    QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel,
    Qt::Horizontal,
    this
  );
  connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
  //apply later.
  mainLayout->addWidget(buttons);
  
  this->setLayout(mainLayout);
}

void BoxDialog::setLength(const double& lengthIn)
{
  lengthEdit->setText(cleanFormat(lengthIn));
}

void BoxDialog::setWidth(const double& widthIn)
{
  widthEdit->setText(cleanFormat(widthIn));
}

void BoxDialog::setHeight(const double& heightIn)
{
  heightEdit->setText(cleanFormat(heightIn));
}

void BoxDialog::setParameters(const double& lengthIn, const double& widthIn, const double& heightIn)
{
  setLength(lengthIn);
  setWidth(widthIn);
  setHeight(heightIn);
}

//this gets rid of trailing zeros while keeping max double precision.
QString BoxDialog::cleanFormat(const double& valueIn)
{
  //note QLocale::decimalPoint return QChar but is going to change in future to QString.
  QString decimalSignature = QLocale::system().decimalPoint();
  QChar zeroDigit = QLocale::system().zeroDigit();
  QString tempString = QString::number(valueIn, 'f', std::numeric_limits<double>::max_digits10);
  if (!tempString.contains(decimalSignature))
    return tempString;
  
  //find last occurence of a non zero after the decimal.
  QString reversed;
  for (auto it = tempString.constBegin(); it != tempString.constEnd(); ++it)
    reversed.push_front(*it);
  
  QString out;
  bool foundNonZeroFlag = false;
  for (const auto &current : reversed)
  {
    if (current != zeroDigit)
      foundNonZeroFlag = true;
    if (!foundNonZeroFlag)
      continue;
    out.push_front(current);
  }
  if (out.endsWith(decimalSignature))
    out.push_back(zeroDigit);
  
  return out;
}

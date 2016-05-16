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

#include <QLabel>
#include <QKeyEvent>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>

#include <boost/uuid/uuid.hpp>

#include <application/application.h>
#include <project/project.h>
#include <application/mainwindow.h>
#include <feature/base.h>
#include <feature/parameter.h>
#include <message/message.h>
#include <message/dispatch.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include "parameterdialog.h"

using namespace dlg;

void EnterEdit::keyPressEvent(QKeyEvent *eventIn)
{
  QLineEdit::keyPressEvent(eventIn);
  if (eventIn->key() == Qt::Key_Tab)
    Q_EMIT goUpdateSignal();
}

ParameterDialog::ParameterDialog(ftr::Parameter *parameterIn, const boost::uuids::uuid &idIn):
  QDialog(static_cast<app::Application*>(qApp)->getMainWindow()),
  parameter(parameterIn)
{
  assert(parameter);
  
  feature = static_cast<app::Application*>(qApp)->getProject()->findFeature(idIn);
  assert(feature);
  buildGui();
  valueHasChanged();
  constantHasChanged();
  
  parameter->connectValue(boost::bind(&ParameterDialog::valueHasChanged, this));
  parameter->connectConstant(boost::bind(&ParameterDialog::constantHasChanged, this));
  
  msg::dispatch().connectMessageOut(boost::bind(&ParameterDialog::messageInSlot, this, _1));
  connectMessageOut(boost::bind(&msg::Dispatch::messageInSlot, &msg::dispatch(), _1));
}

void ParameterDialog::buildGui()
{
  this->setWindowTitle(feature->getName());
  
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  
  QLabel *nameLabel = new QLabel(QString::fromUtf8(parameter->getName().c_str()), this);
  editLine = new EnterEdit(this);
  QHBoxLayout *editLayout = new QHBoxLayout();
  editLayout->addWidget(nameLabel);
  editLayout->addWidget(editLine);
  mainLayout->addLayout(editLayout);
  
  mainLayout->addSpacing(10);
  mainLayout->addStretch();
  
  QHBoxLayout *buttonLayout = new QHBoxLayout();
  linkButton = new QPushButton(this);
  linkButton->setCheckable(true);
  linkButton->setFocusPolicy(Qt::ClickFocus);
  buttonLayout->addWidget(linkButton);
  buttonLayout->addStretch();
  mainLayout->addLayout(buttonLayout);
  
  connect(editLine, SIGNAL(goUpdateSignal()), this, SLOT(updateSlot()));
  connect(this, SIGNAL(accepted()), this, SLOT(updateSlot()));
  connect(linkButton, SIGNAL(toggled(bool)), this, SLOT(linkButtonToggledSlot(bool)));
}

void ParameterDialog::keyPressEvent(QKeyEvent *eventIn)
{
  if
  (
    (eventIn->key() == Qt::Key_Return) ||
    (eventIn->key() == Qt::Key_Enter)
  )
  {
    this->accept();
    return;
  }
  else if (eventIn->key() == Qt::Key_Escape)
  {
    this->reject();
    return;
  }
  
  QDialog::keyPressEvent(eventIn);
}

void ParameterDialog::valueHasChanged()
{
  lastValue = parameter->getValue();
  editLine->setText(QString::number(parameter->getValue(), 'f', 12));
  editLine->selectAll();
}

void ParameterDialog::constantHasChanged()
{
  if (parameter->isConstant())
  {
    linkButton->setChecked(false);
    linkButton->setIcon(QIcon(":resources/images/unlinkIcon.svg"));
    linkButton->setText(tr("unlinked"));
  }
  else
  {
    linkButton->setChecked(true);
    linkButton->setIcon(QIcon(":resources/images/linkIcon.svg"));
    linkButton->setText(tr("linked"));
  }
}

void ParameterDialog::linkButtonToggledSlot(bool checkedState)
{
  parameter->setConstant(!checkedState);
}

void ParameterDialog::messageInSlot(const msg::Message &messageIn)
{
  if (messageIn.mask == (msg::Response | msg::Pre | msg::RemoveFeature))
  {
    prj::Message pMessage = boost::get<prj::Message>(messageIn.payload);
    if(pMessage.feature->getId() == feature->getId())
      this->reject();
  }
}

void ParameterDialog::updateSlot()
{
  double temp = editLine->text().toDouble();
  if
  (
    (parameter->canBeNegative()) ||
    ((!parameter->canBeNegative()) && (temp > 0.0))
  )
  {
    parameter->setValue(temp);
    if (prf::manager().rootPtr->dragger().triggerUpdateOnFinish())
    {
      msg::Message uMessage;
      uMessage.mask = msg::Request | msg::Update;
      messageOutSignal(uMessage);
    }
  }
  else
  {
    editLine->setText(QString::number(lastValue, 'f', 12));
    editLine->selectAll();
  }
}

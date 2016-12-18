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

#include <tools/idtools.h>
#include <expressions/expressionmanager.h>
#include <expressions/stringtranslator.h>
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
  this->setAcceptDrops(true);
  
  feature = static_cast<app::Application*>(qApp)->getProject()->findFeature(idIn);
  assert(feature);
  buildGui();
  constantHasChanged(); //call valueHasChanged if needs to
  
  parameter->connectValue(boost::bind(&ParameterDialog::valueHasChanged, this));
  parameter->connectConstant(boost::bind(&ParameterDialog::constantHasChanged, this));
  
  msg::dispatch().connectMessageOut(boost::bind(&ParameterDialog::messageInSlot, this, boost::placeholders::_1));
  connectMessageOut(boost::bind(&msg::Dispatch::messageInSlot, &msg::dispatch(), boost::placeholders::_1));
}

void ParameterDialog::buildGui()
{
  this->setWindowTitle(feature->getName());
  
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  
  QLabel *nameLabel = new QLabel(QString::fromUtf8(parameter->getName().c_str()), this);
  editLine = new EnterEdit(this);
  editLine->setAcceptDrops(false);
  int iconHeight(editLine->height());
  trafficRed = QPixmap(":/resources/images/trafficRed.svg").scaled(iconHeight, iconHeight, Qt::KeepAspectRatio);
  trafficYellow = QPixmap(":/resources/images/trafficYellow.svg").scaled(iconHeight, iconHeight, Qt::KeepAspectRatio);
  trafficGreen = QPixmap(":/resources/images/trafficGreen.svg").scaled(iconHeight, iconHeight, Qt::KeepAspectRatio);
  trafficLabel = new QLabel(this);
  trafficLabel->setPixmap(trafficGreen);
  QHBoxLayout *editLayout = new QHBoxLayout();
  editLayout->addWidget(nameLabel);
  editLayout->addWidget(editLine);
  editLayout->addWidget(trafficLabel);
  
  mainLayout->addLayout(editLayout);
  
  mainLayout->addSpacing(10);
  mainLayout->addStretch();
  
  QHBoxLayout *buttonLayout = new QHBoxLayout();
  linkButton = new QPushButton(this);
  linkButton->setCheckable(true);
  linkButton->setFocusPolicy(Qt::ClickFocus);
  linkLabel = new QLabel(this);
  buttonLayout->addWidget(linkButton);
  buttonLayout->addStretch();
  buttonLayout->addWidget(linkLabel);
  mainLayout->addLayout(buttonLayout);
  
  connect(editLine, SIGNAL(goUpdateSignal()), this, SLOT(updateSlot()));
  connect(editLine, SIGNAL(textEdited(QString)), this, SLOT(textEditedSlot(QString)));
  connect(this, SIGNAL(accepted()), this, SLOT(updateSlot()));
  connect(linkButton, SIGNAL(clicked(bool)), this, SLOT(linkButtonClickedSlot(bool)));
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

static boost::uuids::uuid getId(const QString &stringIn)
{
  boost::uuids::uuid idOut = gu::createNilId();
  if (stringIn.startsWith("ExpressionId;"))
  {
    QStringList split = stringIn.split(";");
    if (split.size() == 2)
      idOut = gu::stringToId(split.at(1).toStdString());
  }
  return idOut;
}

void ParameterDialog::dragEnterEvent(QDragEnterEvent *event)
{
  if (event->mimeData()->hasText())
  {
    QString textIn = event->mimeData()->text();
    boost::uuids::uuid id = getId(textIn);
    if (!id.is_nil())
    {
      const expr::ExpressionManager &eManager = static_cast<app::Application *>(qApp)->getProject()->getExpressionManager();
      if (eManager.hasFormula(id))
	event->acceptProposedAction();
    }
  }
}

void ParameterDialog::dropEvent(QDropEvent *event)
{
  if (event->mimeData()->hasText())
  {
    QString textIn = event->mimeData()->text();
    boost::uuids::uuid id = getId(textIn);
    if (!id.is_nil())
    {
      expr::ExpressionManager &eManager = static_cast<app::Application *>(qApp)->getProject()->getExpressionManager();
      if (!parameter->isConstant())
      {
        //parameter is already linked.
        assert(eManager.hasFormulaLink(feature->getId(), parameter));
        eManager.removeFormulaLink(feature->getId(), parameter);
      }
      
      assert(eManager.hasFormula(id));
      eManager.addFormulaLink(feature->getId(), parameter, id);
      if (prf::manager().rootPtr->dragger().triggerUpdateOnFinish())
      {
        messageOutSignal(msg::Mask(msg::Request | msg::Update));
      }
    }
  }
}

void ParameterDialog::valueHasChanged()
{
  lastValue = parameter->getValue();
  editLine->setText(QString::number(parameter->getValue(), 'f', 12));
  if (parameter->isConstant())
  {
    editLine->selectAll();
    linkLabel->setText(QString::number(parameter->getValue(), 'f', 4));
  }
}

void ParameterDialog::constantHasChanged()
{
  if (parameter->isConstant())
  {
    linkButton->setChecked(false);
    linkButton->setIcon(QIcon(":resources/images/unlinkIcon.svg"));
    linkButton->setText(tr("unlinked"));
    linkButton->setDisabled(true);
    editLine->setEnabled(true);
    editLine->setFocus();
  }
  else
  {
    linkButton->setChecked(true);
    linkButton->setIcon(QIcon(":resources/images/linkIcon.svg"));
    linkButton->setText(tr("linked"));
    linkButton->setEnabled(true);
    
    const expr::ExpressionManager &eManager = static_cast<app::Application *>(qApp)->getProject()->getExpressionManager();
    assert(eManager.hasFormulaLink(feature->getId(), parameter));
    std::string formulaName = eManager.getFormulaName(eManager.getFormulaLink(feature->getId(), parameter));
    linkLabel->setText(QString::fromStdString(formulaName));
    
    editLine->clearFocus();
    editLine->deselect();
    editLine->setDisabled(true);
  }
  valueHasChanged();
}

void ParameterDialog::linkButtonClickedSlot(bool checkedState)
{
  if (!checkedState)
  {
    expr::ExpressionManager &eManager = static_cast<app::Application *>(qApp)->getProject()->getExpressionManager();
    assert(eManager.hasFormulaLink(feature->getId(), parameter));
    eManager.removeFormulaLink(feature->getId(), parameter);
    //manager sets the parameter to constant or not.
  }
}

void ParameterDialog::messageInSlot(const msg::Message &messageIn)
{
  if (messageIn.mask == (msg::Response | msg::Pre | msg::Remove | msg::Feature))
  {
    prj::Message pMessage = boost::get<prj::Message>(messageIn.payload);
    if(pMessage.feature->getId() == feature->getId())
      this->reject();
  }
}

void ParameterDialog::updateSlot()
{
  std::ostringstream gitStream;
  gitStream
    << QObject::tr("Feature: ").toStdString() << feature->getName().toStdString()
    << QObject::tr("    Parameter ").toStdString() << parameter->getName();
    
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
      gitStream  << QObject::tr("    changed to: ").toStdString() << parameter->getValue();
      messageOutSignal(msg::buildGitMessage(gitStream.str()));
      
      messageOutSignal(msg::Mask(msg::Request | msg::Update));
    }
  }
  else
  {
    expr::ExpressionManager localManager;
    expr::StringTranslator translator(localManager);
    std::string formula("temp = ");
    formula += editLine->text().toStdString();
    if (translator.parseString(formula) == expr::StringTranslator::ParseSucceeded)
    {
      localManager.update();
      double value = localManager.getFormulaValue(translator.getFormulaOutId());
      parameter->setValue(value);
      if (prf::manager().rootPtr->dragger().triggerUpdateOnFinish())
      {
        gitStream  << QObject::tr("    changed to: ").toStdString() << parameter->getValue();
        messageOutSignal(msg::buildGitMessage(gitStream.str()));
        
        messageOutSignal(msg::Mask(msg::Request | msg::Update));
      }
    }
    else
    {
      std::cout << "fail position: " << translator.getFailedPosition() - 7 << std::endl;
      editLine->setText(QString::number(lastValue, 'f', 12));
      editLine->selectAll();
      trafficLabel->setPixmap(trafficGreen);
      linkLabel->setText(QString::number(lastValue, 'f', 4));
    }
  }
}

void ParameterDialog::textEditedSlot(const QString &textIn)
{
  trafficLabel->setPixmap(trafficYellow);
  qApp->processEvents(); //need this or we never see yellow signal.
  
  expr::ExpressionManager localManager;
  expr::StringTranslator translator(localManager);
  std::string formula("temp = ");
  formula += textIn.toStdString();
  if (translator.parseString(formula) == expr::StringTranslator::ParseSucceeded)
  {
    localManager.update();
    trafficLabel->setPixmap(trafficGreen);
    double value = localManager.getFormulaValue(translator.getFormulaOutId());
    linkLabel->setText(QString::number(value, 'f', 4));
  }
  else
  {
    trafficLabel->setPixmap(trafficRed);
    linkLabel->setText("?");
  }
}

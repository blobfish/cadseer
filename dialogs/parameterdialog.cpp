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
#include <QTimer>

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
#include <message/observer.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <dialogs/expressionedit.h>
#include <dialogs/widgetgeometry.h>
#include <dialogs/parameterdialog.h>

using namespace dlg;

ParameterDialog::ParameterDialog(ftr::Parameter *parameterIn, const boost::uuids::uuid &idIn):
  QDialog(static_cast<app::Application*>(qApp)->getMainWindow()),
  parameter(parameterIn)
{
  assert(parameter);
  
  feature = static_cast<app::Application*>(qApp)->getProject()->findFeature(idIn);
  assert(feature);
  buildGui();
  constantHasChanged();
  
  valueConnection = parameter->connectValue
    (boost::bind(&ParameterDialog::valueHasChanged, this));
  constantConnection = parameter->connectConstant
    (boost::bind(&ParameterDialog::constantHasChanged, this));
  
  observer = std::move(std::unique_ptr<msg::Observer>(new msg::Observer()));
  observer->dispatcher.insert(std::make_pair(msg::Response | msg::Pre | msg::Remove | msg::Feature,
    boost::bind(&ParameterDialog::featureRemovedDispatched, this, boost::placeholders::_1)));
  
  WidgetGeometry *filter = new WidgetGeometry(this, "dlg::ParameterDialog");
  this->installEventFilter(filter);
  
  this->setAttribute(Qt::WA_DeleteOnClose);
}

ParameterDialog::~ParameterDialog()
{
  valueConnection.disconnect();
  constantConnection.disconnect();
}

void ParameterDialog::buildGui()
{
  this->setWindowTitle(feature->getName());
  
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  
  QLabel *nameLabel = new QLabel(parameter->getName(), this);
  editLine = new ExpressionEdit(this);
  if (parameter->isConstant())
    QTimer::singleShot(0, editLine->trafficLabel, SLOT(setTrafficGreenSlot()));
  else
    QTimer::singleShot(0, editLine->trafficLabel, SLOT(setLinkSlot()));
  QHBoxLayout *editLayout = new QHBoxLayout();
  editLayout->addWidget(nameLabel);
  editLayout->addWidget(editLine);
  
  mainLayout->addLayout(editLayout);
  
  ExpressionEditFilter *filter = new ExpressionEditFilter(this);
  editLine->lineEdit->installEventFilter(filter);
  
  connect(editLine->lineEdit, SIGNAL(textEdited(QString)), this, SLOT(textEditedSlot(QString)));
  connect(editLine->lineEdit, SIGNAL(returnPressed()), this, SLOT(updateSlot()));
  connect(editLine->trafficLabel, SIGNAL(requestUnlinkSignal()), SLOT(requestUnlinkSlot()));
  connect(filter, SIGNAL(requestLinkSignal(QString)), this, SLOT(requestLinkSlot(QString)));
}

void ParameterDialog::requestLinkSlot(const QString &stringIn)
{
  boost::uuids::uuid id = gu::stringToId(stringIn.toStdString());
  assert(!id.is_nil());
  
  expr::ExpressionManager &eManager = static_cast<app::Application *>(qApp)->getProject()->getExpressionManager();
  if (!parameter->isConstant())
  {
    //parameter is already linked.
    assert(eManager.hasParameterLink(parameter->getId()));
    eManager.removeParameterLink(parameter->getId());
  }
  
  assert(eManager.hasFormula(id));
  eManager.addLink(parameter, id);
  
  if (prf::manager().rootPtr->dragger().triggerUpdateOnFinish())
    observer->out(msg::Mask(msg::Request | msg::Update));
  
  this->activateWindow();
}

void ParameterDialog::requestUnlinkSlot()
{
  expr::ExpressionManager &eManager = static_cast<app::Application *>(qApp)->getProject()->getExpressionManager();
  assert(eManager.hasParameterLink(parameter->getId()));
  eManager.removeParameterLink(parameter->getId());
  //manager sets the parameter to constant or not.
}

void ParameterDialog::valueHasChanged()
{
  lastValue = parameter->getValue();
  if (parameter->isConstant())
  {
    editLine->lineEdit->setText(QString::number(parameter->getValue(), 'f', 12));
    editLine->lineEdit->selectAll();
  }
  //if it is linked we shouldn't need to change.
}

void ParameterDialog::constantHasChanged()
{
  if (parameter->isConstant())
  {
    editLine->trafficLabel->setTrafficGreenSlot();
    editLine->lineEdit->setReadOnly(false);
    editLine->setFocus();
  }
  else
  {
    const expr::ExpressionManager &eManager = static_cast<app::Application *>(qApp)->getProject()->getExpressionManager();
    assert(eManager.hasParameterLink(parameter->getId()));
    std::string formulaName = eManager.getFormulaName(eManager.getFormulaLink(parameter->getId()));
    
    editLine->trafficLabel->setLinkSlot();
    editLine->lineEdit->setText(QString::fromStdString(formulaName));
    editLine->clearFocus();
    editLine->lineEdit->deselect();
    editLine->lineEdit->setReadOnly(true);
  }
  valueHasChanged();
}

void ParameterDialog::featureRemovedDispatched(const msg::Message &messageIn)
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
  //if we are linked, we shouldn't need to do anything.
  if (!parameter->isConstant())
    return;

  auto fail = [&]()
  {
    editLine->lineEdit->setText(QString::number(lastValue, 'f', 12));
    editLine->lineEdit->selectAll();
    editLine->trafficLabel->setTrafficGreenSlot();
  };

  std::ostringstream gitStream;
  gitStream
    << QObject::tr("Feature: ").toStdString() << feature->getName().toStdString()
    << QObject::tr("    Parameter ").toStdString() << parameter->getName().toStdString();

  //just run it through a string translator and expression manager.
  expr::ExpressionManager localManager;
  expr::StringTranslator translator(localManager);
  std::string formula("temp = ");
  formula += editLine->lineEdit->text().toStdString();
  if (translator.parseString(formula) == expr::StringTranslator::ParseSucceeded)
  {
    localManager.update();
    double value = localManager.getFormulaValue(translator.getFormulaOutId());
    if (parameter->isValidValue(value))
    {
      parameter->setValue(value);
      if (prf::manager().rootPtr->dragger().triggerUpdateOnFinish())
      {
        gitStream  << QObject::tr("    changed to: ").toStdString() << parameter->getValue();
        observer->out(msg::buildGitMessage(gitStream.str()));
        observer->out(msg::Mask(msg::Request | msg::Update));
      }
    }
    else
    {
      observer->out(msg::buildStatusMessage(QObject::tr("Value out of range").toStdString()));
      fail();
    }
  }
  else
  {
    observer->out(msg::buildStatusMessage(QObject::tr("Parsing failed").toStdString()));
    fail();
  }
}

void ParameterDialog::textEditedSlot(const QString &textIn)
{
  editLine->trafficLabel->setTrafficYellowSlot();
  qApp->processEvents(); //need this or we never see yellow signal.
  
  expr::ExpressionManager localManager;
  expr::StringTranslator translator(localManager);
  std::string formula("temp = ");
  formula += textIn.toStdString();
  if (translator.parseString(formula) == expr::StringTranslator::ParseSucceeded)
  {
    localManager.update();
    editLine->trafficLabel->setTrafficGreenSlot();
    double value = localManager.getFormulaValue(translator.getFormulaOutId());
    editLine->goToolTipSlot(QString::number(value));
  }
  else
  {
    editLine->trafficLabel->setTrafficRedSlot();
    int position = translator.getFailedPosition() - 8; // 7 chars for 'temp = ' + 1
    editLine->goToolTipSlot(textIn.left(position) + "?");
  }
}

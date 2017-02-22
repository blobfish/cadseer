/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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

#include <QApplication>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QFontMetrics>
#include <QTimer>
#include <QKeyEvent>

#include <expressions/manager.h>
#include <expressions/stringtranslator.h>
#include <dialogs/expressionedit.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <application/incrementwidget.h>

using namespace app;

IncrementWidgetAction::IncrementWidgetAction(QObject* parent, const QString& title1In, const QString& title2In):
  QWidgetAction(parent), title1(title1In), title2(title2In)
{

}

QWidget* IncrementWidgetAction::createWidget(QWidget* parent)
{
  QWidget *out = new QWidget(parent);
  
  QHBoxLayout *hLayout = new QHBoxLayout();
  
  QVBoxLayout *vLayout1 = new QVBoxLayout();
  vLayout1->setSpacing(0);
  QLabel *label1 = new QLabel(title1, out);
  vLayout1->addWidget(label1);
  lineEdit1 = new dlg::ExpressionEdit(out);
  int width1 = QFontMetrics(QApplication::font()).boundingRect(title1).width();
  lineEdit1->setMaximumWidth(width1);
  vLayout1->addWidget(lineEdit1);
  vLayout1->setContentsMargins(0, 0, 0, 0);
  hLayout->addLayout(vLayout1);
  lineEdit1->lineEdit->setText(QString::number(prf::manager().rootPtr->dragger().linearIncrement(), 'f', 12));
  lineEdit1->lineEdit->setCursorPosition(0);
  connect(lineEdit1->lineEdit, SIGNAL(textEdited(QString)), this, SLOT(textEditedSlot1(QString)));
  connect(lineEdit1->lineEdit, SIGNAL(editingFinished()), this, SLOT(editingFinishedSlot1()));
  connect(lineEdit1->lineEdit, SIGNAL(returnPressed()), this, SLOT(returnPressedSlot1()));
  
  QVBoxLayout *vLayout2 = new QVBoxLayout();
  vLayout2->setSpacing(0);
  QLabel *label2 = new QLabel(title2, out);
  vLayout2->addWidget(label2);
  lineEdit2 = new dlg::ExpressionEdit(out);
  int width2 = QFontMetrics(QApplication::font()).boundingRect(title2).width();
  lineEdit2->setMaximumWidth(width2);
  vLayout2->addWidget(lineEdit2);
  vLayout2->setContentsMargins(0, 0, 0, 0);
  hLayout->addLayout(vLayout2);
  lineEdit2->lineEdit->setText(QString::number(prf::manager().rootPtr->dragger().angularIncrement(), 'f', 12));
  lineEdit2->lineEdit->setCursorPosition(0);
  connect(lineEdit2->lineEdit, SIGNAL(textEdited(QString)), this, SLOT(textEditedSlot2(QString)));
  connect(lineEdit2->lineEdit, SIGNAL(editingFinished()), this, SLOT(editingFinishedSlot2()));
  connect(lineEdit2->lineEdit, SIGNAL(returnPressed()), this, SLOT(returnPressedSlot2()));
  
  hLayout->addStretch();
  //this gets rid of the extra space from this widget and it's layout.
  //this needs to be done after widgets added.
  hLayout->setContentsMargins(0, 0, 0, 0);
  
  out->setLayout(hLayout);
  
  HighlightOnFocusFilter *filter1 = new HighlightOnFocusFilter(lineEdit1->lineEdit);
  lineEdit1->lineEdit->installEventFilter(filter1);
  HighlightOnFocusFilter *filter2 = new HighlightOnFocusFilter(lineEdit2->lineEdit);
  lineEdit2->lineEdit->installEventFilter(filter2);
  
  return out;
}

void IncrementWidgetAction::textEditedSlot1(const QString &textIn)
{
  textEditedCommon(textIn, lineEdit1);
}

void IncrementWidgetAction::textEditedSlot2(const QString &textIn)
{
  textEditedCommon(textIn, lineEdit2);
}

void IncrementWidgetAction::textEditedCommon(const QString &textIn, dlg::ExpressionEdit *editor)
{
  editor->trafficLabel->setTrafficYellowSlot();
  qApp->processEvents(); //need this or we never see yellow signal.
  
  expr::ExpressionManager localManager;
  expr::StringTranslator translator(localManager);
  std::string formula("temp = ");
  formula += textIn.toStdString();
  if (translator.parseString(formula) == expr::StringTranslator::ParseSucceeded)
  {
    localManager.update();
    editor->trafficLabel->setTrafficGreenSlot();
    double value = localManager.getFormulaValue(translator.getFormulaOutId());
    editor->goToolTipSlot(QString::number(value));
  }
  else
  {
    editor->trafficLabel->setTrafficRedSlot();
    int position = translator.getFailedPosition() - 8; // 7 chars for 'temp = ' + 1
    editor->goToolTipSlot(textIn.left(position) + "?");
  }
}

void IncrementWidgetAction::editingFinishedSlot1()
{
  lineEdit1->trafficLabel->setPixmap(QPixmap());
  
  double parsedValue = editingFinishedCommon(lineEdit1);
  if (parsedValue > 0.0)
  {
    prf::manager().rootPtr->dragger().linearIncrement() = parsedValue;
    prf::manager().saveConfig();
  }
  else
  {
    parsedValue = prf::manager().rootPtr->dragger().linearIncrement();
  }
  
  lineEdit1->lineEdit->setText(QString::number(parsedValue, 'f', 12));
}

void IncrementWidgetAction::editingFinishedSlot2()
{
  lineEdit2->trafficLabel->setPixmap(QPixmap());
  
  double parsedValue = editingFinishedCommon(lineEdit2);
  if (parsedValue > 0.0)
  {
    prf::manager().rootPtr->dragger().angularIncrement() = parsedValue;
    prf::manager().saveConfig();
  }
  else
  {
    parsedValue = prf::manager().rootPtr->dragger().angularIncrement();
  }
  
  lineEdit2->lineEdit->setText(QString::number(parsedValue, 'f', 12));
}

double IncrementWidgetAction::editingFinishedCommon(dlg::ExpressionEdit *editor)
{
  expr::ExpressionManager localManager;
  expr::StringTranslator translator(localManager);
  std::string formula("temp = ");
  formula += editor->lineEdit->text().toStdString();
  if (translator.parseString(formula) == expr::StringTranslator::ParseSucceeded)
  {
    localManager.update();
    double tempValue = localManager.getFormulaValue(translator.getFormulaOutId());
    return tempValue;
  }
  
  return -1.0; //signals failure to parse.
}

void IncrementWidgetAction::returnPressedSlot1()
{
  lineEdit1->lineEdit->setCursorPosition(0);
  QTimer::singleShot(0, lineEdit1->lineEdit, SLOT(selectAll()));
}

void IncrementWidgetAction::returnPressedSlot2()
{
  lineEdit2->lineEdit->setCursorPosition(0);
  QTimer::singleShot(0, lineEdit2->lineEdit, SLOT(selectAll()));
}

bool HighlightOnFocusFilter::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::FocusIn)
  {
    QLineEdit *lineEdit = qobject_cast<QLineEdit*>(obj);
    assert(lineEdit);
    QTimer::singleShot(0, lineEdit, SLOT(selectAll()));
  }
  else if (event->type() == QEvent::KeyPress)
  {
    QKeyEvent *kEvent = dynamic_cast<QKeyEvent*>(event);
    assert(kEvent);
    if (kEvent->key() == Qt::Key_Escape)
    {
      QLineEdit *lineEdit = qobject_cast<QLineEdit*>(obj);
      assert(lineEdit);
      //here we will just set the text to invalid and
      //send a return event. this should restore the value
      //to what it was prior to editing.
      lineEdit->setText("?");
      QKeyEvent *freshEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
      qApp->postEvent(lineEdit, freshEvent);
    }
  }
  
  return QObject::eventFilter(obj, event);
}

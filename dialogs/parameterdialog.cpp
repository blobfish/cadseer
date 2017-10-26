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

#include <boost/filesystem.hpp>
#include <boost/signals2/shared_connection_block.hpp>

#include <QLabel>
#include <QKeyEvent>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QTimer>
#include <QTextStream>
#include <QFileDialog>

#include <tools/idtools.h>
#include <tools/infotools.h>
#include <expressions/manager.h>
#include <expressions/stringtranslator.h>
#include <expressions/value.h>
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

//build the appropriate widget editor.
class WidgetFactoryVisitor : public boost::static_visitor<QWidget*>
{
public:
  WidgetFactoryVisitor() = delete;
  WidgetFactoryVisitor(ParameterDialog *dialogIn) : dialog(dialogIn){assert(dialog);}
  QWidget* operator()(double) const {return buildExpressionEdit();}
  QWidget* operator()(int) const {return buildExpressionEdit();}
  QWidget* operator()(bool) const {return buildBool();}
  QWidget* operator()(const std::string&) const {return buildString();}
  QWidget* operator()(const boost::filesystem::path&) const {return buildPath();}
  QWidget* operator()(const osg::Vec3d&) const {return buildVec3d();}
  QWidget* operator()(const osg::Quat&) const {return buildQuat();}
  QWidget* operator()(const osg::Matrixd&) const {return buildMatrix();}
  
private:
  ParameterDialog *dialog;
  
  QWidget* buildExpressionEdit() const //!< might used for more than just doubles.
  {
    ExpressionEdit *editLine = new ExpressionEdit(dialog);
    if (dialog->parameter->isConstant())
      QTimer::singleShot(0, editLine->trafficLabel, SLOT(setTrafficGreenSlot()));
    else
      QTimer::singleShot(0, editLine->trafficLabel, SLOT(setLinkSlot()));
    
    //this might be needed for other types with some changes
    ExpressionEditFilter *filter = new ExpressionEditFilter(dialog);
    editLine->lineEdit->installEventFilter(filter);
    QObject::connect(filter, SIGNAL(requestLinkSignal(QString)), dialog, SLOT(requestLinkSlot(QString)));
    
    QObject::connect(editLine->lineEdit, SIGNAL(textEdited(QString)), dialog, SLOT(textEditedDoubleSlot(QString)));
    QObject::connect(editLine->lineEdit, SIGNAL(returnPressed()), dialog, SLOT(updateDoubleSlot()));
    QObject::connect(editLine->trafficLabel, SIGNAL(requestUnlinkSignal()), dialog, SLOT(requestUnlinkSlot()));
    
    return editLine;
  }
  
  QWidget* buildBool() const
  {
    QComboBox *out = new QComboBox(dialog);
    out->addItem(QObject::tr("True"), QVariant(true));
    out->addItem(QObject::tr("False"), QVariant(false));
    
    if (static_cast<bool>(*(dialog->parameter)))
      out->setCurrentIndex(0);
    else
      out->setCurrentIndex(1);
    
    QObject::connect(out, SIGNAL(currentIndexChanged(int)), dialog, SLOT(boolChangedSlot(int)));
    
    return out;
  }
  
  QWidget* buildString() const
  {
    QLineEdit* out = new QLineEdit(dialog);
    out->setText(QString::fromStdString(static_cast<std::string>(*(dialog->parameter))));
    
    //connect to dialog slots
    
    return out;
  }
  
  QWidget* buildPath() const
  {
    QWidget *ww = new QWidget(dialog); //widget wrapper.
    QHBoxLayout *hl = new QHBoxLayout();
    hl->setContentsMargins(0, 0, 0, 0);
    ww->setLayout(hl);
    
    QLineEdit *le = new QLineEdit(ww);
    le->setObjectName(QString("TheLineEdit")); //we use this to search in the slot.
    le->setText(QString::fromStdString(static_cast<boost::filesystem::path>(*(dialog->parameter)).string()));
    hl->addWidget(le);
    
    QPushButton *bb = new QPushButton(QObject::tr("Browse"), ww);
    hl->addWidget(bb);
    QObject::connect(bb, SIGNAL(clicked()), dialog, SLOT(browseForPathSlot()));
    
    return ww;
  }
  
  QWidget* buildVec3d() const
  {
    osg::Vec3d theVec = static_cast<osg::Vec3d>(*(dialog->parameter));
    
    QWidget *ww = new QWidget(dialog); //widget wrapper.
    QVBoxLayout *vl = new QVBoxLayout();
    vl->setContentsMargins(0, 0, 0, 0);
    ww->setLayout(vl);
    
    QHBoxLayout *xLayout = new QHBoxLayout();
    QLabel *xLabel = new QLabel(QObject::tr("X:"), ww);
    xLayout->addWidget(xLabel);
    QLineEdit *xLineEdit = new QLineEdit(ww);
    xLineEdit->setObjectName("XLineEdit");
    xLineEdit->setText(QString::number(theVec.x(), 'f', 12));
    xLayout->addWidget(xLineEdit);
    vl->addLayout(xLayout);
    QObject::connect(xLineEdit, SIGNAL(editingFinished()), dialog, SLOT(vectorChangedSlot()));
    
    QHBoxLayout *yLayout = new QHBoxLayout();
    QLabel *yLabel = new QLabel(QObject::tr("Y:"), ww);
    yLayout->addWidget(yLabel);
    QLineEdit *yLineEdit = new QLineEdit(ww);
    yLineEdit->setObjectName("YLineEdit");
    yLineEdit->setText(QString::number(theVec.y(), 'f', 12));
    yLayout->addWidget(yLineEdit);
    vl->addLayout(yLayout);
    QObject::connect(yLineEdit, SIGNAL(editingFinished()), dialog, SLOT(vectorChangedSlot()));
    
    QHBoxLayout *zLayout = new QHBoxLayout();
    QLabel *zLabel = new QLabel(QObject::tr("Z:"), ww);
    zLayout->addWidget(zLabel);
    QLineEdit *zLineEdit = new QLineEdit(ww);
    zLineEdit->setObjectName("ZLineEdit");
    zLineEdit->setText(QString::number(theVec.z(), 'f', 12));
    zLayout->addWidget(zLineEdit);
    vl->addLayout(zLayout);
    QObject::connect(zLineEdit, SIGNAL(editingFinished()), dialog, SLOT(vectorChangedSlot()));
    
    return ww;
    
    //maybe more elaborate control with vector selection?
  }
  
  QWidget* buildQuat() const
  {
    QLineEdit* out = new QLineEdit(dialog);
    
    QString buffer;
    QTextStream stream(&buffer);
    gu::osgQuatOut(stream, static_cast<osg::Quat>(*(dialog->parameter)));
    out->setText(buffer);
    
    //more elaborate control with quat selection?
    
    //connect to dialog slots
    
    return out;
  }
  
  QWidget* buildMatrix() const
  {
    QLineEdit* out = new QLineEdit(dialog);
    
    QString buffer;
    QTextStream stream(&buffer);
    gu::osgMatrixOut(stream, static_cast<osg::Matrixd>(*(dialog->parameter)));
    out->setText(buffer);
    
    //more elaborate control with matrix selection?
    
    //connect to dialog slots
    
    return out;
  }
};

//react to value has changed based upon type.
class ValueChangedVisitor : public boost::static_visitor<>
{
public:
  ValueChangedVisitor() = delete;
  ValueChangedVisitor(ParameterDialog *dialogIn) : dialog(dialogIn){assert(dialog);}
  void operator()(double) const {dialog->valueHasChangedDouble();}
  void operator()(int) const {}
  void operator()(bool) const {dialog->valueHasChangedBool();}
  void operator()(const std::string&) const {}
  void operator()(const boost::filesystem::path&) const {dialog->valueHasChangedPath();}
  void operator()(const osg::Vec3d&) const {dialog->valueHasChangedVector();}
  void operator()(const osg::Quat&) const {}
  void operator()(const osg::Matrixd&) const {}
private:
  ParameterDialog *dialog;
};

//react to constant has changed based upon type.
class ConstantChangedVisitor : public boost::static_visitor<>
{
public:
  ConstantChangedVisitor() = delete;
  ConstantChangedVisitor(ParameterDialog *dialogIn) : dialog(dialogIn){assert(dialog);}
  void operator()(double) const {dialog->constantHasChangedDouble();}
  void operator()(int) const {}
  void operator()(bool) const {} //don't think bool should be anything other than constant?
  void operator()(const std::string&) const {}
  void operator()(const boost::filesystem::path&) const {}
  void operator()(const osg::Vec3d&) const {}
  void operator()(const osg::Quat&) const {}
  void operator()(const osg::Matrixd&) const {}
private:
  ParameterDialog *dialog;
};

ParameterDialog::ParameterDialog(ftr::prm::Parameter *parameterIn, const boost::uuids::uuid &idIn):
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
  QString title = feature->getName() + ": ";
  title += QString::fromStdString(parameter->getValueTypeString());
  this->setWindowTitle(title);
  
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  
  QHBoxLayout *editLayout = new QHBoxLayout();
  QLabel *nameLabel = new QLabel(parameter->getName(), this);
  editLayout->addWidget(nameLabel);
  
  WidgetFactoryVisitor fv(this);
  editWidget = boost::apply_visitor(fv, parameter->getVariant());
  editLayout->addWidget(editWidget);
  
  mainLayout->addLayout(editLayout);
}

void ParameterDialog::requestLinkSlot(const QString &stringIn)
{
  //for now we only get here if we have a double. because connect inside double condition.
  boost::uuids::uuid id = gu::stringToId(stringIn.toStdString());
  assert(!id.is_nil());
  
  expr::Manager &eManager = static_cast<app::Application *>(qApp)->getProject()->getManager();
  if (!parameter->isConstant())
  {
    //parameter is already linked.
    assert(eManager.hasParameterLink(parameter->getId()));
    eManager.removeParameterLink(parameter->getId());
  }
  
  assert(eManager.hasFormula(id));
  eManager.addLink(parameter, id);
  
  std::ostringstream gm;
  gm << "Linking parameter " << parameter->getName().toStdString()
  << " to formula " << eManager.getFormulaName(id);
  
  observer->outBlocked(msg::buildGitMessage(gm.str()));
  
  if (prf::manager().rootPtr->dragger().triggerUpdateOnFinish())
    observer->out(msg::Mask(msg::Request | msg::Update));
  
  this->activateWindow();
}

void ParameterDialog::requestUnlinkSlot()
{
  //for now we only get here if we have a double. because connect inside double condition.
  expr::Manager &eManager = static_cast<app::Application *>(qApp)->getProject()->getManager();
  assert(eManager.hasParameterLink(parameter->getId()));
  boost::uuids::uuid fId = eManager.getFormulaLink(parameter->getId());
  eManager.removeParameterLink(parameter->getId());
  //manager sets the parameter to constant or not.
  
  std::ostringstream gm;
  gm << "Unlinking parameter " << parameter->getName().toStdString()
  << " from formula " << eManager.getFormulaName(fId);
  
  //unlinking itself shouldn't trigger an update because the parameter value is still the same.
  
  observer->outBlocked(msg::buildGitMessage(gm.str()));
}

void ParameterDialog::valueHasChanged()
{
  ValueChangedVisitor v(this);
  boost::apply_visitor(v, parameter->getVariant());
}

void ParameterDialog::valueHasChangedDouble()
{
  ExpressionEdit *eEdit = dynamic_cast<ExpressionEdit*>(editWidget);
  assert(eEdit);
  
  lastValue = static_cast<double>(*parameter);
  if (parameter->isConstant())
  {
    eEdit->lineEdit->setText(QString::number(lastValue, 'f', 12));
    eEdit->lineEdit->selectAll();
  }
  //if it is linked we shouldn't need to change.
}

void ParameterDialog::valueHasChangedBool()
{
  QComboBox *cBox = dynamic_cast<QComboBox*>(editWidget);
  assert(cBox);
  
  bool cwv = cBox->currentData().toBool(); //current widget value
  bool cpv = static_cast<bool>(*parameter); //current parameter value
  if (cwv != cpv)
  {
    if (cpv)
      cBox->setCurrentIndex(0);
    else
      cBox->setCurrentIndex(1);
    //setting this will trigger this' changed slot, but at that
    //point combo box value and parameter value will be equal,
    //so, no recurse.
  }
}

void ParameterDialog::valueHasChangedPath()
{
  //find the line edit child widget and get text.
  QLineEdit *lineEdit = editWidget->findChild<QLineEdit*>(QString("TheLineEdit"));
  assert(lineEdit);
  if (!lineEdit)
    return;
  
  lineEdit->setText(QString::fromStdString(static_cast<boost::filesystem::path>(*parameter).string()));
}

void ParameterDialog::valueHasChangedVector()
{
  QLineEdit *xLineEdit = editWidget->findChild<QLineEdit*>(QString("XLineEdit"));
  assert(xLineEdit); if (!xLineEdit) return;
  
  QLineEdit *yLineEdit = editWidget->findChild<QLineEdit*>(QString("YLineEdit"));
  assert(yLineEdit); if (!yLineEdit) return;
  
  QLineEdit *zLineEdit = editWidget->findChild<QLineEdit*>(QString("ZLineEdit"));
  assert(zLineEdit); if (!zLineEdit) return;
  
  osg::Vec3d theVec = static_cast<osg::Vec3d>(*parameter);
  
  xLineEdit->setText(QString::number(theVec.x(), 'f', 12));
  yLineEdit->setText(QString::number(theVec.y(), 'f', 12));
  zLineEdit->setText(QString::number(theVec.z(), 'f', 12));
}

void ParameterDialog::constantHasChanged()
{
  ConstantChangedVisitor v(this);
  boost::apply_visitor(v, parameter->getVariant());
}

void ParameterDialog::constantHasChangedDouble()
{
  ExpressionEdit *eEdit = dynamic_cast<ExpressionEdit*>(editWidget);
  assert(eEdit);
  
  if (parameter->isConstant())
  {
    eEdit->trafficLabel->setTrafficGreenSlot();
    eEdit->lineEdit->setReadOnly(false);
    eEdit->setFocus();
  }
  else
  {
    const expr::Manager &eManager = static_cast<app::Application *>(qApp)->getProject()->getManager();
    assert(eManager.hasParameterLink(parameter->getId()));
    std::string formulaName = eManager.getFormulaName(eManager.getFormulaLink(parameter->getId()));
    
    eEdit->trafficLabel->setLinkSlot();
    eEdit->lineEdit->setText(QString::fromStdString(formulaName));
    eEdit->clearFocus();
    eEdit->lineEdit->deselect();
    eEdit->lineEdit->setReadOnly(true);
  }
  valueHasChangedDouble();
}

void ParameterDialog::featureRemovedDispatched(const msg::Message &messageIn)
{
  prj::Message pMessage = boost::get<prj::Message>(messageIn.payload);
  if(pMessage.feature->getId() == feature->getId())
    this->reject();
}

void ParameterDialog::updateDoubleSlot()
{
  //for now we only get here if we have a double. because connect inside double condition.
  
  //if we are linked, we shouldn't need to do anything.
  if (!parameter->isConstant())
    return;
  
  ExpressionEdit *eEdit = dynamic_cast<ExpressionEdit*>(editWidget);
  assert(eEdit);

  auto fail = [&]()
  {
    eEdit->lineEdit->setText(QString::number(lastValue, 'f', 12));
    eEdit->lineEdit->selectAll();
    eEdit->trafficLabel->setTrafficGreenSlot();
  };

  std::ostringstream gitStream;
  gitStream
    << QObject::tr("Feature: ").toStdString() << feature->getName().toStdString()
    << QObject::tr("    Parameter ").toStdString() << parameter->getName().toStdString();

  //just run it through a string translator and expression manager.
  expr::Manager localManager;
  expr::StringTranslator translator(localManager);
  std::string formula("temp = ");
  formula += eEdit->lineEdit->text().toStdString();
  if (translator.parseString(formula) == expr::StringTranslator::ParseSucceeded)
  {
    localManager.update();
    assert(localManager.getFormulaValueType(translator.getFormulaOutId()) == expr::ValueType::Scalar);
    double value = boost::get<double>(localManager.getFormulaValue(translator.getFormulaOutId()));
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

void ParameterDialog::textEditedDoubleSlot(const QString &textIn)
{
  ExpressionEdit *eEdit = dynamic_cast<ExpressionEdit*>(editWidget);
  assert(eEdit);
  
  eEdit->trafficLabel->setTrafficYellowSlot();
  qApp->processEvents(); //need this or we never see yellow signal.
  
  expr::Manager localManager;
  expr::StringTranslator translator(localManager);
  std::string formula("temp = ");
  formula += textIn.toStdString();
  if (translator.parseString(formula) == expr::StringTranslator::ParseSucceeded)
  {
    localManager.update();
    eEdit->trafficLabel->setTrafficGreenSlot();
    assert(localManager.getFormulaValueType(translator.getFormulaOutId()) == expr::ValueType::Scalar);
    double value = boost::get<double>(localManager.getFormulaValue(translator.getFormulaOutId()));
    eEdit->goToolTipSlot(QString::number(value));
  }
  else
  {
    eEdit->trafficLabel->setTrafficRedSlot();
    int position = translator.getFailedPosition() - 8; // 7 chars for 'temp = ' + 1
    eEdit->goToolTipSlot(textIn.left(position) + "?");
  }
}

void ParameterDialog::boolChangedSlot(int i)
{
  //we don't need to respond to this value change, so block.
  boost::signals2::shared_connection_block block(valueConnection);
  
  bool cwv = static_cast<QComboBox*>(QObject::sender())->itemData(i).toBool(); //current widget value
  if(parameter->setValue(cwv))
  {
    std::ostringstream gitStream;
    gitStream
    << QObject::tr("Feature: ").toStdString() << feature->getName().toStdString()
    << QObject::tr("    Parameter ").toStdString() << parameter->getName().toStdString();
    gitStream  << QObject::tr("    changed to: ").toStdString() << static_cast<bool>(*parameter);
    observer->out(msg::buildGitMessage(gitStream.str()));
    if (prf::manager().rootPtr->dragger().triggerUpdateOnFinish())
      observer->out(msg::Mask(msg::Request | msg::Update));
  }
}

void ParameterDialog::browseForPathSlot()
{
  //we are manually setting the lineEdit so we can block value signal.
  boost::signals2::shared_connection_block block(valueConnection);
  
  //find the line edit child widget and get text.
  QLineEdit *lineEdit = editWidget->findChild<QLineEdit*>(QString("TheLineEdit"));
  assert(lineEdit);
  if (!lineEdit)
    return;
  
  namespace bfs = boost::filesystem;
  bfs::path t = lineEdit->text().toStdString();
  std::string extension = "(*";
  if (t.has_extension())
    extension += t.extension().string();
  else
    extension += ".*";
  extension += ")";
  
  if (parameter->getPathType() == ftr::prm::PathType::Read)
  {
    //read expects the file to exist. start browse from project directory if doesn't exist.
    if (!bfs::exists(t))
      t = static_cast<app::Application*>(qApp)->getProject()->getSaveDirectory();
    
    QString fileName = QFileDialog::getOpenFileName
    (
      this,
      tr("Browse For Read"),
      QString::fromStdString(t.string()),
      QString::fromStdString(extension)
    );
    
    if (!fileName.isEmpty())
      lineEdit->setText(fileName);
  }
  else if (parameter->getPathType() == ftr::prm::PathType::Write)
  {
    if (!bfs::exists(t.parent_path()))
    {
      t = static_cast<app::Application*>(qApp)->getProject()->getSaveDirectory();
      t /= "file" + extension;
    }
    
    QString fileName = QFileDialog::getSaveFileName
    (
      this,
      tr("Browse For Write"),
      QString::fromStdString(t.string()),
      QString::fromStdString(extension)
    );
    
    if (!fileName.isEmpty())
      lineEdit->setText(fileName);
  }
}

void ParameterDialog::vectorChangedSlot()
{
  //block value signal.
  boost::signals2::shared_connection_block block(valueConnection);
  
  QLineEdit *xLineEdit = editWidget->findChild<QLineEdit*>(QString("XLineEdit"));
  assert(xLineEdit); if (!xLineEdit) return;
  
  QLineEdit *yLineEdit = editWidget->findChild<QLineEdit*>(QString("YLineEdit"));
  assert(yLineEdit); if (!yLineEdit) return;
  
  QLineEdit *zLineEdit = editWidget->findChild<QLineEdit*>(QString("ZLineEdit"));
  assert(zLineEdit); if (!zLineEdit) return;
  
  osg::Vec3d out
  (
    xLineEdit->text().toDouble(),
    yLineEdit->text().toDouble(),
    zLineEdit->text().toDouble()
  );
  
  if(parameter->setValue(out))
  {
    QString buffer;
    QTextStream vStream(&buffer);
    gu::osgVectorOut(vStream, static_cast<osg::Vec3d>(*parameter));
    
    std::ostringstream gitStream;
    gitStream
    << QObject::tr("Feature: ").toStdString() << feature->getName().toStdString()
    << QObject::tr("    Parameter ").toStdString() << parameter->getName().toStdString();
    gitStream  << QObject::tr("    changed to: ").toStdString() << buffer.toStdString();
    observer->out(msg::buildGitMessage(gitStream.str()));
    if (prf::manager().rootPtr->dragger().triggerUpdateOnFinish())
      observer->out(msg::Mask(msg::Request | msg::Update));
  }
}

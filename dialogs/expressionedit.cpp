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

#include <boost/uuid/uuid.hpp>

#include <QCoreApplication>
#include <QResizeEvent>
#include <QPainter>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QAction>
#include <QMimeData>

#include <tools/idtools.h>
#include <dialogs/expressionedit.h>

using namespace dlg;

TrafficLabel::TrafficLabel(QWidget* parentIn) : QLabel(parentIn)
{
  setContentsMargins(0, 0, 0, 0);
  
  setContextMenuPolicy(Qt::ActionsContextMenu);
  unlinkAction = new QAction(tr("unlink"), this);
  connect(unlinkAction, SIGNAL(triggered()), this, SIGNAL(requestUnlinkSignal()));
}

void TrafficLabel::resizeEvent(QResizeEvent *event)
{
  iconSize = event->size().height();
  updatePixmaps();
}

void TrafficLabel::updatePixmaps()
{
  assert(iconSize > 0);
  trafficRed = buildPixmap(":/resources/images/trafficRed.svg");
  trafficYellow = buildPixmap(":/resources/images/trafficYellow.svg");
  trafficGreen = buildPixmap(":/resources/images/trafficGreen.svg");
  link = buildPixmap(":resources/images/linkIcon.svg");
}

QPixmap TrafficLabel::buildPixmap(const QString &resourceNameIn)
{
  QPixmap temp = QPixmap(resourceNameIn).scaled(iconSize, iconSize, Qt::KeepAspectRatio);
  QPixmap out(iconSize, iconSize);
  QPainter painter(&out);
  painter.fillRect(out.rect(), this->palette().color(QPalette::Window));
  painter.drawPixmap(out.rect(), temp, temp.rect());
  painter.end();
  
  return out;
}

void TrafficLabel::setTrafficRedSlot()
{
  setPixmap(trafficRed);
  this->removeAction(unlinkAction);
}

void TrafficLabel::setTrafficYellowSlot()
{
  setPixmap(trafficYellow);
  this->removeAction(unlinkAction);
}

void TrafficLabel::setTrafficGreenSlot()
{
  setPixmap(trafficGreen);
  this->removeAction(unlinkAction);
}

void TrafficLabel::setLinkSlot()
{
  setPixmap(link);
  this->addAction(unlinkAction);
}

ExpressionEdit::ExpressionEdit(QWidget* parent, Qt::WindowFlags f) :
  QWidget(parent, f)
{
  setupGui();
}

void ExpressionEdit::setupGui()
{
  this->setContentsMargins(0, 0, 0, 0);
  QHBoxLayout *layout = new QHBoxLayout();
  layout->setSpacing(0);
  layout->setContentsMargins(0, 0, 0, 0);
  
  lineEdit = new QLineEdit(this);
  layout->addWidget(lineEdit);
  
  trafficLabel = new TrafficLabel(this);
  layout->addWidget(trafficLabel);
  
  this->setLayout(layout);
  this->setFocusProxy(lineEdit);
  
  setAcceptDrops(false); //the edit line accepts drops by default.
  lineEdit->setWhatsThis(tr("Constant value for editing or name of linked expression"));
  trafficLabel->setWhatsThis(tr("Status of parse or linked icon. When linked, right click to unlink"));
}

void ExpressionEdit::goToolTipSlot(const QString &tipIn)
{
  lineEdit->setToolTip(tipIn);
  
  QPoint point(0.0, -1.5 * (lineEdit->frameGeometry().height()));
  QHelpEvent *toolTipEvent = new QHelpEvent(QEvent::ToolTip, point, lineEdit->mapToGlobal(point));
  qApp->postEvent(lineEdit, toolTipEvent);
}

bool ExpressionEditFilter::eventFilter(QObject *obj, QEvent *event)
{
  auto getId = [](const QString &stringIn)
  {
    boost::uuids::uuid idOut = gu::createNilId();
    if (stringIn.startsWith("ExpressionId;"))
    {
      QStringList split = stringIn.split(";");
      if (split.size() == 2)
        idOut = gu::stringToId(split.at(1).toStdString());
    }
    return idOut;
  };
  
  if (event->type() == QEvent::DragEnter)
  {
    QDragEnterEvent *dEvent = dynamic_cast<QDragEnterEvent*>(event);
    assert(dEvent);
    
    if (dEvent->mimeData()->hasText())
    {
      QString textIn = dEvent->mimeData()->text();
      boost::uuids::uuid id = getId(textIn);
      if (!id.is_nil())
        dEvent->acceptProposedAction();
    }
    return true;
  }
  else if (event->type() == QEvent::Drop)
  {
    QDropEvent *dEvent = dynamic_cast<QDropEvent*>(event);
    assert(dEvent);
    
    if (dEvent->mimeData()->hasText())
    {
      QString textIn = dEvent->mimeData()->text();
      boost::uuids::uuid id = getId(textIn);
      if (!id.is_nil())
      {
        dEvent->acceptProposedAction();
        Q_EMIT requestLinkSignal(QString::fromStdString(gu::idToString(id)));
      }
    }
    
    return true;
  }
  else
    return QObject::eventFilter(obj, event);
}

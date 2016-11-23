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

#ifndef PARAMETERDIALOG_H
#define PARAMETERDIALOG_H

#include <QDialog>
#include <QLineEdit>

#ifndef Q_MOC_RUN
#include <boost/signals2.hpp>
#endif

class QKeyEvent;
class QButton;
class QEnterEvent;
class QDropEvent;
class QLabel;

namespace boost{namespace uuids{class uuid;}}

namespace ftr{class Parameter; class Base;}
namespace msg{class Message;}

namespace dlg
{
  class EnterEdit : public QLineEdit
  {
  Q_OBJECT
  public:
    explicit EnterEdit(QWidget *parentIn) : QLineEdit(parentIn){}
    
  Q_SIGNALS:
    void goUpdateSignal();
    
  protected:
    virtual void keyPressEvent(QKeyEvent*) override;
  };
  
  class ParameterDialog : public QDialog
  {
    Q_OBJECT
  public:
    ParameterDialog(ftr::Parameter *parameterIn, const boost::uuids::uuid &idIn);
    ftr::Parameter *parameter = nullptr;
    
    typedef boost::signals2::signal<void (const msg::Message &)> MessageOutSignal;
    boost::signals2::connection connectMessageOut(const MessageOutSignal::slot_type &subscriber)
    {
      return messageOutSignal.connect(subscriber);
    }
    
  protected:
    virtual void keyPressEvent(QKeyEvent*) override;
    virtual void dragEnterEvent(QDragEnterEvent *) override;
    virtual void dropEvent(QDropEvent *) override;

  private:
    void buildGui();
    void valueHasChanged();
    void constantHasChanged();
    void messageInSlot(const msg::Message &);
    ftr::Base *feature;
    EnterEdit *editLine;
    QPushButton *linkButton;
    QLabel *linkLabel;
    MessageOutSignal messageOutSignal;
    double lastValue;
    QPixmap trafficRed;
    QPixmap trafficYellow;
    QPixmap trafficGreen;
    QLabel *trafficLabel;
  private Q_SLOTS:
    void updateSlot();
    void linkButtonClickedSlot(bool checkedState);
    void textEditedSlot(const QString &);
  };
}

#endif // PARAMETERDIALOG_H

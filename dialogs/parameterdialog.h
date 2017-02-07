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

#include <memory>

#ifndef Q_MOC_RUN
#include <boost/signals2/connection.hpp>
#endif

#include <QDialog>
#include <QLineEdit>

class QKeyEvent;
class QButton;
class QEnterEvent;
class QDropEvent;
class QLabel;

namespace boost{namespace uuids{class uuid;}}

namespace ftr{class Parameter; class Base;}
namespace msg{class Message; class Observer;}

namespace dlg
{
  class ExpressionEdit;
  class ParameterDialog : public QDialog
  {
    Q_OBJECT
  public:
    ParameterDialog(ftr::Parameter *parameterIn, const boost::uuids::uuid &idIn);
    virtual ~ParameterDialog() override;
    ftr::Parameter *parameter = nullptr;

  private:
    void buildGui();
    void valueHasChanged();
    void constantHasChanged();
    boost::signals2::connection valueConnection;
    boost::signals2::connection constantConnection;
    std::unique_ptr<msg::Observer> observer;
    void featureRemovedDispatched(const msg::Message &);
    ftr::Base *feature;
    ExpressionEdit *editLine;
    double lastValue;
  private Q_SLOTS:
    void updateSlot();
    void textEditedSlot(const QString &);
    void requestLinkSlot(const QString &);
    void requestUnlinkSlot();
  };
}

#endif // PARAMETERDIALOG_H

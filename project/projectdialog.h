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

#ifndef DIALOG_H
#define DIALOG_H

#include <qt4/QtGui/QDialog>
#include <QDir>

namespace Ui{ class projectDialog; }

namespace prj
{
  class Dialog : public QDialog
  {
      Q_OBJECT
  public:
    enum class Result
    {
      None = 0, //!< error
      Open, //!< open a project
      New, //!< create a project
      Recent, //!< picked from recent list
      Cancel //!< dialog was cancelled
    };
    Dialog(QWidget* = 0);
    ~Dialog();
    Result getResult(){return result;}
    QDir getDirectory(){return directory;}
  private:
    Ui::projectDialog* ui;
    Result result;
    QDir directory;
    
    void addToRecentList();
    void populateRecentList();
    bool validateDir(const QDir &dir);
    
  private Q_SLOTS:
    void goNewSlot();
    void goOpenSlot();
    void goRecentSlot(int, int);
  };
}

#endif // DIALOG_H

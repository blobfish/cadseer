/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2018  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef DLG_COMMITWIDGET_H
#define DLG_COMMITWIDGET_H

#include <QWidget>

#include <project/libgit2pp/src/commit.hpp>

class QLabel;
class QDateTimeEdit;
class QTextEdit;

namespace dlg
{
  /**
  * @todo write docs
  */
  class CommitWidget : public QWidget
  {
  public:
    CommitWidget(QWidget *parent);
    virtual ~CommitWidget() override;
    
    void setCommit(const git2::Commit&);
    void clear();
    void setShaLength(std::size_t);
    
    QLabel *shaLabel;
    QLabel *authorNameLabel;
    QLabel *authorEMailLabel;
    QDateTimeEdit *dateTimeEdit;
    QTextEdit *textEdit;
    
  private:
    void buildGui();
    void update();
    void updateSha();
    
    git2::Commit commit;
    std::size_t shaLength = 12;
  };
}

#endif // DLG_COMMITWIDGET_H

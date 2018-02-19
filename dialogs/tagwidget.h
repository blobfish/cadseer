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

#ifndef DLG_TAGWIDGET_H
#define DLG_TAGWIDGET_H

#include <QWidget>

#include <project/libgit2pp/src/tag.hpp>

class QLabel;
class QDateTimeEdit;
class QTextEdit;

namespace dlg
{
  /**
  * @todo write docs
  */
  class TagWidget : public QWidget
  {
  public:
    TagWidget(QWidget *parent);
    virtual ~TagWidget() override;
    
    void setTag(const git2::Tag&);
    void clear();
    void setShaLength(std::size_t);
    
    QLabel *nameLabel;
    QLabel *shaTargetLabel;
    QLabel *authorNameLabel;
    QLabel *authorEMailLabel;
    QDateTimeEdit *dateTimeEdit;
    QTextEdit *textEdit;
    
  protected:
    void buildGui();
    void update();
    void updateSha();
    
    git2::Tag tag;
    std::size_t shaLength = 12;
  };
}

#endif // DLG_TAGWIDGET_H

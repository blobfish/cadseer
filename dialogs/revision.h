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

#ifndef DLG_REVISION_H
#define DLG_REVISION_H

#include <memory>

#include <QDialog>

class QListWidget;
class QTextEdit;
class QTabWidget;
class QAction;

namespace msg{class Message; class Observer;}

namespace dlg
{
  class CommitWidget;
  class TagWidget;
  
  class UndoPage : public QWidget
  {
    Q_OBJECT
  public:
    UndoPage(QWidget*);
    virtual ~UndoPage() override;
    
  protected:
    std::unique_ptr<msg::Observer> observer;
    struct Data;
    std::unique_ptr<Data> data;
    QListWidget *commitList;
    CommitWidget *commitWidget;
    
    void buildGui();
    void fillInCommitList();
    
    void commitRowChangedSlot(int);
    void resetActionSlot();
  };
  
  class AdvancedPage : public QWidget
  {
    Q_OBJECT
  public:
    AdvancedPage(QWidget*);
    virtual ~AdvancedPage() override;
    
  protected:
    std::unique_ptr<msg::Observer> observer;
    struct Data;
    std::unique_ptr<Data> data;
    QListWidget *tagList;
    TagWidget *tagWidget;
    QAction *createTagAction;
    QAction *checkoutTagAction;
    QAction *destroyTagAction;
    
    void init();
    void buildGui();
    void fillInTagList();
    void setCurrentHead();
    
    void tagRowChangedSlot(int);
    void createTagSlot();
    void destroyTagSlot();
    void checkoutTagSlot();
  };
  
  /**
  * @todo write docs
  */
  class Revision : public QDialog
  {
    Q_OBJECT
  public:
    Revision(QWidget*);
    virtual ~Revision() override;
    
  protected:
    virtual void closeEvent (QCloseEvent*) override;
    void buildGui();
    
    std::unique_ptr<msg::Observer> observer;
    struct Data;
    QTabWidget *tabWidget;
  };
}

#endif // DLG_REVISION_H

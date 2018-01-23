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

#ifndef DLG_SELECTIONBUTTON_H
#define DLG_SELECTIONBUTTON_H

#include <memory>

#include <QPushButton>

#include <selection/message.h>
#include <selection/definitions.h>

class QHideEvent;
class QShowEvent;

namespace msg{class Message; class Observer;}

namespace dlg
{
  /* a QPushButton connected to selection mechanism.
   * 
   * sync the selection upon showing and clears
   * selection upon hiding.
   */
  class SelectionButton : public QPushButton
  {
    Q_OBJECT
  public:
    SelectionButton() = delete;
    SelectionButton(QWidget*);
    SelectionButton(const QString&, QWidget*);
    SelectionButton(const QIcon&, const QString&, QWidget*);
    virtual ~SelectionButton() override;
    
    slc::Mask mask; //!< to control selection.
    void syncToSelection(); //!< this doesn't clear the current selection.
    const slc::Messages& getMessages() const {return messages;}
    void setMessages(const slc::Messages&);
    void setMessages(const slc::Message&);
    void addMessage(const slc::Message&);
    void addMessages(const slc::Messages&);
    
    bool isSingleSelection = false;
    QString statusPrompt;
    
  Q_SIGNALS:
    void dirty();
    void advance(); //!< used in single selection mode to advance to next button.
    
  protected:
    virtual void hideEvent(QHideEvent *) override;
    virtual void showEvent(QShowEvent *) override;
    
  private Q_SLOTS:
    void toggledSlot(bool);
    
  private:
    std::unique_ptr<msg::Observer> observer;
    void setupDispatcher();
    void selectionAdditionDispatched(const msg::Message&);
    void selectionSubtractionDispatched(const msg::Message&);
    void selectionMaskDispatched(const msg::Message&);
    
    slc::Messages messages;
  };
}

#endif // DLG_SELECTIONBUTTON_H

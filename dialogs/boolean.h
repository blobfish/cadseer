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

#ifndef DLG_BOOLEAN_H
#define DLG_BOOLEAN_H

#include <memory>

#include <boost/uuid/uuid.hpp>

#include <QDialog>

class QButtonGroup;

namespace ftr{class Intersect; class Subtract; class Union;}
namespace msg{class Message; class Observer;}
namespace dlg{class SelectionButton;}

namespace dlg
{
  /**
  * @todo write docs
  */
  class Boolean : public QDialog
  {
    Q_OBJECT
  public:
    Boolean() = delete;
    Boolean(ftr::Intersect*, QWidget*);
    Boolean(ftr::Subtract*, QWidget*);
    Boolean(ftr::Union*, QWidget*);
    virtual ~Boolean() override;
    
    void setEditDialog();
    void buildGui();
    
  public Q_SLOTS:
    virtual void reject() override;
    virtual void accept() override;
    
  private:
    std::unique_ptr<msg::Observer> observer;
    
    ftr::Intersect *intersect = nullptr;
    ftr::Subtract *subtract = nullptr;
    ftr::Union *onion = nullptr;
    
    QButtonGroup *bGroup;
    SelectionButton *targetButton;
    SelectionButton *toolButton;
    
    bool isAccepted = false;
    bool isEditDialog = false;
    
    std::vector<boost::uuids::uuid> leafChildren; //used to save and restore state.
    boost::uuids::uuid booleanId;
    
    void init();
    void finishDialog();
    
  private Q_SLOTS:
    void advanceSlot(); //!< move to next button in selection group.
  };
}

#endif // DLG_BOOLEAN_H

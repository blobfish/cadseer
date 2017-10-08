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

#ifndef DLG_STRIP_H
#define DLG_STRIP_H

#include <memory>

#include <boost/uuid/uuid.hpp>

#include <QDialog>
#include <QLabel>

class QTabWidget;
class QWidget;
class QLineEdit;
class QComboBox;
class QListWidget;
class QDragEnterEvent;
class QDragMoveEvent;
class QDragLeaveEvent;
class QDropEvent;
class QButtonGroup;
class QCheckBox;

namespace ftr{class Strip;}
namespace msg{class Message; class Observer;}
namespace dlg{class SelectionButton;}

namespace dlg
{
  /*! This is a simple QLabel with a picture
   * to absorb drags.
   */
  class TrashCan : public QLabel
  {
    Q_OBJECT
  public:
    TrashCan() = delete;
    TrashCan(QWidget*);
  protected:
    void dragEnterEvent(QDragEnterEvent *) override;
    void dragMoveEvent(QDragMoveEvent *) override;
    void dragLeaveEvent(QDragLeaveEvent *) override;
    void dropEvent(QDropEvent *) override;
  };
  
  class Strip : public QDialog
  {
    Q_OBJECT
  public:
    Strip() = delete;
    Strip(ftr::Strip*, QWidget*);
    virtual ~Strip() override;
    
    void setPartId(const boost::uuids::uuid&);
    void setBlankId(const boost::uuids::uuid&);
    void setNestId(const boost::uuids::uuid&);
    
  public Q_SLOTS:
    virtual void reject() override;
    virtual void accept() override;
    
  private:
    std::unique_ptr<msg::Observer> observer;
    ftr::Strip *strip;
    
    QTabWidget *tabWidget;
    SelectionButton *partButton;
    SelectionButton *blankButton;
    SelectionButton *nestButton;
    QLabel *partIdLabel;
    QLabel *blankIdLabel;
    QLabel *nestIdLabel;
    QButtonGroup *bGroup;
    QLineEdit *pNameEdit;
    QLineEdit *pNumberEdit;
    QLineEdit *pRevisionEdit;
    QLineEdit *pmTypeEdit;
    QLineEdit *pmThicknessEdit;
    QLineEdit *sQuoteNumberEdit;
    QLineEdit *sCustomerNameEdit;
    QComboBox *sPartSetupCombo;
    QComboBox *sProcessTypeCombo;
    QLineEdit *sAnnualVolumeEdit;
    QListWidget *stationsList;
    QLabel *pLabel;
    QCheckBox *acCheckBox;
    
    bool isAccepted = false;
    
    void buildGui();
    void initGui();
    QWidget* buildInputPage();
    QWidget* buildPartPage();
    QWidget* buildStripPage();
    QWidget* buildStationPage();
    QWidget* buildPicturePage();
    void finishDialog();
    
  private Q_SLOTS:
    void takePictureSlot();
    void loadLabelPixmapSlot();
    void updatePartIdSlot();
    void updateBlankIdSlot();
    void updateNestIdSlot();
    void advanceSlot(); //!< move to next button in selection group.
  };
}

#endif // DLG_STRIP_H

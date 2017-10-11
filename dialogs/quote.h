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

#ifndef DLG_QUOTE_H
#define DLG_QUOTE_H

#include <memory>

#include <boost/uuid/uuid.hpp>

#include <QDialog>
#include <QLabel>

class QTabWidget;
class QWidget;
class QLineEdit;
class QComboBox;
class QButtonGroup;
class QLabel;

namespace ftr{class Quote;}
namespace msg{class Message; class Observer;}
namespace dlg{class SelectionButton;}

namespace dlg
{
  class Quote : public QDialog
  {
    Q_OBJECT
  public:
    Quote() = delete;
    Quote(ftr::Quote*, QWidget*);
    virtual ~Quote() override;
    
    void setStripId(const boost::uuids::uuid&);
    void setDieSetId(const boost::uuids::uuid&);
    
  public Q_SLOTS:
    virtual void reject() override;
    virtual void accept() override;
    
  private:
    std::unique_ptr<msg::Observer> observer;
    ftr::Quote *quote;
    
    QTabWidget *tabWidget;
    SelectionButton *stripButton;
    SelectionButton *diesetButton;
    QLabel *stripIdLabel;
    QLabel *diesetIdLabel;
    QLineEdit *tFileEdit; //!< template sheet in.
    QLineEdit *oFileEdit; //!< quote sheet out
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
    QLabel *pLabel;
    
    bool isAccepted = false;
    
    void buildGui();
    void initGui();
    QWidget* buildInputPage();
    QWidget* buildPartPage();
    QWidget* buildQuotePage();
    QWidget* buildPicturePage();
    void finishDialog();
    
  private Q_SLOTS:
    void takePictureSlot();
    void loadLabelPixmapSlot();
    void updateStripIdSlot();
    void updateDiesetIdSlot();
    void advanceSlot(); //!< move to next button in selection group.
    void browseForTemplateSlot();
    void browseForOutputSlot();
  };
}

#endif // DLG_QUOTE_H

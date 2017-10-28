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

#ifndef DLG_PREFERENCES_H
#define DLG_PREFERENCES_H

#include <QDialog>

namespace Ui{ class dialog; }
namespace prf{class Manager;}
class SplitterDecorated;

namespace dlg
{
  class Manager;
  
  class Preferences : public QDialog
  {
      Q_OBJECT
  public:
    Preferences(prf::Manager *, QWidget* = 0);
    ~Preferences();
    bool isVisualDirty(){return visualDirty;}
    bool isHiddenLinesDirty(){return hiddenLinesDirty;}
  public Q_SLOTS:
    virtual void accept() override;
    void basePathBrowseSlot();
    void quoteTemplateBrowseSlot();
  private:
    void initialize();
    void setupFeatureSplitter();
    void updateVisual();
    void updateDragger();
    void updateProject();
    void updateGesture();
    void updateFeature();
    Ui::dialog* ui;
    prf::Manager *manager;
    bool visualDirty = false;
    bool hiddenLinesDirty = false;
    SplitterDecorated *fsSplitter;
  };
}

#endif // DLG_PREFERENCES_H

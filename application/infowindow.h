/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef APP_INFOWINDOW_H
#define APP_INFOWINDOW_H

#include <memory>

#include <QTextEdit>
#include <QDialog>

class QShowEvent;
class QResizeEvent;
class QCloseEvent;

namespace msg{class Message; class Observer;}

namespace app
{
    class InfoWindow : public QTextEdit
    {
        Q_OBJECT
    public:
        explicit InfoWindow(QWidget *parent = 0);
        virtual ~InfoWindow() override;
        
    private:
        std::unique_ptr<msg::Observer> observer;
        void setupDispatcher();
        void infoTextDispatched(const msg::Message&);
    };
    
    class InfoDialog : public QDialog
    {
        Q_OBJECT
    public:
        explicit InfoDialog(QWidget *parent = 0);
        InfoWindow *infoWindow;
        void restoreSettings();
        void saveSettings();
    protected:
        virtual void resizeEvent(QResizeEvent*) override;
        virtual void showEvent(QShowEvent*) override;
        virtual void closeEvent(QCloseEvent*) override;
    };
}

#endif // APP_INFOWINDOW_H

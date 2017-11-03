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

#include <QTimer>

#include <application/application.h>
#include <application/mainwindow.h>

int main( int argc, char** argv )
{
  app::Application app(argc, argv);
  QObject::connect(&app, SIGNAL(aboutToQuit()), &app, SLOT(quittingSlot()));

  app.initializeSpaceball();
  app.getMainWindow()->showMaximized();
  QTimer::singleShot(0, &app, SLOT(appStartSlot()));
  
  return app.exec();
}

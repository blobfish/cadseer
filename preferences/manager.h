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

#ifndef MANAGER_H
#define MANAGER_H

#include <memory>

#include <QDir>

class Root;

namespace Preferences
{
  class Manager
  {
  public:
    Manager();
    std::unique_ptr<Root> rootPtr;
    void saveConfig();
    bool isOk(){return ok;}
  private:
    bool createDefault();
    void setup();
    bool readConfig();
    QDir appDirectory;
    QString fileNameXML;
    QString filePathXML;
    QString filePathXSD;
    bool ok;
  };
}

#endif // MANAGER_H

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

#ifndef CMD_INTERSECT_H
#define CMD_INTERSECT_H

#include <command/base.h>

namespace dlg{class Boolean;}
namespace ftr{class Intersect;}

namespace cmd
{
  /**
  * @todo write docs
  */
  class Intersect : public Base
  {
  public:
    Intersect();
    virtual ~Intersect() override;
    
    virtual std::string getCommandName() override{return "Intersect";}
    virtual std::string getStatusMessage() override;
    virtual void activate() override;
    virtual void deactivate() override;
  private:
    bool firstRun = true;
    dlg::Boolean *dialog = nullptr;
    
    void go();
  };
  
  /**
  * @todo write docs
  */
  class IntersectEdit : public Base
  {
  public:
    IntersectEdit(ftr::Base*);
    virtual ~IntersectEdit() override;
    
    virtual std::string getCommandName() override{return "Intersect Edit";}
    virtual std::string getStatusMessage() override;
    virtual void activate() override;
    virtual void deactivate() override;
  private:
    dlg::Boolean *dialog = nullptr;
    ftr::Intersect *intersect = nullptr;
  };
}

#endif // CMD_INTERSECT_H

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

#ifndef CMD_UNION_H
#define CMD_UNION_H

#include <command/base.h>

namespace dlg{class Boolean;}
namespace ftr{class Union;}

namespace cmd
{
  /**
  * @todo write docs
  */
  class Union : public Base
  {
  public:
    Union();
    virtual ~Union() override;
    
    virtual std::string getCommandName() override{return "Union";}
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
  class UnionEdit : public Base
  {
  public:
    UnionEdit(ftr::Base*);
    virtual ~UnionEdit() override;
    
    virtual std::string getCommandName() override{return "Union Edit";}
    virtual std::string getStatusMessage() override;
    virtual void activate() override;
    virtual void deactivate() override;
  private:
    dlg::Boolean *dialog = nullptr;
    ftr::Union *onion= nullptr;
  };
}

#endif // UNION_H

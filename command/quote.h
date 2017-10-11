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

#ifndef CMD_QUOTE_H
#define CMD_QUOTE_H

#include <command/base.h>

namespace dlg{class Quote;}
namespace ftr{class Quote;}

namespace cmd
{
  class Quote : public Base
  {
  public:
    Quote();
    virtual ~Quote() override;
    
    virtual std::string getCommandName() override{return "Quote";}
    virtual std::string getStatusMessage() override;
    virtual void activate() override;
    virtual void deactivate() override;
  private:
    dlg::Quote *dialog = nullptr;
    void go();
  };
  
  class QuoteEdit : public Base
  {
  public:
    QuoteEdit(ftr::Base*);
    virtual ~QuoteEdit() override;
    
    virtual std::string getCommandName() override{return "Quote Edit";}
    virtual std::string getStatusMessage() override;
    virtual void activate() override;
    virtual void deactivate() override;
  private:
    dlg::Quote *dialog = nullptr;
    ftr::Quote *quote = nullptr;
    void go();
  };
}

#endif // CMD_QUOTE_H

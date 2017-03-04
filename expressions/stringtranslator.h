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

#ifndef EXPR_STRINGTRANSLATOR_H
#define EXPR_STRINGTRANSLATOR_H

#include <memory>

#include <boost/uuid/uuid.hpp>

namespace expr
{
  class StringTranslatorStow;
  class Manager;
  
  class StringTranslator
  {
    public:
      enum TotalState{None, ParseFailed, ParseSucceeded};
      
      StringTranslator(Manager &);
      ~StringTranslator();
      
      //! parse the string.
      TotalState parseString(const std::string &);
      //! where in the string the parse failed or -1 for success.
      int getFailedPosition() const;
      //! failure message. hint on why parsing failed
      std::string getFailureMessage() const;
      //! get the entire string representation of the formula with id.
      std::string buildStringAll(const boost::uuids::uuid &) const;
      //! Build the RHS string. Has no name or equals sign.
      std::string buildStringRhs(const boost::uuids::uuid &) const;
      //! Get the id of the newly created or edit formula
      boost::uuids::uuid getFormulaOutId() const;
      //! the underlying Manager.
      Manager& eManager;
    private:
      std::unique_ptr<StringTranslatorStow> translatorStow;
  };
}

#endif // EXPR_STRINGTRANSLATOR_H

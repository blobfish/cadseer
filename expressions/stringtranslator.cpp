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

#include <iostream>
#include <string>


#include <QString>

#include <expressions/stringtranslatorstow.h>
#include <expressions/stringtranslator.h>
#include <expressions/manager.h>

using namespace expr;

StringTranslator::StringTranslator(Manager &eManagerIn) : eManager(eManagerIn)
{
  translatorStow = std::move(std::unique_ptr<StringTranslatorStow>(new StringTranslatorStow(eManagerIn)));
}

StringTranslator::~StringTranslator(){}

StringTranslator::TotalState StringTranslator::parseString(const std::string &formula)
{
  translatorStow->parseString(formula);
  if (translatorStow->failedPosition == -1)
    return TotalState::ParseSucceeded;
  else
    return TotalState::ParseFailed;
}

int StringTranslator::getFailedPosition() const
{
  return translatorStow->failedPosition;
}

std::string StringTranslator::getFailureMessage() const
{
  return translatorStow->failureMessage;
}

std::string StringTranslator::buildStringAll(const boost::uuids::uuid& idIn) const
{
  return translatorStow->buildStringAll(idIn);
}

std::string StringTranslator::buildStringRhs(const boost::uuids::uuid& idIn) const
{
  return translatorStow->buildStringRhs(idIn);
}

boost::uuids::uuid StringTranslator::getFormulaOutId() const
{
  return translatorStow->getFormulaOutId();
}

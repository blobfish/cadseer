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

#ifndef EXPR_FORMULALINK_H
#define EXPR_FORMULALINK_H

#include <boost/uuid/uuid.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace expr
{
  /*! @brief Link between formula and parameters
   * 
   * boost multi_index link.
   */
  struct FormulaLink
  {
    boost::uuids::uuid parameterId;
    boost::uuids::uuid formulaId;
    ftr::prm::Parameter *parameter = nullptr;
    
    //@{
    //! used as tags.
    struct ByParameterId{};
    struct ByFormulaId{};
    //@}
  };
  
  namespace BMI = boost::multi_index;
  //! Container type to hold formula to property links.
  typedef boost::multi_index_container
  <
    FormulaLink,
    BMI::indexed_by
    <
      BMI::ordered_unique //parameter can only be linked to one formula
      <
        BMI::tag<FormulaLink::ByParameterId>,
        BMI::member<FormulaLink, boost::uuids::uuid, &FormulaLink::parameterId>
      >,
      BMI::ordered_non_unique //formula can be linked to many parameters.
      <
        BMI::tag<FormulaLink::ByFormulaId>,
        BMI::member<FormulaLink, boost::uuids::uuid, &FormulaLink::formulaId>
      >
    >
  > FormulaLinkContainerType;
  
  //! so I can forward declare.
  struct FormulaLinksWrapper
  {
    FormulaLinkContainerType container;
  };
}

#endif // EXPR_FORMULALINK_H

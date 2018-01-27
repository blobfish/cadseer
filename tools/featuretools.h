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

#ifndef TLS_FEATURETOOLS_H
#define TLS_FEATURETOOLS_H

#include <vector>

#include <boost/uuid/uuid.hpp>

namespace ftr{class Base; class Pick; class ShapeHistory;}

namespace tls
{
  /*! @brief Get feature and shape id pairs.
   * 
   * @param features input features that contain the result of the id.
   * @param picks picks to resolve.
   * @param pHistory project history.
   * @return vector of pairs containing the parent feature pointer and the shapeid.
   * @note pilot test for this is the booleans.
   */
  std::vector<std::pair<const ftr::Base*, boost::uuids::uuid>>
  resolvePicks
  (
    const std::vector<const ftr::Base*> &features,
    std::vector<ftr::Pick> picks, //make a work copy of the picks.
    const ftr::ShapeHistory &pHistory
  );
  
  /*! @brief Same as overloaded. Just convenience.

   */
  std::vector<std::pair<const ftr::Base*, boost::uuids::uuid>>
  resolvePicks
  (
    const ftr::Base *feature,
    const ftr::Pick &pick,
    const ftr::ShapeHistory &pHistory
  );
}

#endif // TLS_FEATURETOOLS_H

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

#ifndef FTR_UPDATEPAYLOAD_H
#define FTR_UPDATEPAYLOAD_H

#include <map>

namespace ftr
{
  class ShapeHistory;
  class Base;
  
  /*! @brief Update information needed by features.
   * 
   * had a choice to make. Feature update will need shapeHistory
   * for resolving references. Features need to fill in shape history
   * even when the update isn't needed. so shape history passed into update
   * is const and we have a virtual method to fill in shape history.
   */
  class UpdatePayload
  {
  public:
    typedef std::multimap<std::string, const Base*> UpdateMap;
    
    UpdatePayload(const UpdateMap &updateMapIn, const ShapeHistory &shapeHistoryIn) :
    updateMap(updateMapIn),
    shapeHistory(shapeHistoryIn)
    {}
    
    std::vector<const Base*> getFeatures(const std::string &tag) const;
    
    const UpdateMap &updateMap;
    const ShapeHistory &shapeHistory;
    
    static std::vector<const Base*> getFeatures(const UpdateMap &updateMapIn, const std::string &tag);
  };
  
}

#endif // FTR_UPDATEPAYLOAD_H

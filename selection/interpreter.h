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

#ifndef SLC_INTERPRETER_H
#define SLC_INTERPRETER_H

#include <selection/container.h>
#include <selection/intersection.h>

namespace slc
{
  /*! @brief Interpretation from intersector to event handler.
   *
   * Converts osgUtil::Polytope::Intersections(array) to a
   * slc::Containers(array).
   * 
   * When dealing with endpoints, pointLocation member of slc::container should
   * be accurate as we use mdv::connector to get the location from the occ model.
   * This is not true for edges and faces. In those cases the user will be responsible
   * for getting a 'true' location from the occ shape using the approximate 'pointLocation'
   */ 
  class Interpreter
  {
  public:
    Interpreter(const Intersections&, Mask);
    Containers containersOut;
  private:
    void go();
    const Intersections &intersections;
    Mask selectionMask;
  };
}
#endif // SLC_INTERPRETER_H

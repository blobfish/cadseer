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

#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <osgUtil/LineSegmentIntersector>

#include <selection/container.h>

namespace slc
{
  /*! @brief Interpretation from intersector to event handler.
   *
   * Converts osgUtil::LineSegment::Intersections(array) to a
   * slc::Containers(array).
   */ 
  class Interpreter
  {
  public:
    Interpreter(const osgUtil::LineSegmentIntersector::Intersections &intersectionsIn, std::size_t selectionMaskIn);
    Containers containersOut;
  private:
    void go();
    const osgUtil::LineSegmentIntersector::Intersections &intersections;
    std::size_t selectionMask;
  };
}
#endif // INTERPRETER_H

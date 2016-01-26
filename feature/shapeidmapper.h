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

#ifndef FTR_SHAPEIDMAPPER_H
#define FTR_SHAPEIDMAPPER_H

#include <feature/maps.h>

class BRepBuilderAPI_MakeShape;

/* These are general shape mapping functions that may be useful
 * for all features.
 */

namespace ftr
{
  /*! Create a container with all the shapes of shapeIn. ids will be nil.*/
  ResultContainer createInitialContainer(const TopoDS_Shape &shapeIn);
  
  /*! copy ids from source to target if the shapes match. based upon TopoDS_Shape::IsSame.
   * this will overwrite the id in target whether it is nil or not.
   */
  void shapeMatch(const ResultContainer &source, ResultContainer &target);
  
 /*! Copy id from source to target based on unique types.
  * will only copy the id into target if each container only contains 1 shape of
  * the specified type and if the shapes id in target container is_nil
  */
  void uniqueTypeMatch(const ResultContainer &source, ResultContainer &target);
  
  /*! Copy outer wire ids from source to target based on face id matching.
  */
  void outerWireMatch(const ResultContainer &source, ResultContainer &target);
  
  /*! Copy ids from source to target using the make shape class as a guide*/
  void modifiedMatch(BRepBuilderAPI_MakeShape&, const ResultContainer&, ResultContainer&);
  
  /*! when we have edges and vertices that can't be id, we map parent shapes to id.
   * Shape is in result container. Once a mapping is established the derived container,
   * will be check. if the dervied container has mapping the result container will be
   * updated. Else new ids will be generated and the derived container will be updated.
   */
  void derivedMatch(const TopoDS_Shape &, ResultContainer&, DerivedContainer&);
  
  /*! give some development feedback if container has nil entries.
   */
  void dumpNils(const ResultContainer &, const std::string &);
  
  /*! nil ids will eventually cause a crash. So this help keep a running program, but
   * will also hide errors from development. Best to call dumpNils before this, so
   * there is a sign of failed mappings.
   */
  void ensureNoNils(ResultContainer &);
  
  /*! when topology gets split we end up with duplicates in the result container.
   * this function scans the evolution container to find an entry for the duplicate. if
   * it finds it, update the resultContainer. if it doesn't find it creates an entry
   * for the evolution container and also updates the result container.
   */
  void updateSplits(ResultContainer &, EvolutionContainer&);
}

#endif // FTR_SHAPEIDMAPPER_H

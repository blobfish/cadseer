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

#ifndef SLC_CONTAINER_H
#define SLC_CONTAINER_H

#include <osg/ref_ptr>
#include <osg/Vec3d>

#include <tools/idtools.h>
#include <feature/types.h>
#include <selection/definitions.h>

namespace osg {class Geometry;}

namespace slc
{
  /*! @brief storing selections.
   * 
   * pointLocation is accurate for points, but not for edges and faces. @see Interpreter.
   */
  class Container
  {
  public:
      ~Container();
      Type selectionType = Type::None;
      ftr::Type featureType = ftr::Type::Base;
      boost::uuids::uuid featureId = gu::createNilId();
      boost::uuids::uuid shapeId = gu::createNilId();
      std::vector<boost::uuids::uuid> selectionIds; //!< objects to color. i.e. faces for a solid
      osg::ref_ptr<osg::Geometry> pointGeometry;
      osg::Vec3d pointLocation;
  };
  
  inline bool operator==(const Container& lhs, const Container& rhs)
  {
    bool out =
    (
      (lhs.selectionType == rhs.selectionType) &&
      (lhs.featureId == rhs.featureId) &&
      (lhs.shapeId == rhs.shapeId)
      //we don't consider the selection ids in comparison.
      //we don't consider the pointGeometry in comparison?
    );
      
    if (slc::isPointType(lhs.selectionType))
      out = (out && (lhs.pointLocation == rhs.pointLocation));
    
    return out;
  }
  inline bool operator!=(const Container& lhs, const Container& rhs)
  {
    return !(lhs == rhs);
  }
  std::ostream& operator<<(std::ostream& os, const Container& container);
  
  //allright. fuck it. I want a unique container that keeps insertion order.
  //set and unordered_set both can change order. Don't use vector
  //methods i.e. push_back etc..
  typedef std::vector<Container> Containers;
  std::ostream& operator<<(std::ostream& os, const Containers& containers);
  bool has(Containers &containersIn, const Container &containerIn);
  void add(Containers &containersIn, const Container &containerIn);
  void remove(Containers &containersIn, const Container &containerIn);
}

#endif // SLC_CONTAINER_H

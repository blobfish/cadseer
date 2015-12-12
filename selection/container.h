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

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/nil_generator.hpp>

#include <osg/observer_ptr>
#include <osg/ref_ptr>
#include <osg/Vec4>
#include <osg/Vec3d>

#include <selection/definitions.h>

namespace osg {class Geometry;}

namespace slc
{
  class Container
  {
  public:
      ~Container();
      Type selectionType = Type::None;
      boost::uuids::uuid featureId = boost::uuids::nil_generator()();
      boost::uuids::uuid shapeId = boost::uuids::nil_generator()();
      std::vector<boost::uuids::uuid> selectionIds; //!< objects to color. i.e. faces for a solid
      osg::ref_ptr<osg::Geometry> pointGeometry;
      osg::Vec3d pointLocation;
  };
  
  inline bool operator==(const Container& lhs, const Container& rhs)
  {
    return
    (
      (lhs.selectionType == rhs.selectionType) &&
      (lhs.featureId == rhs.featureId) &&
      (lhs.shapeId == rhs.shapeId) &&
      (lhs.pointLocation == rhs.pointLocation)
      //we don't consider the selection ids in comparison.
      //we don't consider the pointGeometry in comparison?
    );
  }
  inline bool operator!=(const Container& lhs, const Container& rhs)
  {
    return !(lhs == rhs);
  }
  std::ostream& operator<<(std::ostream& os, const Container& container);

  typedef std::vector<Container> Containers;
  std::ostream& operator<<(std::ostream& os, const Containers& containers);
}

#endif // SLC_CONTAINER_H

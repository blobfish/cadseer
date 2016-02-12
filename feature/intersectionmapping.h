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

#ifndef FTR_INTERSECTIONMAPPING_H
#define FTR_INTERSECTIONMAPPING_H

#include <set>
#include <vector>

#include <boost/uuid/uuid.hpp>

#include <TopAbs_Orientation.hxx>

class TopoDS_Shape;

namespace ftr
{
  /*! Edge created from the intersection
   * of 2 faces that results in one edge.
   */
  struct IntersectionEdge
  {
    IntersectionEdge();
    std::set<boost::uuids::uuid> faces; //!< faces from target and tools NOT resultant
    boost::uuids::uuid resultEdge; //!< result edge id.
    bool operator<(const IntersectionEdge &rhs) const;
  };
  std::ostream& operator<<(std::ostream &, const IntersectionEdge&);
  bool operator==(const IntersectionEdge &lhs, const IntersectionEdge &rhs);
  
  /*! Face created from the intersection split
   * update matching on edges to be very liberal
   * face need only to contain 1 edges for a match.
   */
  struct SplitFace
  {
    SplitFace();
    boost::uuids::uuid sourceFace; //!< source face, the one that was split.
    std::set<boost::uuids::uuid> edges; //!< intersection edges
    boost::uuids::uuid resultFace; //!< result face, an output of the split.
    boost::uuids::uuid resultWire; //!< result face, an output of the split.
    bool matchStrong(const SplitFace&) const;
    std::size_t matchWeak(const SplitFace&) const; //!< number of intersection edges shared.
  };
  std::ostream& operator<<(std::ostream &, const SplitFace&);
  
  struct IMapWrapper
  {
    std::set<IntersectionEdge> intersectionEdges;
    std::vector<SplitFace> splitFaces;
    void add(const SplitFace &);
    std::pair<SplitFace, bool> matchStrong(const SplitFace &) const; //!< returns exact match and bool indicating a match.
    std::vector<SplitFace> matchWeak(const SplitFace &) const; //!< find all split faces that have at lease one matching iedge.
  };
}

#endif // FTR_INTERSECTIONMAPPING_H

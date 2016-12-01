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

#include <algorithm>

#include <TopExp.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>

#include <tools/idtools.h>
#include "intersectionmapping.h"

using namespace ftr;

IntersectionEdge::IntersectionEdge()
{
  resultEdge = gu::createNilId();
}

bool IntersectionEdge::operator<(const IntersectionEdge &rhs) const
{
  return
  (
    (faces < rhs.faces)
  );
}

bool operator==(const IntersectionEdge &lhs, const IntersectionEdge &rhs)
{
  return
  (
    (lhs.faces == rhs.faces)
  );
}

std::ostream & ftr::operator<<(std::ostream &stream, const IntersectionEdge &iEdgeIn)
{
  stream << "faces: " << std::endl;
  for (const auto &faceId : iEdgeIn.faces)
    std::cout << "    " << gu::idToString(faceId) << std::endl;
  std::cout << "result edge: " << gu::idToString(iEdgeIn.resultEdge) << std::endl;
    
  return stream;
}

std::ostream & ftr::operator<<(std::ostream &stream, const SplitFace &splitIn)
{
  stream << "source face: " << gu::idToString(splitIn.sourceFace) << std::endl
  << "edges:" << std::endl;
  for (const auto &entry : splitIn.edges)
    stream << "    " << gu::idToString(entry) << std::endl;
  stream << "result face: " << gu::idToString(splitIn.resultFace) << std::endl
  << "result wire: " << gu::idToString(splitIn.resultWire) << std::endl;
  
  return stream;
}

SplitFace::SplitFace()
{
  sourceFace = gu::createNilId();
  resultFace = gu::createNilId();
  resultWire = gu::createNilId();
}

bool SplitFace::matchStrong(const SplitFace &other) const
{
  return edges == other.edges;
}

std::size_t SplitFace::matchWeak(const SplitFace &other) const
{
  //match is true if share at least one intersection edge.
  std::set<boost::uuids::uuid> idIntersection;
  std::set_intersection(other.edges.begin(), other.edges.end(),
    edges.begin(), edges.end(),  std::inserter(idIntersection, idIntersection.begin()));
  
  return idIntersection.size();
}

void IMapWrapper::add(const SplitFace &splitFaceIn)
{
  splitFaces.push_back(splitFaceIn);
}

std::pair<SplitFace, bool> IMapWrapper::matchStrong(const SplitFace &splitFaceIn) const
{
  //just return first hit?
  for (const auto &sFace : splitFaces)
  {
    if (sFace.matchStrong(splitFaceIn))
      return std::make_pair(sFace, true);
  }
  return std::make_pair(SplitFace(), false);
}

std::vector< SplitFace > IMapWrapper::matchWeak(const SplitFace &splitFaceIn) const
{
  typedef std::pair<int, SplitFace> SplitPair;
  auto compare = [](const SplitPair &lhs, const SplitPair &rhs) -> bool
  {
    return lhs.first > rhs.first; //descending
  };
  auto splitPairs = std::set<SplitPair, decltype(compare)> (compare);
  
  for (const auto &sFace : splitFaces)
  {
    std::size_t matchCount = sFace.matchWeak(splitFaceIn);
    if (matchCount > 0)
      splitPairs.insert(std::make_pair(matchCount, sFace));
  }
  
  std::vector <SplitFace> out;
  for (const auto &pair : splitPairs)
    out.push_back(pair.second);
  
  return out;
}


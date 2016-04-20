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

#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <project/serial/xsdcxxoutput/featurebooleanbase.h>
#include "booleanbase.h"

using namespace ftr;

BooleanBase::BooleanBase(): Base()
{

}

prj::srl::FeatureBooleanBase BooleanBase::serialOut()
{
  using boost::uuids::to_string;
  
  prj::srl::FeatureBase fBase = Base::serialOut();
  
  prj::srl::IntersectionEdges edgesOut;
  for (const auto &edge : iMapWrapper.intersectionEdges)
  {
    prj::srl::IntersectionEdge::FaceIdsType faceIds;
    for (const auto fId : edge.faces)
      faceIds.id().push_back(to_string(fId));
    edgesOut.intersectionEdges().push_back(prj::srl::IntersectionEdge(faceIds, to_string(edge.resultEdge)));
  }

  prj::srl::SplitFaces facesOut;
  for (const auto &splitFace : iMapWrapper.splitFaces)
  {
    prj::srl::SplitFace::IntersectionEdgeIdsType edgeIds;
    for (const auto eId : splitFace.edges)
      edgeIds.id().push_back(to_string(eId));
    facesOut.splitFaces().push_back
    (
      prj::srl::SplitFace(to_string(splitFace.sourceFace),
	edgeIds, to_string(splitFace.resultFace), to_string(splitFace.resultWire))
    );
  }
  
  prj::srl::IMapWrapper iMapWrapperOut(edgesOut, facesOut);
  return prj::srl::FeatureBooleanBase(fBase, iMapWrapperOut);
}

void BooleanBase::serialIn(const prj::srl::FeatureBooleanBase& sBooleanBaseIn)
{
  boost::uuids::string_generator sg;
  
  Base::serialIn(sBooleanBaseIn.featureBase());
  
  iMapWrapper.intersectionEdges.clear();
  for (const auto &sIEdge : sBooleanBaseIn.iMapWrapper().intersectionEdges().intersectionEdges())
  {
    IntersectionEdge edgeIn;
    for (const auto &idIn : sIEdge.faceIds().id())
      edgeIn.faces.insert(sg(idIn));
    edgeIn.resultEdge = sg(sIEdge.edgeId());
    iMapWrapper.intersectionEdges.insert(edgeIn);
  }
  
  iMapWrapper.splitFaces.clear();
  for (const auto &sSFace : sBooleanBaseIn.iMapWrapper().splitFaces().splitFaces())
  {
    SplitFace faceIn;
    faceIn.sourceFace = sg(sSFace.sourceFaceId());
    for (const auto &idIn : sSFace.intersectionEdgeIds().id())
      faceIn.edges.insert(sg(idIn));
    faceIn.resultFace = sg(sSFace.resultFaceId());
    faceIn.resultWire = sg(sSFace.resultWireId());
    iMapWrapper.splitFaces.push_back(faceIn);
  }
}


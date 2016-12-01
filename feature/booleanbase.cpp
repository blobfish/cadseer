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

#include <tools/idtools.h>
#include <project/serial/xsdcxxoutput/featurebooleanbase.h>
#include "booleanbase.h"

using namespace ftr;

BooleanBase::BooleanBase(): Base()
{

}

prj::srl::FeatureBooleanBase BooleanBase::serialOut()
{
  prj::srl::FeatureBase fBase = Base::serialOut();
  
  prj::srl::IntersectionEdges edgesOut;
  for (const auto &edge : iMapWrapper.intersectionEdges)
  {
    prj::srl::IntersectionEdge::FaceIdsType faceIds;
    for (const auto fId : edge.faces)
      faceIds.id().push_back(gu::idToString(fId));
    edgesOut.intersectionEdges().push_back(prj::srl::IntersectionEdge(faceIds, gu::idToString(edge.resultEdge)));
  }

  prj::srl::SplitFaces facesOut;
  for (const auto &splitFace : iMapWrapper.splitFaces)
  {
    prj::srl::SplitFace::IntersectionEdgeIdsType edgeIds;
    for (const auto eId : splitFace.edges)
      edgeIds.id().push_back(gu::idToString(eId));
    facesOut.splitFaces().push_back
    (
      prj::srl::SplitFace(gu::idToString(splitFace.sourceFace),
                          edgeIds, gu::idToString(splitFace.resultFace), gu::idToString(splitFace.resultWire))
    );
  }
  
  prj::srl::IMapWrapper iMapWrapperOut(edgesOut, facesOut);
  return prj::srl::FeatureBooleanBase(fBase, iMapWrapperOut);
}

void BooleanBase::serialIn(const prj::srl::FeatureBooleanBase& sBooleanBaseIn)
{
  Base::serialIn(sBooleanBaseIn.featureBase());
  
  iMapWrapper.intersectionEdges.clear();
  for (const auto &sIEdge : sBooleanBaseIn.iMapWrapper().intersectionEdges().intersectionEdges())
  {
    IntersectionEdge edgeIn;
    for (const auto &idIn : sIEdge.faceIds().id())
      edgeIn.faces.insert(gu::stringToId(idIn));
    edgeIn.resultEdge = gu::stringToId(sIEdge.edgeId());
    iMapWrapper.intersectionEdges.insert(edgeIn);
  }
  
  iMapWrapper.splitFaces.clear();
  for (const auto &sSFace : sBooleanBaseIn.iMapWrapper().splitFaces().splitFaces())
  {
    SplitFace faceIn;
    faceIn.sourceFace = gu::stringToId(sSFace.sourceFaceId());
    for (const auto &idIn : sSFace.intersectionEdgeIds().id())
      faceIn.edges.insert(gu::stringToId(idIn));
    faceIn.resultFace = gu::stringToId(sSFace.resultFaceId());
    faceIn.resultWire = gu::stringToId(sSFace.resultWireId());
    iMapWrapper.splitFaces.push_back(faceIn);
  }
}


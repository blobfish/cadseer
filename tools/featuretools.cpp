/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2018  Thomas S. Anderson blobfish.at.gmx.com
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

#include <annex/seershape.h>
#include <feature/shapehistory.h>
#include <feature/pick.h>
#include <feature/base.h>
#include <tools/featuretools.h>

std::vector <std::pair<const ftr::Base*, boost::uuids::uuid>>
tls::resolvePicks
(
  const std::vector<const ftr::Base*> &features,
  std::vector<ftr::Pick> picks, //make a work copy of the picks.
  const ftr::ShapeHistory &pHistory
)
{
  std::vector <std::pair<const ftr::Base*, boost::uuids::uuid>> out;
  
  for (const auto *tf : features)
  {
    //i can see wrong resolution happening here. the input graph edges don't store the shapeid and the picks
    //don't store the feature id. When shape history has splits and joins and we have multiple tools with picks,
    //it will be easy to resolve to a different shape then intended by user. Not sure what to do.
    assert(tf->hasAnnex(ann::Type::SeerShape)); //caller verifies.
    if (!tf->hasAnnex(ann::Type::SeerShape))
      continue;
    const ann::SeerShape &toolSeerShape = tf->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
    assert(!toolSeerShape.isNull()); //caller verifies.
    if (toolSeerShape.isNull())
      continue;
    
    bool foundPick = false;
    for (auto it = picks.begin(); it != picks.end();)
    {
      std::vector<boost::uuids::uuid> rids = pHistory.resolveHistories(it->shapeHistory, tf->getId());
      if (rids.empty())
      {
        ++it;
        continue;
      }
      foundPick = true;
      it = picks.erase(it);
      for (const auto rid : rids)
      {
        assert(toolSeerShape.hasShapeIdRecord(rid)); //project shape history and the feature shapeid records out of sync.
        if (!toolSeerShape.hasShapeIdRecord(rid))
          continue;
        out.push_back(std::make_pair(tf, rid));
      }
    }
    if (!foundPick) //we assume the whole shape.
      out.push_back(std::make_pair(tf, gu::createNilId()));
  }
  gu::uniquefy(out);
  
  return out;
}

std::vector<std::pair<const ftr::Base*, boost::uuids::uuid>>
tls::resolvePicks
(
  const ftr::Base *feature,
  const ftr::Pick &pick,
  const ftr::ShapeHistory &pHistory
)
{
  std::vector<const ftr::Base*> features(1, feature);
  ftr::Picks picks(1, pick);
  return tls::resolvePicks(features, picks, pHistory);
}

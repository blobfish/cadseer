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

#include <BRepAlgoAPI_Common.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

#include <tools/featuretools.h>
#include <feature/booleanoperation.h>
#include <project/serial/xsdcxxoutput/featureintersect.h>
#include <feature/shapecheck.h>
#include <annex/seershape.h>
#include <annex/intersectionmapper.h>
#include <feature/updatepayload.h>
#include <feature/intersect.h>

using boost::uuids::uuid;

using namespace ftr;

QIcon Intersect::icon;

Intersect::Intersect() : Base(), sShape(new ann::SeerShape()), iMapper(new ann::IntersectionMapper())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionIntersect.svg");
  
  name = QObject::tr("Intersect");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
  annexes.insert(std::make_pair(ann::Type::IntersectionMapper, iMapper.get()));
}

Intersect::~Intersect(){}

void Intersect::setTargetPicks(const Picks &psIn)
{
  if (targetPicks == psIn)
    return;
  targetPicks = psIn;
  setModelDirty();
}

void Intersect::setToolPicks(const Picks &psIn)
{
  if (toolPicks == psIn)
    return;
  toolPicks = psIn;
  setModelDirty();
}

void Intersect::updateModel(const UpdatePayload &payloadIn)
{
  setFailure(); //assume failure until success.
  lastUpdateLog.clear();
  try
  {
    //target
    std::vector<const Base*> targetFeatures = payloadIn.getFeatures(InputType::target);
    if (targetFeatures.empty())
      throw std::runtime_error("no target features");
    for (const Base* f : targetFeatures)
    {
      if (!f->hasAnnex(ann::Type::SeerShape))
        throw std::runtime_error("target feature doesn't have seer shape");
      const ann::SeerShape &targetSeerShape = f->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
      assert(!targetSeerShape.isNull());
      if (targetSeerShape.isNull())
        throw std::runtime_error("target seerShape is null");
    }
    auto targetPairs = tls::resolvePicks(targetFeatures, targetPicks, payloadIn.shapeHistory);
    assert(!targetPairs.empty());
    if (targetPairs.empty())
      throw std::runtime_error("no target pairs");
    occt::ShapeVector targetOCCTShapes;
    for (const auto &pair : targetPairs)
    {
      const ann::SeerShape &tShape = pair.first->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
      if (pair.second.is_nil())
      {
        //don't include the compound.
        const auto &c = tShape.useGetNonCompoundChildren();
        std::copy(c.begin(), c.end(), std::back_inserter(targetOCCTShapes));
      }
      else
        targetOCCTShapes.push_back(tShape.getOCCTShape(pair.second));
    }
    occt::uniquefy(targetOCCTShapes); //just in case.
    for (auto it = targetOCCTShapes.begin(); it != targetOCCTShapes.end();)
    {
      //remove anything below a solid.
      if (it->ShapeType() > TopAbs_SOLID)
        it = targetOCCTShapes.erase(it);
      else
        ++it;
    }
    
    //tools
    std::vector<const Base*> toolFeatures = payloadIn.getFeatures(InputType::tool);
    if (toolFeatures.empty())
      throw std::runtime_error("no tool features found");
    for (const Base* tf : toolFeatures) //do some verfication.
    {
      assert(tf->hasAnnex(ann::Type::SeerShape)); //make user interface verify the input.
      if (!tf->hasAnnex(ann::Type::SeerShape))
        throw std::runtime_error("tool feature has no seer shape");
      const ann::SeerShape &toolSeerShape = tf->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
      assert(!toolSeerShape.isNull());
      if (toolSeerShape.isNull())
        throw std::runtime_error("tool seerShape is null");
    }
    auto pairs = tls::resolvePicks(toolFeatures, toolPicks, payloadIn.shapeHistory);
    occt::ShapeVector toolOCCTShapes;
    for (const auto &pair : pairs)
    {
      const ann::SeerShape &tShape = pair.first->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
      if (pair.second.is_nil())
      {
        //don't include the compound.
        const auto &c = tShape.useGetNonCompoundChildren();
        std::copy(c.begin(), c.end(), std::back_inserter(toolOCCTShapes));
      }
      else
        toolOCCTShapes.push_back(tShape.getOCCTShape(pair.second));
    }
    occt::uniquefy(toolOCCTShapes); //just in case.
    for (auto it = toolOCCTShapes.begin(); it != toolOCCTShapes.end();)
    {
      //remove anything below a solid.
      if (it->ShapeType() > TopAbs_SOLID)
        it = toolOCCTShapes.erase(it);
      else
        ++it;
    }
    
    BooleanOperation intersector(targetOCCTShapes, toolOCCTShapes, BOPAlgo_COMMON);
    intersector.Build();
    if (!intersector.IsDone())
      throw std::runtime_error("OCC intersect failed");
    ShapeCheck check(intersector.Shape());
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    sShape->setOCCTShape(intersector.Shape());
    
    iMapper->go(payloadIn, intersector.getBuilder(), *sShape);
    
    for (const auto *it : targetFeatures)
      sShape->shapeMatch(it->getAnnex<ann::SeerShape>(ann::Type::SeerShape));
    for (const auto *it : toolFeatures)
      sShape->shapeMatch(it->getAnnex<ann::SeerShape>(ann::Type::SeerShape));
    for (const auto *it : targetFeatures)
      sShape->uniqueTypeMatch(it->getAnnex<ann::SeerShape>(ann::Type::SeerShape));
    for (const auto *it : targetFeatures)
      sShape->outerWireMatch(it->getAnnex<ann::SeerShape>(ann::Type::SeerShape));
    for (const auto *it : toolFeatures)
      sShape->outerWireMatch(it->getAnnex<ann::SeerShape>(ann::Type::SeerShape));
    sShape->derivedMatch();
    sShape->dumpNils("intersect feature"); //only if there are shapes with nil ids.
    sShape->dumpDuplicates("intersect feature");
    sShape->ensureNoNils();
    sShape->ensureNoDuplicates();
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in intersect update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in intersect update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in intersect update." << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Intersect::serialWrite(const QDir &dIn)
{
  prj::srl::FeatureIntersect intersectOut
  (
    Base::serialOut(),
    iMapper->serialOut(),
    ftr::serialOut(targetPicks),
    ftr::serialOut(toolPicks)
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::intersect(stream, intersectOut, infoMap);
}

void Intersect::serialRead(const prj::srl::FeatureIntersect& sIntersectIn)
{
  Base::serialIn(sIntersectIn.featureBase());
  iMapper->serialIn(sIntersectIn.intersectionMapper());
  targetPicks = ftr::serialIn(sIntersectIn.targetPicks());
  toolPicks = ftr::serialIn(sIntersectIn.toolPicks());
}

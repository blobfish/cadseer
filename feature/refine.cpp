/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#include <ShapeUpgrade_UnifySameDomain.hxx>
#include <BRepTools_History.hxx>

#include <project/serial/xsdcxxoutput/featurerefine.h>
#include <annex/seershape.h>
#include <feature/refine.h>

using namespace ftr;

using boost::uuids::uuid;

QIcon Refine::icon;

Refine::Refine() : Base(), sShape(new ann::SeerShape())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionRefine.svg");
  
  name = QObject::tr("Refine");
  mainSwitch->setUserValue(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
}

Refine::~Refine(){}

void Refine::updateModel(const UpdatePayload &payloadIn)
{
  setFailure();
  lastUpdateLog.clear();
  try
  {
    if (payloadIn.updateMap.count(ftr::InputType::target) != 1)
      throw std::runtime_error("couldn't find 'target' input");
    const ftr::Base *pbf = payloadIn.updateMap.equal_range(ftr::InputType::target).first->second;
    if(!pbf->hasAnnex(ann::Type::SeerShape))
      throw std::runtime_error("no seer shape for target");
    const ann::SeerShape &tss = pbf->getAnnex<ann::SeerShape>(ann::Type::SeerShape); //target seer shape.
    const TopoDS_Shape &shape = tss.getRootOCCTShape();
    
    ShapeUpgrade_UnifySameDomain usd(shape);
    usd.Build();
    sShape->setOCCTShape(usd.Shape());
    sShape->shapeMatch(tss);
    sShape->uniqueTypeMatch(tss);
    
    historyMatch(*(usd.History()), tss);
    
    sShape->outerWireMatch(tss);
    sShape->derivedMatch();
    sShape->dumpNils("refine feature");
    sShape->dumpDuplicates("refine feature");
    sShape->ensureNoNils();
    sShape->ensureNoDuplicates();
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in refine update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in refine update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in refine update. " << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Refine::historyMatch(const BRepTools_History &hIn , const ann::SeerShape &tIn)
{
  std::vector<uuid> tIds = tIn.getAllShapeIds();
  
  for (const auto &ctId : tIds) //cuurent target id
  {
    const TopoDS_Shape &cs = tIn.findShapeIdRecord(ctId).shape;
    
    //not getting anything from generated
    //modified appears to be a one to one mapping of faces and edges
    
    const TopTools_ListOfShape &modified = hIn.Modified(cs);
    for (const auto &ms : modified)
    {
      assert(sShape->hasShapeIdRecord(ms));
      if (sShape->findShapeIdRecord(ms).id.is_nil())
      {
        std::map<uuid, uuid>::iterator mapItFace;
        bool dummy;
        std::tie(mapItFace, dummy) = shapeMap.insert(std::make_pair(ctId, gu::createRandomId()));
        sShape->updateShapeIdRecord(ms, mapItFace->second);
      }
      sShape->insertEvolve(ctId, sShape->findShapeIdRecord(ms).id);
    }
    if (hIn.IsRemoved(cs)) //appears to remove only edges and vertices.
      sShape->insertEvolve(ctId, gu::createNilId());
  }
}

void Refine::serialWrite(const QDir &dIn)
{
  prj::srl::FeatureRefine::ShapeMapType shapeMapOut;
  for (const auto &p : shapeMap)
  {
    prj::srl::EvolveRecord eRecord
    (
      gu::idToString(p.first),
      gu::idToString(p.second)
    );
    shapeMapOut.evolveRecord().push_back(eRecord);
  }
  
  prj::srl::FeatureRefine ro
  (
    Base::serialOut(),
    shapeMapOut
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::refine(stream, ro, infoMap);
}

void Refine::serialRead(const prj::srl::FeatureRefine &rfIn)
{
  Base::serialIn(rfIn.featureBase());
  
  shapeMap.clear();
  for (const prj::srl::EvolveRecord &sERecord : rfIn.shapeMap().evolveRecord())
  {
    std::pair<uuid, uuid> record;
    record.first = gu::stringToId(sERecord.idIn());
    record.second = gu::stringToId(sERecord.idOut());
    shapeMap.insert(record);
  }
}

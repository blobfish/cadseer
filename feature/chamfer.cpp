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

#include <BRepFilletAPI_MakeChamfer.hxx>
#include <TopExp.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <Geom_Curve.hxx>

#include <globalutilities.h>
#include <tools/idtools.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <project/serial/xsdcxxoutput/featurechamfer.h>
#include <feature/seershape.h>
#include <feature/shapecheck.h>
#include <feature/chamfer.h>

using namespace ftr;
using boost::uuids::uuid;

QIcon Chamfer::icon;

boost::uuids::uuid Chamfer::referenceFaceId(const SeerShape &seerShapeIn, const boost::uuids::uuid &edgeIdIn)
{
  assert(seerShapeIn.hasShapeIdRecord(edgeIdIn));
  TopoDS_Shape edgeShape = seerShapeIn.findShapeIdRecord(edgeIdIn).shape;
  assert(edgeShape.ShapeType() == TopAbs_EDGE);
  
  std::vector<boost::uuids::uuid> faceIds = seerShapeIn.useGetParentsOfType(edgeIdIn, TopAbs_FACE);
  assert(!faceIds.empty());
  return faceIds.front();
}

std::shared_ptr< Parameter > Chamfer::buildSymParameter()
{
  std::shared_ptr<Parameter> out(new Parameter(ParameterNames::Distance, prf::manager().rootPtr->features().chamfer().get().distance()));
  out->setConstraint(ParameterConstraint::buildNonZeroPositive());
  return out;
}

Chamfer::Chamfer() : Base()
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionChamfer.svg");
  
  name = QObject::tr("Chamfer");
  mainSwitch->setUserValue(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
}

void Chamfer::addSymChamfer(const SymChamfer &chamferIn)
{
  symChamfers.push_back(chamferIn);
  
  if (!symChamfers.back().distance)
    symChamfers.back().distance = buildSymParameter();
  symChamfers.back().distance->connectValue(boost::bind(&Chamfer::setModelDirty, this));
  
  if (!symChamfers.back().label)
    symChamfers.back().label = new lbr::PLabel(symChamfers.back().distance.get());
  symChamfers.back().label->valueHasChanged();
  overlaySwitch->addChild(symChamfers.back().label.get());
}

void Chamfer::updateModel(const UpdatePayload &payloadIn)
{
  setFailure();
  try
  {
    if (payloadIn.updateMap.count(InputType::target) != 1)
      throw std::runtime_error("no parent for chamfer");
    
    const SeerShape &targetSeerShape = payloadIn.updateMap.equal_range(InputType::target).first->second->getSeerShape();
    
    BRepFilletAPI_MakeChamfer chamferMaker(targetSeerShape.getRootOCCTShape());
    for (const auto &chamfer : symChamfers)
    {
      assert(chamfer.distance);
      bool labelDone = false; //set label position to first pick.
      for (const auto &pick : chamfer.picks)
      {
        std::vector<uuid> resolvedEdgeIds = targetSeerShape.resolvePick(pick.edgePick.shapeHistory);
        if (resolvedEdgeIds.empty())
        {
          std::cout << "Chamfer: can't find target edge id. Skipping id: " << gu::idToString(pick.edgePick.id) << std::endl;
          continue;
        }
        assert(resolvedEdgeIds.size() == 1); //want to examine this condition.
        
        std::vector<uuid> resolvedFaceIds = targetSeerShape.resolvePick(pick.facePick.shapeHistory);
        if (resolvedFaceIds.empty())
        {
          std::cout << "Chamfer: can't find target face id. Skipping id: " << gu::idToString(pick.facePick.id) << std::endl;
          continue;
        }
        assert(resolvedFaceIds.size() == 1); //want to examine this condition.
        
        updateShapeMap(resolvedEdgeIds.front(), pick.edgePick.shapeHistory);
        updateShapeMap(resolvedFaceIds.front(), pick.facePick.shapeHistory);
        TopoDS_Edge edge = TopoDS::Edge(targetSeerShape.findShapeIdRecord(resolvedEdgeIds.front()).shape);
        TopoDS_Face face = TopoDS::Face(targetSeerShape.findShapeIdRecord(resolvedFaceIds.front()).shape);
        chamferMaker.Add(chamfer.distance->getValue(), edge, face);
        //update location of parameter label.
        if (!labelDone)
        {
          labelDone = true;
          chamfer.label->setMatrix(osg::Matrixd::translate(pick.edgePick.getPoint(TopoDS::Edge(edge))));
        }
      }
    }
    chamferMaker.Build();
    if (!chamferMaker.IsDone())
      throw std::runtime_error("chamferMaker failed");
    ShapeCheck check(chamferMaker.Shape());
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");

    seerShape->setOCCTShape(chamferMaker.Shape());
    seerShape->shapeMatch(targetSeerShape);
    seerShape->uniqueTypeMatch(targetSeerShape);
    seerShape->modifiedMatch(chamferMaker, targetSeerShape);
    generatedMatch(chamferMaker, targetSeerShape);
    seerShape->outerWireMatch(targetSeerShape);
    seerShape->derivedMatch();
    seerShape->dumpNils("chamfer feature"); //only if there are shapes with nil ids.
    seerShape->dumpDuplicates("chamfer feature");
    seerShape->ensureNoNils();
    seerShape->ensureNoDuplicates();
    
    setSuccess();
  }
  catch (Standard_Failure)
  {
    Handle_Standard_Failure e = Standard_Failure::Caught();
    std::cout << std::endl << "Error in chamfer update. " << e->GetMessageString() << std::endl;
  }
  catch (std::exception &e)
  {
    std::cout << std::endl << "Error in chamfer update. " << e.what() << std::endl;
  }
  setModelClean();
}

//duplicated with blend.
void Chamfer::generatedMatch(BRepFilletAPI_MakeChamfer &chamferMakerIn, const SeerShape &targetShapeIn)
{
  using boost::uuids::uuid;
  
  std::vector<uuid> targetShapeIds = targetShapeIn.getAllShapeIds();
  
  for (const auto &cId : targetShapeIds)
  {
    const TopoDS_Shape &currentShape = targetShapeIn.findShapeIdRecord(cId).shape;
    TopTools_ListOfShape generated = chamferMakerIn.Generated(currentShape);
    if (generated.IsEmpty())
      continue;
    if(generated.Extent() != 1)
      std::cout << "Warning: more than one generated shape in chamfer::generatedMatch" << std::endl;
    const TopoDS_Shape &chamferFace = generated.First();
    assert(!chamferFace.IsNull());
    assert(chamferFace.ShapeType() == TopAbs_FACE);
    
    std::map<uuid, uuid>::iterator mapItFace;
    bool dummy;
    std::tie(mapItFace, dummy) = shapeMap.insert(std::make_pair(cId, gu::createRandomId()));
    seerShape->updateShapeIdRecord(chamferFace, mapItFace->second);
    if (dummy) //insertion took place.
      seerShape->insertEvolve(gu::createNilId(), mapItFace->second);
    
    //now look for outerwire for newly generated face.
    //we use the generated face id to map to outer wire.
    std::map<uuid, uuid>::iterator mapItWire;
    std::tie(mapItWire, dummy) = shapeMap.insert(std::make_pair(mapItFace->second, gu::createRandomId()));
    //now get the wire and update the result to id.
    const TopoDS_Shape &chamferedFaceWire = BRepTools::OuterWire(TopoDS::Face(chamferFace));
    seerShape->updateShapeIdRecord(chamferedFaceWire, mapItWire->second);
    if (dummy) //insertion took place.
      seerShape->insertEvolve(gu::createNilId(), mapItWire->second);
  }
}

void Chamfer::updateShapeMap(const boost::uuids::uuid &resolvedId, const ShapeHistory &pick)
{
  for (const auto &historyId : pick.getAllIds())
  {
    assert(shapeMap.count(historyId) < 2);
    auto it = shapeMap.find(historyId);
    if (it == shapeMap.end())
      continue;
    auto entry = std::make_pair(resolvedId, it->second);
    shapeMap.erase(it);
    shapeMap.insert(entry);
  }
}

void Chamfer::serialWrite(const QDir &dIn)
{
  prj::srl::FeatureChamfer::ShapeMapType shapeMapOut;
  for (const auto &p : shapeMap)
  {
    prj::srl::EvolveRecord eRecord
    (
      gu::idToString(p.first),
      gu::idToString(p.second)
    );
    shapeMapOut.evolveRecord().push_back(eRecord);
  }
  
  prj::srl::SymChamfers sSymChamfersOut;
  for (const auto &sSymChamfer : symChamfers)
  {
    prj::srl::ChamferPicks cPicksOut;
    for (const auto &cPick : sSymChamfer.picks)
    {
      prj::srl::ChamferPick cPickOut
      (
        cPick.edgePick.serialOut(),
        cPick.facePick.serialOut()
      );
      cPicksOut.array().push_back(cPickOut);
    }
    prj::srl::SymChamfer sChamferOut
    (
      cPicksOut,
      sSymChamfer.distance->serialOut()
    );
    sSymChamfersOut.array().push_back(sChamferOut);
  }
  
  prj::srl::FeatureChamfer chamferOut
  (
    Base::serialOut(),
    shapeMapOut,
    sSymChamfersOut
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::chamfer(stream, chamferOut, infoMap);
}

void Chamfer::serialRead(const prj::srl::FeatureChamfer &sChamferIn)
{
  Base::serialIn(sChamferIn.featureBase());
  
  shapeMap.clear();
  for (const prj::srl::EvolveRecord &sERecord : sChamferIn.shapeMap().evolveRecord())
  {
    std::pair<uuid, uuid> record;
    record.first = gu::stringToId(sERecord.idIn());
    record.second = gu::stringToId(sERecord.idOut());
    shapeMap.insert(record);
  }
  
  for (const auto &symChamferIn : sChamferIn.symChamfers().array())
  {
    SymChamfer symChamfer;
    for (const auto &cPickIn : symChamferIn.chamferPicks().array())
    {
      ChamferPick pick;
      pick.edgePick.serialIn(cPickIn.edgePick());
      pick.facePick.serialIn(cPickIn.facePick());
      symChamfer.picks.push_back(pick);
    }
    symChamfer.distance = buildSymParameter();
    symChamfer.distance->serialIn(symChamferIn.distance());
    addSymChamfer(symChamfer);
  }
}

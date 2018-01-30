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

#include <BRepOffsetAPI_MakeThickSolid.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRep_Tool.hxx>
#include <BRepTools.hxx>
#include <Geom_Surface.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>

#include <globalutilities.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <annex/seershape.h>
#include <feature/shapecheck.h>
#include <library/plabel.h>
#include <project/serial/xsdcxxoutput/featurehollow.h>
#include <tools/featuretools.h>
#include <feature/updatepayload.h>
#include <feature/parameter.h>
#include <feature/hollow.h>

using namespace ftr;

using boost::uuids::uuid;

QIcon Hollow::icon;

Hollow::Hollow() :
Base(),
offset(new prm::Parameter(prm::Names::Offset, prf::manager().rootPtr->features().hollow().get().offset())),
sShape(new ann::SeerShape())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionHollow.svg");
  
  name = QObject::tr("Hollow");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  offset->setConstraint(prm::Constraint::buildNonZero());
  offset->connectValue(boost::bind(&Hollow::setModelDirty, this));
  
  label = new lbr::PLabel(offset.get());
  label->valueHasChanged();
  overlaySwitch->addChild(label.get());
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
}

Hollow::~Hollow(){}

/* as occt v7.1(actually commit 4d97335) there seems to be a tolerance bug
 * in BRepOffsetAPI_MakeThickSolid. to reproduce the bug in drawexe:
 * 
 * pload MODELING
 * box aBox 20 10 6
 * explode aBox f
 * offsetshape result aBox 1.0 aBox_1
 * maxtolerance aBox
 * maxtolerance result
 * 
 * for both 'aBox' and 'result' the max tolerances for both edge and vertex
 * are 1.00000e-03. way too loose. going to let one point release go by
 * and check these results again before trying to get a bug report.
 * 
 * will this work with no removal faces?
 */

void Hollow::updateModel(const UpdatePayload &payloadIn)
{
  setFailure();
  lastUpdateLog.clear();
  sShape->reset();
  try
  {
    std::vector<const Base*> tfs = payloadIn.getFeatures(InputType::target);
    if (tfs.size() != 1)
      throw std::runtime_error("no parent for hollow");
    if (!tfs.front()->hasAnnex(ann::Type::SeerShape))
      throw std::runtime_error("parent doesn't have seer shape");
    const ann::SeerShape &tss= tfs.front()->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
    if (tss.isNull())
      throw std::runtime_error("target seer shape is null");
    
    //setup new failure state.
    sShape->setOCCTShape(tss.getRootOCCTShape());
    sShape->shapeMatch(tss);
    sShape->uniqueTypeMatch(tss);
    sShape->outerWireMatch(tss);
    sShape->derivedMatch();
    sShape->ensureNoNils(); //just in case
    sShape->ensureNoDuplicates(); //just in case
    
    bool labelSet = false;
    occt::ShapeVector closingFaceShapes;
    std::vector<uuid> solidIds;
    auto resolvedPicks = tls::resolvePicks(tfs, hollowPicks, payloadIn.shapeHistory);
    for (const auto &resolved : resolvedPicks)
    {
      if (resolved.resultId.is_nil())
        continue;
      assert(tss.hasShapeIdRecord(resolved.resultId));
      if (!tss.hasShapeIdRecord(resolved.resultId))
        continue;
      TopoDS_Shape face = tss.findShapeIdRecord(resolved.resultId).shape;
      if (face.ShapeType() != TopAbs_FACE)
        continue;
      closingFaceShapes.push_back(face);
      std::vector<uuid> sp = tss.useGetParentsOfType(resolved.resultId, TopAbs_SOLID);
      std::copy(sp.begin(), sp.end(), std::back_inserter(solidIds));
      if (!labelSet)
      {
        labelSet = true;
        label->setMatrix(osg::Matrixd::translate(resolved.pick.getPoint(TopoDS::Face(face))));
      }
    }
    if (closingFaceShapes.empty())
      throw std::runtime_error("no closing faces");
    gu::uniquefy(solidIds);
    if (solidIds.size() != 1)
      throw std::runtime_error("Only works with 1 solid");
    
    BRepOffsetAPI_MakeThickSolid operation;
    operation.MakeThickSolidByJoin
    (
      tss.getRootOCCTShape(),
      occt::ShapeVectorCast(closingFaceShapes),
      -static_cast<double>(*offset), //default direction sucks.
      Precision::Confusion(),
      BRepOffset_Skin,
      Standard_False,
      Standard_False,
      GeomAbs_Intersection,
      Standard_True
    );
    operation.Check();
    
    ShapeCheck check(operation.Shape());
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed in hollow feature");
    
    sShape->setOCCTShape(operation.Shape());
    sShape->shapeMatch(tss);
    sShape->uniqueTypeMatch(tss);
    sShape->modifiedMatch(operation, tss);
    generatedMatch(operation, tss);
    sShape->outerWireMatch(tss);
    sShape->derivedMatch();
    sShape->dumpNils("hollow feature");
    sShape->dumpDuplicates("hollow feature");
    sShape->ensureNoNils();
    sShape->ensureNoDuplicates();
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in hollow update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in hollow update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in hollow update." << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Hollow::setHollowPicks(const Picks &picksIn)
{
  hollowPicks = picksIn;
}

void Hollow::addHollowPick(const Pick &pickIn)
{
  hollowPicks.push_back(pickIn);
}

void Hollow::removeHollowPick(const Pick &pickIn)
{
  auto it = std::find(hollowPicks.begin(), hollowPicks.end(), pickIn);
  if (it == hollowPicks.end())
  {
    std::ostringstream s; s << "warning: hollow: trying to remove a pick not in closing faces" << std::endl;
    lastUpdateLog += s.str();
    return;
  }
  
  hollowPicks.erase(it);
}

void Hollow::generatedMatch(BRepOffsetAPI_MakeThickSolid &operationIn, const ann::SeerShape &targetShapeIn)
{
  //note: makeThickSolid::generated is returning all kinds of shapes.
  for (const auto &targetId : targetShapeIn.getAllShapeIds())
  {
    const TopoDS_Shape &originalShape = targetShapeIn.getOCCTShape(targetId);
    const TopTools_ListOfShape &list = operationIn.Generated(originalShape);
    if (list.IsEmpty())
      continue;
    if (list.Size() > 1)
    {
      std::ostringstream s; s << "Warning: unexpected list size: " << list.Size()
        << "    in Hollow::generatedMatch " << std::endl;
      lastUpdateLog += s.str();
    }
    const TopoDS_Shape &newShape = list.First();
    assert(!newShape.IsNull());
    
    //if think generated is returning all the geometry from operation
    //so we have to skip non-existing geometry. like other half of a split.
    if (!sShape->hasShapeIdRecord(newShape))
      continue;
    
    if (!sShape->findShapeIdRecord(newShape).id.is_nil())
      continue; //only set ids for shapes with nil ids.
      
    //should only have faces left to assign. why don't nil wires come through?
    assert(newShape.ShapeType() == TopAbs_FACE);
      
    uuid freshFaceId = gu::createRandomId();
    bool results;
    std::map<uuid, uuid>::iterator it;
    std::tie(it, results) = shapeMap.insert(std::make_pair(targetId, freshFaceId));
    if (!results)
      freshFaceId = it->second;
    sShape->updateShapeIdRecord(newShape, freshFaceId);
    
    //in a hollow these generated entities are actually new entities, whereas for
    //other features, like draft and blend, this is not the case. So these get a nil evolve in.
    if (!sShape->hasEvolveRecordOut(freshFaceId))
      sShape->insertEvolve(gu::createNilId(), freshFaceId);
    
    //update the outerwire.
    uuid freshWireId = gu::createRandomId();
    std::tie(it, results) = shapeMap.insert(std::make_pair(freshFaceId, freshWireId));
    if (!results)
      freshWireId = it->second;
    
    const TopoDS_Shape &wireShape = BRepTools::OuterWire(TopoDS::Face(newShape));
    sShape->updateShapeIdRecord(wireShape, freshWireId);
  }
}

void Hollow::serialWrite(const QDir &dIn)
{
  prj::srl::Picks hPicksOut = ::ftr::serialOut(hollowPicks);
  prj::srl::FeatureHollow hollowOut
  (
    Base::serialOut(),
    hPicksOut,
    offset->serialOut(),
    label->serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::hollow(stream, hollowOut, infoMap);
}

void Hollow::serialRead(const prj::srl::FeatureHollow &sHollowIn)
{
  Base::serialIn(sHollowIn.featureBase());
  hollowPicks = ::ftr::serialIn(sHollowIn.hollowPicks());
  offset->serialIn(sHollowIn.offset());
  label->serialIn(sHollowIn.plabel());
}

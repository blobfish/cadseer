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
#include <feature/seershape.h>
#include <feature/shapecheck.h>
#include <library/plabel.h>
#include <project/serial/xsdcxxoutput/featurehollow.h>
#include <feature/hollow.h>

using namespace ftr;

using boost::uuids::uuid;

QIcon Hollow::icon;

Hollow::Hollow() : Base(), offset(ParameterNames::Offset, prf::manager().rootPtr->features().hollow().get().offset())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionHollow.svg");
  
  name = QObject::tr("Hollow");
  mainSwitch->setUserValue(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  offset.setConstraint(ParameterConstraint::buildNonZero());
  offset.connectValue(boost::bind(&Hollow::setModelDirty, this));
  
  label = new lbr::PLabel(&offset);
  label->valueHasChanged();
  overlaySwitch->addChild(label.get());
}

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
 */

void Hollow::updateModel(const UpdatePayload &payloadIn)
{
  setFailure();
  try
  {
    if (payloadIn.updateMap.count(InputType::target) != 1)
      throw std::runtime_error("no parent for hollow");
    
    const SeerShape &targetSeerShape = payloadIn.updateMap.equal_range(InputType::target).first->second->getSeerShape();
    
    TopTools_ListOfShape closingFaceShapes = resolveClosingFaces(targetSeerShape);
    
    if (closingFaceShapes.IsEmpty())
      throw std::runtime_error("Hollow: no closing faces");
    
    std::vector<uuid> firstFaceIds = targetSeerShape.resolvePick(hollowPicks.front().shapeHistory);
    if (firstFaceIds.empty())
      throw std::runtime_error("Hollow: can't find first face id");
    assert(firstFaceIds.size() == 1);
    const TopoDS_Face &firstFace = TopoDS::Face(targetSeerShape.getOCCTShape(firstFaceIds.front()));
    label->setMatrix(osg::Matrixd::translate(hollowPicks.front().getPoint(firstFace)));
    
    BRepOffsetAPI_MakeThickSolid operation;
    operation.MakeThickSolidByJoin
    (
      targetSeerShape.getRootOCCTShape(),
      closingFaceShapes,
      -offset.getValue(), //default direction sucks.
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
    
    seerShape->setOCCTShape(operation.Shape());
    seerShape->shapeMatch(targetSeerShape);
    seerShape->uniqueTypeMatch(targetSeerShape);
    seerShape->modifiedMatch(operation, targetSeerShape);
    generatedMatch(operation, targetSeerShape);
    seerShape->outerWireMatch(targetSeerShape);
    seerShape->derivedMatch();
    seerShape->dumpNils("hollow feature");
    seerShape->dumpDuplicates("hollow feature");
    seerShape->ensureNoNils();
    seerShape->ensureNoDuplicates();
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::cout << std::endl << "Error in hollow update. " << e.GetMessageString() << std::endl;
  }
  catch (std::exception &e)
  {
    std::cout << std::endl << "Error in hollow update. " << e.what() << std::endl;
  }
  setModelClean();
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
    std::cout << "warning: hollow: trying to remove a pick not in closing faces" << std::endl;
    return;
  }
  
  hollowPicks.erase(it);
}

TopTools_ListOfShape Hollow::resolveClosingFaces(const SeerShape &seerShapeIn)
{
  TopTools_ListOfShape out;
  for (const auto &closingFace : hollowPicks)
  {
    std::vector<uuid> ids = seerShapeIn.resolvePick(closingFace.shapeHistory);
    if (ids.empty())
    {
      std::cout << "feature id: " << gu::idToString(getId()) << "     can't find face: "
        << gu::idToString(closingFace.id) << " of parent" << std::endl;
      continue;
    }
    assert(ids.size() == 1);
    const TopoDS_Shape& faceShape = seerShapeIn.getOCCTShape(ids.front());
    if (faceShape.ShapeType() != TopAbs_FACE)
    {
      std::cout << "feature id: " << gu::idToString(getId()) << "      wrong shape type: "
        << shapeStrings.at(faceShape.ShapeType()) << std::endl;
      continue;
    }
    
    out.Append(faceShape);
  }
  
  return out;
}

void Hollow::generatedMatch(BRepOffsetAPI_MakeThickSolid &operationIn, const SeerShape &targetShapeIn)
{
  //note: makeThickSolid::generated is returning all kinds of shapes.
  for (const auto &targetId : targetShapeIn.getAllShapeIds())
  {
    const TopoDS_Shape &originalShape = targetShapeIn.getOCCTShape(targetId);
    const TopTools_ListOfShape &list = operationIn.Generated(originalShape);
    if (list.IsEmpty())
      continue;
    if (list.Size() > 1)
      std::cout << "Warning: unexpected list size: " << list.Size()
      << "    in Hollow::generatedMatch " << std::endl;
    const TopoDS_Shape &newShape = list.First();
    assert(!newShape.IsNull());
    
    //if think generated is returning all the geometry from operation
    //so we have to skip non-existing geometry. like other half of a split.
    if (!seerShape->hasShapeIdRecord(newShape))
      continue;
    
    if (!seerShape->findShapeIdRecord(newShape).id.is_nil())
      continue; //only set ids for shapes with nil ids.
      
    //should only have faces left to assign. why don't nil wires come through?
    assert(newShape.ShapeType() == TopAbs_FACE);
      
    uuid freshFaceId = gu::createRandomId();
    bool results;
    std::map<uuid, uuid>::iterator it;
    std::tie(it, results) = shapeMap.insert(std::make_pair(targetId, freshFaceId));
    if (!results)
      freshFaceId = it->second;
    seerShape->updateShapeIdRecord(newShape, freshFaceId);
    
    //in a hollow these generated entities are actually new entities, whereas for
    //other features, like draft and blend, this is not the case. So these get a nil evolve in.
    if (!seerShape->hasEvolveRecordOut(freshFaceId))
      seerShape->insertEvolve(gu::createNilId(), freshFaceId);
    
    //update the outerwire.
    uuid freshWireId = gu::createRandomId();
    std::tie(it, results) = shapeMap.insert(std::make_pair(freshFaceId, freshWireId));
    if (!results)
      freshWireId = it->second;
    
    const TopoDS_Shape &wireShape = BRepTools::OuterWire(TopoDS::Face(newShape));
    seerShape->updateShapeIdRecord(wireShape, freshWireId);
  }
}

void Hollow::serialWrite(const QDir &dIn)
{
  prj::srl::Picks hPicksOut = ::ftr::serialOut(hollowPicks);
  prj::srl::FeatureHollow hollowOut
  (
    Base::serialOut(),
    hPicksOut,
    offset.serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::hollow(stream, hollowOut, infoMap);
}

void Hollow::serialRead(const prj::srl::FeatureHollow &sHollowIn)
{
  Base::serialIn(sHollowIn.featureBase());
  hollowPicks = ::ftr::serialIn(sHollowIn.hollowPicks());
  offset.serialIn(sHollowIn.offset());
  offset.connectValue(boost::bind(&Hollow::setModelDirty, this));
  
  label->valueHasChanged();
}


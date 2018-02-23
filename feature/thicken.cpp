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

#include <TopoDS.hxx>
#include <BRepOffsetAPI_MakeOffsetShape.hxx>
#include <BRepTools.hxx>

#include <osg/Switch>

#include <globalutilities.h>
#include <tools/occtools.h>
#include <tools/idtools.h>
#include <library/plabel.h>
#include <annex/seershape.h>
#include <tools/featuretools.h>
#include <project/serial/xsdcxxoutput/featurethicken.h>
#include <feature/updatepayload.h>
#include <feature/parameter.h>
#include <feature/inputtype.h>
#include <feature/thicken.h>

using namespace ftr;

using boost::uuids::uuid;

QIcon Thicken::icon;

Thicken::Thicken():
Base(),
distance(new prm::Parameter(prm::Names::Distance, 0.1)),
sShape(new ann::SeerShape())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionThicken.svg");
  
  name = QObject::tr("Thicken");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  distance->setConstraint(prm::Constraint::buildNonZero());
  distance->connectValue(boost::bind(&Thicken::setModelDirty, this));
  parameters.push_back(distance.get());
  
  distanceLabel = new lbr::PLabel(distance.get());
  distanceLabel->showName = true;
  distanceLabel->valueHasChanged();
  overlaySwitch->addChild(distanceLabel.get());
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
  
  solidId = gu::createRandomId();
  shellId = gu::createRandomId();
}

Thicken::~Thicken(){}

//duplicated in offset
static std::string getErrorString(const BRepOffset_MakeOffset &bIn)
{
  static const std::map<BRepOffset_Error, std::string> errors = 
  {
    std::make_pair(BRepOffset_NoError, "BRepOffset_NoError"),
    std::make_pair(BRepOffset_UnknownError, "BRepOffset_UnknownError"),
    std::make_pair(BRepOffset_BadNormalsOnGeometry, "BRepOffset_BadNormalsOnGeometry"),
    std::make_pair(BRepOffset_C0Geometry, "BRepOffset_C0Geometry"),
    std::make_pair(BRepOffset_NullOffset, "BRepOffset_NullOffset"),
    std::make_pair(BRepOffset_NotConnectedShell, "BRepOffset_NotConnectedShell")
  };
  
  BRepOffset_Error e = bIn.Error();
  std::ostringstream stream;
  stream << "failed with error: " << errors.at(e);
  return stream.str();
}

void Thicken::updateModel(const UpdatePayload &payloadIn)
{
  setFailure();
  lastUpdateLog.clear();
  sShape->reset();
  try
  {
    std::vector<const Base*> tfs = payloadIn.getFeatures(InputType::target);
    if (tfs.size() != 1)
      throw std::runtime_error("wrong number of parents");
    if (!tfs.front()->hasAnnex(ann::Type::SeerShape))
      throw std::runtime_error("parent doesn't have seer shape.");
    const ann::SeerShape &tss = tfs.front()->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
    if (tss.isNull())
      throw std::runtime_error("target seer shape is null");
    
    //setup failure state.
    sShape->setOCCTShape(tss.getRootOCCTShape());
    sShape->shapeMatch(tss);
    sShape->uniqueTypeMatch(tss);
    sShape->outerWireMatch(tss);
    sShape->derivedMatch();
    sShape->ensureNoNils();
    sShape->ensureNoDuplicates();
    
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    std::vector<uuid> targetShapes = tss.useGetChildrenOfType(tss.getRootShapeId(), TopAbs_SHELL);
    if (targetShapes.empty())
    {
      targetShapes = tss.useGetChildrenOfType(tss.getRootShapeId(), TopAbs_FACE);
      if (targetShapes.empty())
        throw std::runtime_error("target seer shape has no shells or faces");
    }
    if (targetShapes.size() > 1)
    {
      std::ostringstream s; s << "WARNING: target has more than 1 target shape" << std::endl;
      lastUpdateLog += s.str();
    }
    const TopoDS_Shape &targetShape = tss.getOCCTShape(targetShapes.front());
    if (targetShape.Closed())
      throw std::runtime_error("target shape is closed");
    
    BRepOffset_MakeOffset builder;
    builder.Initialize
    (
      targetShape,
      static_cast<double>(*distance), //offset
      1.0e-06, //same tolerance as the sewing default.
      BRepOffset_Skin, //offset mode
      Standard_False, //intersection
      Standard_False, //self intersection
      GeomAbs_Intersection, //join type.
      Standard_True, //thickening.
      Standard_False //remove internal edges
    );
    builder.MakeOffsetShape();
    if (!builder.IsDone())
      throw std::runtime_error(getErrorString(builder));
    
    sShape->setOCCTShape(builder.Shape());
    sShape->shapeMatch(tss);
    sShape->uniqueTypeMatch(tss);
    thickenMatch(builder);
    sShape->outerWireMatch(tss);
    sShape->derivedMatch();
    sShape->dumpNils("thicken feature");
    sShape->dumpDuplicates("thicken feature");
    sShape->ensureNoNils();
    sShape->ensureNoDuplicates();
    
    occt::BoundingBox bb(builder.Shape());
    distanceLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(bb.getCenter())));
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in thicken update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in thicken update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in thicken update. " << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Thicken::thickenMatch(const BRepOffset_MakeOffset &offseter)
{
  const BRepAlgo_Image &oFaces = offseter.OffsetFacesFromShapes();
  const BRepAlgo_Image &oEdges = offseter.OffsetEdgesFromShapes();
  occt::ShapeVector as = sShape->getAllShapes();
  
  //testing results
//   int fc = 0;
//   int ec = 0;
//   for (const auto &s : as)
//   {
//     if (oFaces.HasImage(s))
//       fc++;
//     if (oEdges.HasImage(s))
//       ec++;
//   }
//   std::cout << std::endl << "face count is: " << fc
//   << "    edge count is: " << ec
//   << "    closing face count is: " << offseter.ClosingFaces().Extent()
//   << std::endl;
  
  /* this is ran post shapeMatch, so the shapes that come from target should be assigned an id.
   * the offset faces and edges are in the BRepAlgo_Images. So all that is left is the boundary
   * faces and edges. I tested and the offset images are in both the target seer shape and 'this' seer shape.
   */
  
  /* boundary faces:
   * any edge that is assigned an id is 'copy' of the target edge. So we will find a face
   * that is connected to this edge and has a nil id and that is a boundary face. We will use
   * the boundary map to store these associations.
   * 
   */
  for (const auto &s : as)
  {
    if (!oEdges.HasImage(s))
      continue;
    uuid edgeId = sShape->findShapeIdRecord(s).id;
    assert(!edgeId.is_nil());
    if (edgeId.is_nil())
      continue;
    occt::ShapeVector parentFaces = sShape->useGetParentsOfType(s, TopAbs_FACE);
    assert(parentFaces.size() == 2);
    TopoDS_Shape nilParent;
    TopoDS_Shape nonNilParent;
    for (const auto &pf : parentFaces)
    {
      if (sShape->findShapeIdRecord(pf).id.is_nil())
        nilParent = pf;
      else
        nonNilParent = pf;
    }
    //if both parent faces have an id, this is an internal edge and not a boundary, so skip.
    if (nilParent.IsNull())
      continue;
    auto bit = boundaryMap.find(edgeId);
    uuid idForNilFace;
    if (bit == boundaryMap.end())
    {
      idForNilFace = gu::createRandomId();
      boundaryMap.insert(std::make_pair(edgeId, idForNilFace));
    }
    else
      idForNilFace = bit->second;
    sShape->updateShapeIdRecord(nilParent, idForNilFace);
    if (!sShape->hasEvolveRecordOut(idForNilFace))
      sShape->insertEvolve(gu::createNilId(), idForNilFace);
  }
  
  //now assign the offset faces and edges and derived match should handle the rest.
  auto update = [&](const TopoDS_Shape &source, const TopoDS_Shape &offset, std::map<uuid, uuid> &map)
  {
    const uuid &sourceId = sShape->findShapeIdRecord(source).id;
    assert(!sourceId.is_nil());
    if (sourceId.is_nil())
      return;
    assert(sShape->hasShapeIdRecord(offset));
    if (!sShape->hasShapeIdRecord(offset))
      return;
    uuid idForNilShape;
    auto it = map.find(sourceId);
    if (it == map.end())
    {
      idForNilShape = gu::createRandomId();
      map.insert(std::make_pair(sourceId, idForNilShape));
    }
    else
      idForNilShape = it->second;
    sShape->updateShapeIdRecord(offset, idForNilShape);
    if (!sShape->hasEvolveRecordOut(idForNilShape))
      sShape->insertEvolve(gu::createNilId(), idForNilShape);
  };
  
  for (const auto &s : as)
  {
    if (oFaces.HasImage(s))
      update(s, oFaces.Image(s).First(), faceMap);
    else if (oEdges.HasImage(s))
      update(s, oEdges.Image(s).First(), edgeMap);
  }
  
  //faces should have an id now, so map outer wire
  for (const auto &s : as)
  {
    if (s.ShapeType() != TopAbs_FACE)
      continue;
    uuid fid = sShape->findShapeIdRecord(s).id;
    assert(!fid.is_nil());
    if (fid.is_nil())
    {
      std::cout << "WARNING: face id is nil assigning outer wires" << std::endl;
      continue;
    }
    TopoDS_Wire ow = BRepTools::OuterWire(TopoDS::Face(s));
    assert(!ow.IsNull());
    assert(sShape->hasShapeIdRecord(ow));
    if (!sShape->hasShapeIdRecord(ow))
    {
      std::cout << "WARNING: outer wire is not in seershape when assigning outer wires" << std::endl;
      continue;
    }
    uuid owid = sShape->findShapeIdRecord(ow).id;
    if (!owid.is_nil())
      continue;
    uuid idForNilShape;
    auto it = oWireMap.find(fid);
    if (it == oWireMap.end())
    {
      idForNilShape = gu::createRandomId();
      oWireMap.insert(std::make_pair(fid, idForNilShape));
    }
    else
      idForNilShape = it->second;
    sShape->updateShapeIdRecord(ow, idForNilShape);
    if (!sShape->hasEvolveRecordOut(idForNilShape))
      sShape->insertEvolve(gu::createNilId(), idForNilShape);
  }
  
  //now do the solid and shell.
  //shell maybe already assigned by uniqueTypeMatch, if we thickened the sheet.
  occt::ShapeVector solids = sShape->useGetChildrenOfType(sShape->getRootOCCTShape(), TopAbs_SOLID);
  assert(solids.size() == 1);
  if (solids.size() != 1)
    throw std::runtime_error("wrong number of result solids");
  sShape->updateShapeIdRecord(solids.front(), solidId);
  if (!sShape->hasEvolveRecordOut(solidId))
    sShape->insertEvolve(gu::createNilId(), solidId);
  
  //no link to possible target shell.
  occt::ShapeVector shells = sShape->useGetChildrenOfType(sShape->getRootOCCTShape(), TopAbs_SHELL);
  assert(shells.size() == 1);
  if (shells.size() != 1)
    throw std::runtime_error("wrong number of result shells");
  sShape->updateShapeIdRecord(shells.front(), shellId);
  if (!sShape->hasEvolveRecordOut(shellId))
    sShape->insertEvolve(gu::createNilId(), shellId);
}

void Thicken::serialWrite(const QDir &dIn)
{
  auto serializeMap = [](const std::map<uuid, uuid> &map) -> prj::srl::EvolveContainer
  {
    prj::srl::EvolveContainer out;
    for (const auto &p : map)
    {
      prj::srl::EvolveRecord r
      (
        gu::idToString(p.first),
        gu::idToString(p.second)
      );
      out.evolveRecord().push_back(r);
    }
    return out;
  };
  
  prj::srl::FeatureThicken so
  (
    Base::serialOut(),
    distance->serialOut(),
    distanceLabel->serialOut(),
    serializeMap(faceMap),
    serializeMap(edgeMap),
    serializeMap(boundaryMap),
    serializeMap(oWireMap),
    gu::idToString(solidId),
    gu::idToString(shellId)
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::thicken(stream, so, infoMap);
}

void Thicken::serialRead(const prj::srl::FeatureThicken &so)
{
  auto serializeMap = [](const prj::srl::EvolveContainer &container) -> std::map<uuid, uuid>
  {
    std::map<uuid, uuid> out;
    for (const auto &r : container.evolveRecord())
      out.insert(std::make_pair(gu::stringToId(r.idIn()), gu::stringToId(r.idOut())));
    return out;
  };
  
  Base::serialIn(so.featureBase());
  distance->serialIn(so.distance());
  distanceLabel->serialIn(so.distanceLabel());
  faceMap = serializeMap(so.faceMap());
  edgeMap = serializeMap(so.edgeMap());
  boundaryMap = serializeMap(so.boundaryMap());
  oWireMap = serializeMap(so.oWireMap());
  solidId = gu::stringToId(so.solidId());
  shellId = gu::stringToId(so.shellId());
}

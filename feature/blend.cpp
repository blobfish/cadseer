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

#include <assert.h>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/string_generator.hpp>

#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <TopExp.hxx> //maybe temp.
#include <TopTools_IndexedMapOfShape.hxx> //maybe temp.
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopExp_Explorer.hxx>
#include <BOPTools_AlgoTools.hxx>
#include <BRepTools.hxx>

#include <globalutilities.h>
#include <project/serial/xsdcxxoutput/featureblend.h>
#include <feature/shapeidmapper.h>
#include <feature/blend.h>

using namespace ftr;
using boost::uuids::uuid;

QIcon Blend::icon;

Blend::Blend() : Base(), radius(1.0)
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionBlend.svg");
  
  name = QObject::tr("Blend");
}

void Blend::setRadius(const double& radiusIn)
{
  if (radius == radiusIn)
    return;
  assert(radiusIn > Precision::Confusion());
  setModelDirty();
  radius = radiusIn;
}

void Blend::setEdgeIds(const std::vector<boost::uuids::uuid>& edgeIdsIn)
{
  edgeIds = edgeIdsIn;
  setModelDirty();
}

void Blend::updateModel(const UpdateMap& mapIn)
{
  //clear shape so if we fail the feature will be empty. what about maps?
  shape = TopoDS_Shape();
  setFailure(); //assume failure until succes.
  
  if (mapIn.count(InputTypes::target) < 1)
    return; //much better error handeling.
  
  const TopoDS_Shape &targetShape = mapIn.at(InputTypes::target)->getShape();
  const ResultContainer& targetResultContainer = mapIn.at(InputTypes::target)->getResultContainer();
  
  try
  {
    BRepFilletAPI_MakeFillet blendMaker(targetShape);
    for (const auto &currentId : edgeIds)
    {
      if (!hasResult(targetResultContainer, currentId))
      {
	std::cout << "Blend: can't find target edge id. Skipping id: " << boost::uuids::to_string(currentId) << std::endl;
	continue;
      }
      TopoDS_Shape tempShape = findResultById(targetResultContainer, currentId).shape;
      assert(!tempShape.IsNull());
      assert(tempShape.ShapeType() == TopAbs_EDGE);
      blendMaker.Add(radius, TopoDS::Edge(tempShape));
    }
    blendMaker.Build();
    if (!blendMaker.IsDone())
      return;
//     dumpInfo(blendMaker, mapIn.at(InputTypes::target));
    shape = blendMaker.Shape();
    ResultContainer freshContainer = createInitialContainer(shape);
    shapeMatch(targetResultContainer, freshContainer);
    modifiedMatch(blendMaker, mapIn.at(InputTypes::target)->getResultContainer(), freshContainer);
    generatedMatch(blendMaker, mapIn.at(InputTypes::target), freshContainer);
    uniqueTypeMatch(targetResultContainer, freshContainer);
    outerWireMatch(targetResultContainer, freshContainer);
    derivedMatch(shape, freshContainer, derivedContainer);
    dumpNils(freshContainer, "blend feature"); //only if there are shapes with nil ids.
    dumpDuplicates(freshContainer, "blend feature");
    ensureNoNils(freshContainer);
    ensureNoDuplicates(freshContainer);
    resultContainer = freshContainer;
    
    setSuccess();
  }
  catch (Standard_Failure)
  {
    Handle_Standard_Failure e = Standard_Failure::Caught();
    std::cout << std::endl << "Error in cylinder update. " << e->GetMessageString() << std::endl;
  }
  setModelClean();
}

/* matching by interogating the MakeFillet objects generated method.
 * generated only outputs the fillet faces from the edge. Nothing else.
 * tried iterating over MakeFillet contours and edges, but in certain situations
 * blends are made from vertices. blending 3 edges of a box corner is such a situation.
 * so we will iterate over all the shapes of the targe object.
 */
void Blend::generatedMatch(BRepFilletAPI_MakeFillet &blendMakerIn, const Base *targetFeatureIn, ResultContainer &freshContainer)
{
  TopTools_IndexedMapOfShape targetShapeMap;
  TopExp::MapShapes(targetFeatureIn->getShape(), targetShapeMap);
  
  for (int index = 1; index <= targetShapeMap.Extent(); ++index)
  {
    const TopoDS_Shape &currentShape = targetShapeMap(index);
    TopTools_ListOfShape generated = blendMakerIn.Generated(currentShape);
    if (generated.IsEmpty())
      continue;
    assert(generated.Extent() == 1);
    const TopoDS_Shape &blendedFace = generated.First();
    assert(!blendedFace.IsNull());
    assert(blendedFace.ShapeType() == TopAbs_FACE);
    
    //targetEdgeId should also be in member edgeIds
    uuid targetEdgeId = findResultByShape(targetFeatureIn->getResultContainer(), currentShape).id;
    uuid blendedFaceId = boost::uuids::nil_generator()();
    //first time edge has been blended
    if (!hasInId(shapeMap, targetEdgeId))
    {
      //build new record.
      EvolutionRecord record;
      record.inId = targetEdgeId;
      record.outId = idGenerator();
      shapeMap.insert(record);
      
      blendedFaceId = record.outId;
    }
    else
      blendedFaceId = findRecordByIn(shapeMap, targetEdgeId).outId;
    
    //now we have the id for the face, just update the result map.
    updateId(freshContainer, blendedFaceId, blendedFace);
    
    //now look for outerwire for newly generated face.
    uuid blendedFaceWireId = boost::uuids::nil_generator()();
    if (!hasInId(shapeMap, blendedFaceId))
    {
      //this means that the face id is in both columns.
      EvolutionRecord record;
      record.inId = blendedFaceId;
      record.outId = idGenerator();
      shapeMap.insert(record);
      
      blendedFaceWireId = record.outId;
    }
    else
      blendedFaceWireId = findRecordByIn(shapeMap, blendedFaceId).outId;
    
    //now get the wire and update the result to id.
    const TopoDS_Shape &blendedFaceWire = BRepTools::OuterWire(TopoDS::Face(blendedFace));
    updateId(freshContainer, blendedFaceWireId, blendedFaceWire);
  }
}

void Blend::dumpInfo(BRepFilletAPI_MakeFillet &blendMakerIn, const Base *targetFeatureIn)
{
  std::cout << std::endl << std::endl <<
    "shape out type is: " << gu::getShapeTypeString(blendMakerIn.Shape()) << std::endl <<
    "fillet dump:" << std::endl;
  
  const TopoDS_Shape &targetShape = targetFeatureIn->getShape();
  TopTools_IndexedMapOfShape localShapeMap;
  TopExp::MapShapes(targetShape, localShapeMap);
  for (int index = 1; index <= localShapeMap.Extent(); ++index)
  {
    const TopoDS_Shape &currentShape = localShapeMap(index);
    std::cout << "ShapeType is: " << std::setw(10) << gu::getShapeTypeString(currentShape) << 
      "      Generated Count is: " << blendMakerIn.Generated(currentShape).Extent() <<
      "      Modified Count is: " << blendMakerIn.Modified(currentShape).Extent() <<
      "      is deleted: " << ((blendMakerIn.IsDeleted(currentShape)) ? "true" : "false") << std::endl; 
      
      if (blendMakerIn.Generated(currentShape).Extent() > 0)
        std::cout << "   generated type is: " << 
          gu::getShapeTypeString(blendMakerIn.Generated(currentShape).First()) << std::endl;
  }
  
  std::cout << std::endl << std::endl << "output of blend: " << std::endl;
  TopTools_IndexedMapOfShape outShapeMap;
  TopExp::MapShapes(blendMakerIn.Shape(), outShapeMap);
  for (int index = 1; index <= outShapeMap.Extent(); ++index)
  {
    const TopoDS_Shape &currentShape = outShapeMap(index);
    std::cout << "ShapteType is: " << std::setw(10) << gu::getShapeTypeString(currentShape) <<
    "      is in targetResults: " <<
      ((hasResult(targetFeatureIn->getResultContainer(), currentShape)) ? "true" : "false") << std::endl;
  }
}

void Blend::serialWrite(const QDir &dIn)
{
  prj::srl::FeatureBlend::EdgeIdsType edgeIdsOut;
  for (const auto &lId : edgeIds)
    edgeIdsOut.id().push_back(boost::uuids::to_string(lId));
  
  prj::srl::FeatureBlend::ShapeMapType shapeMapOut;
  typedef EvolutionContainer::index<EvolutionRecord::ByInId>::type EList;
  const EList &eList = shapeMap.get<EvolutionRecord::ByInId>();
  for (EList::const_iterator it = eList.begin(); it != eList.end(); ++it)
  {
    prj::srl::EvolutionRecord eRecord
    (
      boost::uuids::to_string(it->inId),
      boost::uuids::to_string(it->outId)
    );
    shapeMapOut.evolutionRecord().push_back(eRecord);
  }
  
  prj::srl::FeatureBlend blendOut
  (
    Base::serialOut(),
    radius,
    edgeIdsOut,
    shapeMapOut
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::blend(stream, blendOut, infoMap);
}

void Blend::serialRead(const prj::srl::FeatureBlend& sBlendIn)
{
  boost::uuids::string_generator gen;
  
  Base::serialIn(sBlendIn.featureBase());
  
  radius = sBlendIn.radius();
  
  edgeIds.clear();
  for (const auto &stringId : sBlendIn.edgeIds().id())
    edgeIds.push_back(gen(stringId));
  
  shapeMap.get<EvolutionRecord::ByInId>().clear();
  for (const prj::srl::EvolutionRecord &sERecord : sBlendIn.shapeMap().evolutionRecord())
  {
    EvolutionRecord record;
    record.inId = gen(sERecord.idIn());
    record.outId = gen(sERecord.idOut());
    shapeMap.insert(record);
  }
}

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
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <TopExp.hxx> //maybe temp.
#include <TopTools_IndexedMapOfShape.hxx> //maybe temp.
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopExp_Explorer.hxx>
#include <BOPTools_AlgoTools.hxx>
#include <BRepTools.hxx>
#include <TColgp_Array1OfPnt2d.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRep_Tool.hxx>
#include <Geom_Curve.hxx>

#include <globalutilities.h>
#include <project/serial/xsdcxxoutput/featureblend.h>
#include <feature/shapecheck.h>
#include <feature/seershape.h>
#include <feature/blend.h>

using namespace ftr;
using boost::uuids::uuid;

QIcon Blend::icon;

Blend::Blend() : Base()
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionBlend.svg");
  
  name = QObject::tr("Blend");
  mainSwitch->setUserValue(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
}

std::shared_ptr< Parameter > Blend::buildRadiusParameter()
{
  //some kind of default radius?
  std::shared_ptr<Parameter> out(new Parameter(ParameterNames::Radius, 1.0));
  return out;
}

std::shared_ptr< Parameter > Blend::buildPositionParameter()
{
  std::shared_ptr<Parameter> out(new Parameter(ParameterNames::Position, 0.0));
  return out;
}

VariableBlend Blend::buildDefaultVariable(const SeerShape &seerShapeIn, const BlendPick &pickIn)
{
  VariableBlend out;
  out.pick = pickIn;
  
  const TopoDS_Shape &rootShape = seerShapeIn.getRootOCCTShape();
  BRepFilletAPI_MakeFillet blendMaker(rootShape);
  const TopoDS_Shape &edge = seerShapeIn.getOCCTShape(pickIn.id); //TODO might be face!
  blendMaker.Add(TopoDS::Edge(edge));
  
  //no position parameter for vertices
  VariableEntry entry1;
  entry1.id = seerShapeIn.findShapeIdRecord(blendMaker.FirstVertex(1)).id;
  entry1.radius = buildRadiusParameter();
  out.entries.push_back(entry1);
  
  VariableEntry entry2;
  entry2.id = seerShapeIn.findShapeIdRecord(blendMaker.LastVertex(1)).id;
  entry2.radius = buildRadiusParameter();
  entry2.radius->setValue(1.5);
  out.entries.push_back(entry2);
  
  return out;
}

double Blend::calculateUParameter(const TopoDS_Edge &edgeIn, const osg::Vec3d &point)
{
  TopoDS_Shape aVertex = BRepBuilderAPI_MakeVertex(gp_Pnt(point.x(), point.y(), point.z())).Vertex();
  BRepExtrema_DistShapeShape dist(edgeIn, aVertex);
  if (dist.NbSolution() < 1)
  {
    std::cout << "Warning: no solution for distshapeshape in Blend::calculateU" << std::endl;
    return 0.0;
  }
  if (dist.SupportTypeShape1(1) != BRepExtrema_IsOnEdge)
  {
    std::cout << "Warning: wrong support type in Blend::calculateU" << std::endl;
    return 0.0;
  }
  
  double closestParameter;
  dist.ParOnEdgeS1(1, closestParameter);
  
  double firstP, lastP;
  BRep_Tool::Curve(edgeIn, firstP, lastP);
  //normalize parameter to 0 to 1.
  return (closestParameter - firstP) / (lastP - firstP);
}

osg::Vec3d Blend::calculateUPoint(const TopoDS_Edge &edgeIn, double parameter)
{
  double firstP, lastP;
  const Handle_Geom_Curve &curve = BRep_Tool::Curve(edgeIn, firstP, lastP);
  double parameterOut = parameter * (lastP - firstP) + firstP;
  gp_Pnt occPoint;
  curve->D0(parameterOut, occPoint);
  return gu::toOsg(occPoint);
}

void Blend::addSimpleBlend(const SimpleBlend &simpleBlendIn)
{
  if (simpleBlendIn.radius)
    simpleBlends.push_back(simpleBlendIn);
  else
  {
    SimpleBlend temp = simpleBlendIn;
    temp.radius = buildRadiusParameter();
    simpleBlends.push_back(temp);
  }
  simpleBlends.back().radius->connectValue(boost::bind(&Blend::setModelDirty, this));
  if (!simpleBlends.back().label)
    simpleBlends.back().label = new lbr::PLabel(simpleBlends.back().radius.get());
  simpleBlends.back().label->valueHasChanged();
  overlaySwitch->addChild(simpleBlends.back().label.get());
}

void Blend::addVariableBlend(const VariableBlend &variableBlendIn)
{
  variableBlends.push_back(variableBlendIn);
  for (const auto &e : variableBlends.back().entries)
  {
    if (e.position)
      e.position->connectValue(boost::bind(&Blend::setModelDirty, this));
    e.radius->connectValue(boost::bind(&Blend::setModelDirty, this));
  }
}

void Blend::updateModel(const UpdateMap& mapIn)
{
  setFailure();
  try
  {
    if (mapIn.count(InputTypes::target) != 1)
      throw std::runtime_error("no parent for blend");
    
    const SeerShape &targetSeerShape = mapIn.equal_range(InputTypes::target).first->second->getSeerShape();
    
    //starting from the stand point that this feature has failed.
    //set the shape and container to the parent target.
    seerShape->partialAssign(targetSeerShape);
  
    BRepFilletAPI_MakeFillet blendMaker(targetSeerShape.getRootOCCTShape());
    for (const auto &simpleBlend : simpleBlends)
    {
      bool labelDone = false; //set label position to first pick.
      for (const auto &pick : simpleBlend.picks)
      {
	if (!targetSeerShape.hasShapeIdRecord(pick.id))
	{
	  std::cout << "Blend: can't find target edge id. Skipping id: " << boost::uuids::to_string(pick.id) << std::endl;
	  continue;
	}
	TopoDS_Shape tempShape = targetSeerShape.getOCCTShape(pick.id);
	assert(!tempShape.IsNull());
	assert(tempShape.ShapeType() == TopAbs_EDGE);
	blendMaker.Add(simpleBlend.radius->getValue(), TopoDS::Edge(tempShape));
	//update location of parameter label.
	if (!labelDone)
	{
	  labelDone = true;
	  simpleBlend.label->setMatrix(osg::Matrixd::translate(calculateUPoint(TopoDS::Edge(tempShape), pick.u)));
	}
      }
    }
    std::size_t vBlendIndex = 1;
    for (const auto &vBlend : variableBlends)
    {
      if (!targetSeerShape.hasShapeIdRecord(vBlend.pick.id))
      {
	std::cout << "Blend: can't find target edge id. Skipping id: " << boost::uuids::to_string(vBlend.pick.id) << std::endl;
	continue;
      }
      TopoDS_Shape tempShape = targetSeerShape.getOCCTShape(vBlend.pick.id);
      assert(!tempShape.IsNull());
      assert(tempShape.ShapeType() == TopAbs_EDGE); //TODO faces someday.
      blendMaker.Add(TopoDS::Edge(tempShape));
      for (const auto &e : vBlend.entries)
      {
	const TopoDS_Shape &blendShape = targetSeerShape.getOCCTShape(e.id);
	if (blendShape.ShapeType() == TopAbs_VERTEX)
	  blendMaker.SetRadius(e.radius->getValue(), vBlendIndex, TopoDS::Vertex(blendShape));
	//TODO deal with edges.
      }
      vBlendIndex++;
    }
    blendMaker.Build();
    if (!blendMaker.IsDone())
      throw std::runtime_error("blendMaker failed");
    ShapeCheck check(blendMaker.Shape());
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    seerShape->setOCCTShape(blendMaker.Shape());
    seerShape->shapeMatch(targetSeerShape);
    seerShape->uniqueTypeMatch(targetSeerShape);
    seerShape->modifiedMatch(blendMaker, targetSeerShape);
    generatedMatch(blendMaker, targetSeerShape);
    ensureNoFaceNils(); //hack see notes in function.
    seerShape->outerWireMatch(targetSeerShape);
    seerShape->derivedMatch();
    seerShape->dumpNils("blend feature");
    seerShape->dumpDuplicates("blend feature");
    seerShape->ensureNoNils();
    seerShape->ensureNoDuplicates();
    
    setSuccess();
  }
  catch (Standard_Failure)
  {
    Handle_Standard_Failure e = Standard_Failure::Caught();
    std::cout << std::endl << "Error in blend update. " << e->GetMessageString() << std::endl;
  }
  catch (std::exception &e)
  {
    std::cout << std::endl << "Error in blend update. " << e.what() << std::endl;
  }
  setModelClean();
}

/* matching by interogating the MakeFillet objects generated method.
 * generated only outputs the fillet faces from the edge. Nothing else.
 * tried iterating over MakeFillet contours and edges, but in certain situations
 * blends are made from vertices. blending 3 edges of a box corner is such a situation.
 * so we will iterate over all the shapes of the targe object.
 */
void Blend::generatedMatch(BRepFilletAPI_MakeFillet &blendMakerIn, const SeerShape &targetShapeIn)
{
  //duplicated with chamfer.
  using boost::uuids::uuid;
  
  std::vector<uuid> targetShapeIds = targetShapeIn.getAllShapeIds();
  
  for (const auto &cId : targetShapeIds)
  {
    const TopoDS_Shape &currentShape = targetShapeIn.findShapeIdRecord(cId).shape;
    TopTools_ListOfShape generated = blendMakerIn.Generated(currentShape);
    if (generated.IsEmpty())
      continue;
    if(generated.Extent() != 1) //see ensure noFaceNils
      std::cout << "Warning: more than one generated shape in blend::generatedMatch" << std::endl;
    const TopoDS_Shape &blendFace = generated.First();
    assert(!blendFace.IsNull());
    assert(blendFace.ShapeType() == TopAbs_FACE);
    
    std::map<uuid, uuid>::iterator mapItFace;
    bool dummy;
    std::tie(mapItFace, dummy) = shapeMap.insert(std::make_pair(cId, idGenerator()));
    
    //now we have the id for the face, just update the result map.
    seerShape->updateShapeIdRecord(blendFace, mapItFace->second);
    
    //now look for outerwire for newly generated face.
    //we use the generated face id to map to outer wire.
    std::map<uuid, uuid>::iterator mapItWire;
    std::tie(mapItWire, dummy) = shapeMap.insert(std::make_pair(mapItFace->second, idGenerator()));
    
    //now get the wire and update the result to id.
    const TopoDS_Shape &blendFaceWire = BRepTools::OuterWire(TopoDS::Face(blendFace));
    seerShape->updateShapeIdRecord(blendFaceWire, mapItWire->second);
  }
}

void Blend::ensureNoFaceNils()
{
  //ok this is a hack. Because of an OCCT bug, generated is missing
  //an edge to blended face mapping and on another edge has 2 faces
  //generated from a single edge. Without the face having an id, derived
  //shapes (edges and verts) can't be assigned an id. This causes a lot
  //of id failures. So for now just assign an id and check for bug
  //in next release.
  
  
  auto shapes = seerShape->getAllShapes();
  for (const auto &shape : shapes)
  {
    if (shape.ShapeType() != TopAbs_FACE)
      continue;
    if (!seerShape->findShapeIdRecord(shape).id.is_nil())
      continue;
    
    seerShape->updateShapeIdRecord(shape, idGenerator());
  }
}

void Blend::dumpInfo(BRepFilletAPI_MakeFillet &blendMakerIn, const SeerShape &targetFeatureIn)
{
  std::cout << std::endl << std::endl <<
    "shape out type is: " << gu::getShapeTypeString(blendMakerIn.Shape()) << std::endl <<
    "fillet dump:" << std::endl;
  
  auto shapes = targetFeatureIn.getAllShapes();
  for (const auto &currentShape : shapes)
  {
    std::cout << "ShapeType is: " << std::setw(10) << gu::getShapeTypeString(currentShape) << 
      "      Generated Count is: " << blendMakerIn.Generated(currentShape).Extent() <<
      "      Modified Count is: " << blendMakerIn.Modified(currentShape).Extent() <<
      "      is deleted: " << ((blendMakerIn.IsDeleted(currentShape)) ? "true" : "false") << std::endl; 
      
      if (blendMakerIn.Generated(currentShape).Extent() > 0)
        std::cout << "   generated type is: " << 
          gu::getShapeTypeString(blendMakerIn.Generated(currentShape).First()) << std::endl;
  }
  
//   std::cout << std::endl << std::endl << "output of blend: " << std::endl;
//   TopTools_IndexedMapOfShape outShapeMap;
//   TopExp::MapShapes(blendMakerIn.Shape(), outShapeMap);
//   for (int index = 1; index <= outShapeMap.Extent(); ++index)
//   {
//     const TopoDS_Shape &currentShape = outShapeMap(index);
//     std::cout << "ShapteType is: " << std::setw(10) << gu::getShapeTypeString(currentShape) <<
//     "      is in targetResults: " <<
//       ((hasResult(targetFeatureIn->getResultContainer(), currentShape)) ? "true" : "false") << std::endl;
//   }
}

void Blend::serialWrite(const QDir &dIn)
{
  using boost::uuids::to_string;
  
  prj::srl::FeatureBlend::ShapeMapType shapeMapOut;
  for (const auto &p : shapeMap)
  {
    prj::srl::EvolveRecord eRecord
    (
      to_string(p.first),
      to_string(p.second)
    );
    shapeMapOut.evolveRecord().push_back(eRecord);
  }
  
  prj::srl::SimpleBlends sBlendsOut;
  for (const auto &sBlend : simpleBlends)
  {
    prj::srl::BlendPicks bPicksOut;
    for (const auto &bPick : sBlend.picks)
    {
      prj::srl::BlendPick bPickOut
      (
	to_string(bPick.id),
        bPick.u,
        bPick.v
      );
      bPicksOut.array().push_back(bPickOut);
    }
    prj::srl::SimpleBlend sBlendOut
    (
      bPicksOut,
      sBlend.radius->serialOut()
    );
    sBlendsOut.array().push_back(sBlendOut);
  }
  
  prj::srl::VariableBlends vBlendsOut;
  for (const auto &vBlend : variableBlends)
  {
    prj::srl::BlendPick bPickOut
    (
      to_string(vBlend.pick.id),
      vBlend.pick.u,
      vBlend.pick.v
    );
    
    prj::srl::VariableEntries vEntriesOut;
    for (const auto vEntry : vBlend.entries)
    {
      prj::srl::VariableEntry vEntryOut
      (
	to_string(vEntry.id),
        vEntry.position->serialOut(),
        vEntry.radius->serialOut()
      );
    }
    
    prj::srl::VariableBlend vBlendOut
    (
      bPickOut,
      vEntriesOut
    );
    vBlendsOut.array().push_back(vBlendOut);
  }
  
  prj::srl::FeatureBlend blendOut
  (
    Base::serialOut(),
    shapeMapOut,
    sBlendsOut,
    vBlendsOut
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::blend(stream, blendOut, infoMap);
}

void Blend::serialRead(const prj::srl::FeatureBlend& sBlendIn)
{
  boost::uuids::string_generator gen;
  
  Base::serialIn(sBlendIn.featureBase());
  
  shapeMap.clear();
  for (const prj::srl::EvolveRecord &sERecord : sBlendIn.shapeMap().evolveRecord())
  {
    std::pair<uuid, uuid> record;
    record.first = gen(sERecord.idIn());
    record.second = gen(sERecord.idOut());
    shapeMap.insert(record);
  }
  
  for (const auto &simpleBlendIn : sBlendIn.simpleBlends().array())
  {
    SimpleBlend simpleBlend;
    for (const auto &bPickIn : simpleBlendIn.blendPicks().array())
    {
      BlendPick pick;
      pick.id = gen(bPickIn.id());
      pick.u = bPickIn.u();
      pick.v = bPickIn.v();
      simpleBlend.picks.push_back(pick);
    }
    simpleBlend.radius = buildRadiusParameter();
    simpleBlend.radius->serialIn(simpleBlendIn.radius());
    addSimpleBlend(simpleBlend);
  }
  
  for (const auto &variableBlendIn : sBlendIn.variableBlends().array())
  {
    VariableBlend vBlend;
    vBlend.pick.id = gen(variableBlendIn.blendPick().id());
    vBlend.pick.u = variableBlendIn.blendPick().u();
    vBlend.pick.v = variableBlendIn.blendPick().v();
    for (const auto &entryIn : variableBlendIn.variableEntries().array())
    {
      VariableEntry entry;
      entry.id = gen(entryIn.id());
      entry.position = buildPositionParameter();
      entry.position->serialIn(entryIn.position());
      entry.radius = buildRadiusParameter();
      entry.radius->serialIn(entryIn.radius());
      vBlend.entries.push_back(entry);
    }
    addVariableBlend(vBlend);
  }
}

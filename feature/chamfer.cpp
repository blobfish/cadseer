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

#include <boost/uuid/string_generator.hpp>

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

#include <project/serial/xsdcxxoutput/featurechamfer.h>
#include <feature/shapeidmapper.h>
#include <feature/shapecheck.h>
#include <feature/chamfer.h>

using namespace ftr;

QIcon Chamfer::icon;

boost::uuids::uuid Chamfer::referenceFaceId(const ResultContainer &cIn, const boost::uuids::uuid &edgeIdIn)
{
  assert(hasResult(cIn, edgeIdIn));
  TopoDS_Shape edgeShape = findResultById(cIn, edgeIdIn).shape;
  assert(edgeShape.ShapeType() == TopAbs_EDGE);
  
  TopTools_IndexedDataMapOfShapeListOfShape eToF;
  TopExp::MapShapesAndAncestors(findRootShape(cIn), TopAbs_EDGE, TopAbs_FACE, eToF);
  TopoDS_Face faceShape = TopoDS::Face(eToF.FindFromKey(edgeShape).First());
  assert(hasResult(cIn, faceShape));
  return findResultByShape(cIn, faceShape).id;
}

std::shared_ptr< Parameter > Chamfer::buildSymParameter()
{
  //some kind of default distance?
  std::shared_ptr<Parameter> out(new Parameter(ParameterNames::Distance, 1.0));
  return out;
}

//duplicated from blend.
double Chamfer::calculateUParameter(const TopoDS_Edge &edgeIn, const osg::Vec3d &point)
{
  TopoDS_Shape aVertex = BRepBuilderAPI_MakeVertex(gp_Pnt(point.x(), point.y(), point.z())).Vertex();
  BRepExtrema_DistShapeShape dist(edgeIn, aVertex);
  if (dist.NbSolution() < 1)
  {
    std::cout << "Warning: no solution for distshapeshape in Chamfer::calculateU" << std::endl;
    return 0.0;
  }
  if (dist.SupportTypeShape1(1) != BRepExtrema_IsOnEdge)
  {
    std::cout << "Warning: wrong support type in Chamfer::calculateU" << std::endl;
    return 0.0;
  }
  
  double closestParameter;
  dist.ParOnEdgeS1(1, closestParameter);
  
  double firstP, lastP;
  BRep_Tool::Curve(edgeIn, firstP, lastP);
  //normalize parameter to 0 to 1.
  return (closestParameter - firstP) / (lastP - firstP);
}

//duplicated from blend.
osg::Vec3d Chamfer::calculateUPoint(const TopoDS_Edge &edgeIn, double parameter)
{
  double firstP, lastP;
  const Handle_Geom_Curve &curve = BRep_Tool::Curve(edgeIn, firstP, lastP);
  double parameterOut = parameter * (lastP - firstP) + firstP;
  gp_Pnt occPoint;
  curve->D0(parameterOut, occPoint);
  return gu::toOsg(occPoint);
}

Chamfer::Chamfer() : Base()
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionChamfer.svg");
  
  name = QObject::tr("Chamfer");
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

void Chamfer::updateModel(const UpdateMap &mapIn)
{
  setFailure();
  try
  {
    if (mapIn.count(InputTypes::target) < 1)
      throw std::runtime_error("no parent for chamfer");
    
    const TopoDS_Shape &targetShape = mapIn.at(InputTypes::target)->getShape();
    const ResultContainer& targetResultContainer = mapIn.at(InputTypes::target)->getResultContainer();
    
    //starting from the stand point that this feature has failed.
    //set the shape and container to the parent target.
    shape = targetShape;
    resultContainer = targetResultContainer;
  
    //this probably temp.
    TopTools_IndexedDataMapOfShapeListOfShape eToF;
    TopExp::MapShapesAndAncestors(targetShape, TopAbs_EDGE, TopAbs_FACE, eToF);
    
    BRepFilletAPI_MakeChamfer chamferMaker(targetShape);
    for (const auto &chamfer : symChamfers)
    {
      assert(chamfer.distance);
      bool labelDone = false; //set label position to first pick.
      for (const auto &pick : chamfer.picks)
      {
	if (!hasResult(targetResultContainer, pick.edgeId))
	{
	  std::cout << "edge id of: " << boost::uuids::to_string(pick.edgeId) << " not found in chamfer" << std::endl;
	  continue;
	}
	if (!hasResult(targetResultContainer, pick.faceId))
	{
	  std::cout << "face id of: " << boost::uuids::to_string(pick.faceId) << " not found in chamfer" << std::endl;
	  continue;
	}
	TopoDS_Edge edge = TopoDS::Edge(findResultById(targetResultContainer, pick.edgeId).shape);
	TopoDS_Face face = TopoDS::Face(findResultById(targetResultContainer, pick.faceId).shape);
	chamferMaker.Add(chamfer.distance->getValue(), edge, face);
	//update location of parameter label.
	if (!labelDone)
	{
	  labelDone = true;
	  chamfer.label->setMatrix(osg::Matrixd::translate(calculateUPoint(TopoDS::Edge(edge), pick.u)));
	}
      }
    }
    chamferMaker.Build();
    if (!chamferMaker.IsDone())
      throw std::runtime_error("chamferMaker failed");
    ShapeCheck check(chamferMaker.Shape());
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");

    shape = chamferMaker.Shape();
    ResultContainer freshContainer = createInitialContainer(shape);
    shapeMatch(targetResultContainer, freshContainer);
    uniqueTypeMatch(targetResultContainer, freshContainer);
    modifiedMatch(chamferMaker, mapIn.at(InputTypes::target)->getResultContainer(), freshContainer);
    generatedMatch(chamferMaker, mapIn.at(InputTypes::target), freshContainer);
//     ensureNoFaceNils(freshContainer);
    outerWireMatch(targetResultContainer, freshContainer);
    derivedMatch(shape, freshContainer, derivedContainer);
    dumpNils(freshContainer, "chamfer feature"); //only if there are shapes with nil ids.
    dumpDuplicates(freshContainer, "chamfer feature");
    ensureNoNils(freshContainer);
    ensureNoDuplicates(freshContainer);
    resultContainer = freshContainer;
    
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

//duplicated code from blend.
void Chamfer::generatedMatch(BRepFilletAPI_MakeChamfer &chamferMakerIn, const Base *targetFeatureIn, ResultContainer &freshContainer)
{
  using boost::uuids::uuid;
  
  TopTools_IndexedMapOfShape targetShapeMap;
  TopExp::MapShapes(targetFeatureIn->getShape(), targetShapeMap);
  
  for (int index = 1; index <= targetShapeMap.Extent(); ++index)
  {
    const TopoDS_Shape &currentShape = targetShapeMap(index);
    TopTools_ListOfShape generated = chamferMakerIn.Generated(currentShape);
    if (generated.IsEmpty())
      continue;
    if(generated.Extent() != 1) //see ensure noFaceNils
      std::cout << "Warning: more than one generated shape in chamfer::generatedMatch" << std::endl;
    const TopoDS_Shape &chamferFace = generated.First();
    assert(!chamferFace.IsNull());
    assert(chamferFace.ShapeType() == TopAbs_FACE);
    
    //targetEdgeId should also be in member edgeIds
    uuid targetEdgeId = findResultByShape(targetFeatureIn->getResultContainer(), currentShape).id;
    uuid chamferedFaceId = boost::uuids::nil_generator()();
    //first time edge has been blended
    if (!hasInId(shapeMap, targetEdgeId))
    {
      //build new record.
      EvolutionRecord record;
      record.inId = targetEdgeId;
      record.outId = idGenerator();
      shapeMap.insert(record);
      
      chamferedFaceId = record.outId;
    }
    else
      chamferedFaceId = findRecordByIn(shapeMap, targetEdgeId).outId;
    
    //now we have the id for the face, just update the result map.
    updateId(freshContainer, chamferedFaceId, chamferFace);
    
    //now look for outerwire for newly generated face.
    uuid chamferedFaceWireId = boost::uuids::nil_generator()();
    if (!hasInId(shapeMap, chamferedFaceId))
    {
      //this means that the face id is in both columns.
      EvolutionRecord record;
      record.inId = chamferedFaceId;
      record.outId = idGenerator();
      shapeMap.insert(record);
      
      chamferedFaceWireId = record.outId;
    }
    else
      chamferedFaceWireId = findRecordByIn(shapeMap, chamferedFaceId).outId;
    
    //now get the wire and update the result to id.
    const TopoDS_Shape &chamferedFaceWire = BRepTools::OuterWire(TopoDS::Face(chamferFace));
    updateId(freshContainer, chamferedFaceWireId, chamferedFaceWire);
  }
}

void Chamfer::serialWrite(const QDir &dIn)
{
  using boost::uuids::to_string;
  
  prj::srl::FeatureChamfer::ShapeMapType shapeMapOut;
  typedef EvolutionContainer::index<EvolutionRecord::ByInId>::type EList;
  const EList &eList = shapeMap.get<EvolutionRecord::ByInId>();
  for (EList::const_iterator it = eList.begin(); it != eList.end(); ++it)
  {
    prj::srl::EvolutionRecord eRecord
    (
      to_string(it->inId),
      to_string(it->outId)
    );
    shapeMapOut.evolutionRecord().push_back(eRecord);
  }
  
  prj::srl::SymChamfers sSymChamfersOut;
  for (const auto &sSymChamfer : symChamfers)
  {
    prj::srl::ChamferPicks cPicksOut;
    for (const auto &cPick : sSymChamfer.picks)
    {
      prj::srl::ChamferPick cPickOut
      (
	to_string(cPick.edgeId),
        cPick.u,
        cPick.v,
        to_string(cPick.faceId)
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
  boost::uuids::string_generator gen;
  
  Base::serialIn(sChamferIn.featureBase());
  
  shapeMap.get<EvolutionRecord::ByInId>().clear();
  for (const prj::srl::EvolutionRecord &sERecord : sChamferIn.shapeMap().evolutionRecord())
  {
    EvolutionRecord record;
    record.inId = gen(sERecord.idIn());
    record.outId = gen(sERecord.idOut());
    shapeMap.insert(record);
  }
  
  for (const auto &symChamferIn : sChamferIn.symChamfers().array())
  {
    SymChamfer symChamfer;
    for (const auto &cPickIn : symChamferIn.chamferPicks().array())
    {
      ChamferPick pick;
      pick.edgeId = gen(cPickIn.edgeId());
      pick.u = cPickIn.u();
      pick.v = cPickIn.v();
      pick.faceId = gen(cPickIn.faceId());
      symChamfer.picks.push_back(pick);
    }
    symChamfer.distance = buildSymParameter();
    symChamfer.distance->serialIn(symChamferIn.distance());
    addSymChamfer(symChamfer);
  }
}

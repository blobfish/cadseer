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

#include <cassert>

#include <gp_Pln.hxx>
#include <BRepOffsetAPI_DraftAngle.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopTools_ListOfShape.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <Geom_Surface.hxx>

#include <globalutilities.h>
#include <tools/idtools.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <feature/shapecheck.h>
#include <library/plabel.h>
#include <project/serial/xsdcxxoutput/featuredraft.h>
#include <annex/seershape.h>
#include <tools/featuretools.h>
#include <feature/updatepayload.h>
#include <feature/parameter.h>
#include <feature/draft.h>

using namespace ftr;
using boost::uuids::uuid;

QIcon Draft::icon;

Draft::Draft() : Base(), sShape(new ann::SeerShape())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionDraft.svg");
  
  name = QObject::tr("Draft");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
}

Draft::~Draft() //for forward declare with osg::ref_ptr
{
}

std::shared_ptr<prm::Parameter> Draft::buildAngleParameter()
{
   //some kind of default angle?
  std::shared_ptr<prm::Parameter> out(new prm::Parameter(prm::Names::Angle, prf::manager().rootPtr->features().draft().get().angle()));
  return out;
}

void Draft::setDraft(const DraftConvey &conveyIn)
{
  targetPicks = conveyIn.targets;
  neutralPick = conveyIn.neutralPlane;
  if (!conveyIn.angle)
    angle = buildAngleParameter();
  angle->connectValue(boost::bind(&Draft::setModelDirty, this));
  if (!conveyIn.label)
    label = new lbr::PLabel(angle.get());
  label->valueHasChanged();
  overlaySwitch->addChild(label.get());
}

void Draft::updateModel(const UpdatePayload &payloadIn)
{
  setFailure();
  lastUpdateLog.clear();
  sShape->reset();
  try
  {
    std::vector<const Base*> features = payloadIn.getFeatures(InputType::target);
    if (features.size() != 1)
      throw std::runtime_error("no parent");
    if (!features.front()->hasAnnex(ann::Type::SeerShape))
      throw std::runtime_error("no seer shape in parent");
    const ann::SeerShape &targetSeerShape = features.front()->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
    if (targetSeerShape.isNull())
      throw std::runtime_error("target seer shape is null");
    
    //setup new failure state.
    sShape->setOCCTShape(targetSeerShape.getRootOCCTShape());
    sShape->shapeMatch(targetSeerShape);
    sShape->uniqueTypeMatch(targetSeerShape);
    sShape->outerWireMatch(targetSeerShape);
    sShape->derivedMatch();
    sShape->ensureNoNils(); //just in case
    sShape->ensureNoDuplicates(); //just in case
    
    //neutral plane might be outside of target, if so we should have an input with a type of 'tool'
//     const TopoDS_Shape &tool; //TODO
    
    //code here
    //not sure exactly how the pull direction affects the operation, but it does.
    //rotation of planar faces happens around intersection of neutral plane and draft face.
    //multiple target faces and 1 neutral plane.
    //if neutral plane comes from another feature then we will have another graph edge with tool input type
    //not going to expose pull direction yet. Not sure how useful it is.
    //of course we will have a reverse direction, but the direction will be derived from the neutral plane.
    
    BRepOffsetAPI_DraftAngle dMaker(targetSeerShape.getRootOCCTShape());
    
    
    auto resolvedNeutralPicks = tls::resolvePicks(features.front(), neutralPick, payloadIn.shapeHistory);
    uuid neutralId = gu::createNilId();
    for (const auto &resolved : resolvedNeutralPicks)
    {
      if (resolved.resultId.is_nil())
        continue;
      neutralId = resolved.resultId;
      break;
    }
    if (neutralId.is_nil())
      throw std::runtime_error("neutral id is nil");
    TopoDS_Shape neutralShape = targetSeerShape.getOCCTShape(neutralId);
    gp_Pln plane = derivePlaneFromShape(neutralShape);
    double localAngle = osg::DegreesToRadians(static_cast<double>(*angle));
    gp_Dir direction = plane.Axis().Direction();
    bool labelDone = false; //set label position to first pick.
    
    auto resolvedTargetPicks = tls::resolvePicks(features, targetPicks, payloadIn.shapeHistory);
    for (const auto &resolved : resolvedTargetPicks)
    {
      if (resolved.resultId.is_nil())
        continue;
      assert(targetSeerShape.hasShapeIdRecord(resolved.resultId)); //project shape history out of sync.
      const TopoDS_Face &face = TopoDS::Face(targetSeerShape.getOCCTShape(resolved.resultId));
      
      dMaker.Add(face, direction, localAngle, plane);
      if (!dMaker.AddDone())
      {
        std::ostringstream s; s << "Draft failed adding face: " << gu::idToString(resolved.resultId) << ". Removing" << std::endl;
        lastUpdateLog += s.str();
        dMaker.Remove(face);
      }
      if (!labelDone)
      {
        labelDone = true;
        label->setMatrix(osg::Matrixd::translate(resolved.pick.getPoint(face)));
      }
    }
    dMaker.Build();
    if (!dMaker.IsDone())
      throw std::runtime_error("draft maker failed");
    ShapeCheck check(dMaker.Shape());
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    sShape->setOCCTShape(dMaker.Shape());
    sShape->shapeMatch(targetSeerShape);
    sShape->uniqueTypeMatch(targetSeerShape);
    sShape->modifiedMatch(dMaker, targetSeerShape);
    generatedMatch(dMaker, targetSeerShape);
    sShape->outerWireMatch(targetSeerShape);
    sShape->derivedMatch();
    sShape->dumpNils("draft feature"); //only if there are shapes with nil ids.
    sShape->dumpDuplicates("draft feature");
    sShape->ensureNoNils();
    sShape->ensureNoDuplicates();
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in draft update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in draft update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in draft update." << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

gp_Pln Draft::derivePlaneFromShape(const TopoDS_Shape &shapeIn)
{
  //just planar surfaces for now.
  BRepAdaptor_Surface sAdapt(TopoDS::Face(shapeIn));
  if (sAdapt.GetType() != GeomAbs_Plane)
    throw std::runtime_error("wrong surface type");
  return sAdapt.Plane();
}

void Draft::generatedMatch(BRepOffsetAPI_DraftAngle &dMaker, const ann::SeerShape &targetShapeIn)
{
  using boost::uuids::uuid;
  std::vector<uuid> targetShapeIds = targetShapeIn.getAllShapeIds();
  
  for (const auto &cId : targetShapeIds)
  {
    const ann::ShapeIdRecord &record = targetShapeIn.findShapeIdRecord(cId);
    const TopTools_ListOfShape &shapes = dMaker.Generated(record.shape);
    if (shapes.Extent() < 1)
      continue;
    assert(shapes.Extent() == 1); //want to know about a situation where we have more than 1.
    uuid freshId;
    if (sShape->hasEvolveRecordIn(cId))
      freshId = sShape->evolve(cId).front();
    else
    {
      freshId = gu::createRandomId();
      sShape->insertEvolve(cId, freshId);
    }
    
    sShape->updateShapeIdRecord(shapes.First(), freshId);
  }
}

void Draft::serialWrite(const QDir &dIn)
{
  prj::srl::Picks targetPicksOut = ::ftr::serialOut(targetPicks);
  prj::srl::Pick neutralPickOut = neutralPick.serialOut();
  prj::srl::FeatureDraft draftOut
  (
    Base::serialOut(),
    targetPicksOut,
    neutralPickOut,
    angle->serialOut(),
    label->serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::draft(stream, draftOut, infoMap);
}

void Draft::serialRead(const prj::srl::FeatureDraft &sDraftIn)
{
  Base::serialIn(sDraftIn.featureBase());
  
  targetPicks = ::ftr::serialIn(sDraftIn.targetPicks());
  neutralPick.serialIn(sDraftIn.neutralPick());
  
  angle = buildAngleParameter();
  angle->serialIn(sDraftIn.angle());
  angle->connectValue(boost::bind(&Draft::setModelDirty, this));
  
  label = new lbr::PLabel(angle.get());
  label->serialIn(sDraftIn.plabel());
  overlaySwitch->addChild(label.get());
}

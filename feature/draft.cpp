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
#include <feature/seershape.h>
#include <feature/draft.h>

using namespace ftr;
using boost::uuids::uuid;

QIcon Draft::icon;

Draft::Draft() : Base()
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionDraft.svg");
  
  name = QObject::tr("Draft");
  mainSwitch->setUserValue(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
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
  try
  {
    if (payloadIn.updateMap.count(InputType::target) != 1)
      throw std::runtime_error("no parent for blend");
    
    const SeerShape &targetSeerShape = payloadIn.updateMap.equal_range(InputType::target).first->second->getSeerShape();
    
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
    
    std::vector<uuid> neutralIds = targetSeerShape.resolvePick(neutralPick.shapeHistory);
    if (neutralIds.empty())
      throw std::runtime_error("couldn't resolve neutral pick");
    assert(neutralIds.size() == 1);
    TopoDS_Shape neutralShape = targetSeerShape.getOCCTShape(neutralIds.front());
    gp_Pln plane = derivePlaneFromShape(neutralShape);
    
    double localAngle = osg::DegreesToRadians(static_cast<double>(*angle));
    gp_Dir direction = plane.Axis().Direction();
    bool labelDone = false; //set label position to first pick.
    for (const auto &p : targetPicks)
    {
      std::vector<uuid> pickIds = targetSeerShape.resolvePick(p.shapeHistory);
      if (pickIds.empty())
      {
        std::cout << "Draft can't resolve id: " << gu::idToString(p.id) << std::endl;
        continue;
      }
      assert(pickIds.size() == 1);
      const TopoDS_Face &face = TopoDS::Face(targetSeerShape.getOCCTShape(pickIds.front()));
      dMaker.Add(face, direction, localAngle, plane);
      if (!dMaker.AddDone())
      {
        std::cout << "Add face to draft failed. Removing" << std::endl;
        dMaker.Remove(face);
      }
      if (!labelDone)
      {
        labelDone = true;
        label->setMatrix(osg::Matrixd::translate(Pick::point(face, p.u, p.v)));
      }
    }
    dMaker.Build();
    if (!dMaker.IsDone())
      throw std::runtime_error("draft maker failed");
    ShapeCheck check(dMaker.Shape());
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    seerShape->setOCCTShape(dMaker.Shape());
    seerShape->shapeMatch(targetSeerShape);
    seerShape->uniqueTypeMatch(targetSeerShape);
    seerShape->modifiedMatch(dMaker, targetSeerShape);
    generatedMatch(dMaker, targetSeerShape);
    seerShape->outerWireMatch(targetSeerShape);
    seerShape->derivedMatch();
    seerShape->dumpNils("draft feature"); //only if there are shapes with nil ids.
    seerShape->dumpDuplicates("draft feature");
    seerShape->ensureNoNils();
    seerShape->ensureNoDuplicates();
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::cout << std::endl << "Error in draft update. " << e.GetMessageString() << std::endl;
  }
  catch (std::exception &e)
  {
    std::cout << std::endl << "Error in draft update. " << e.what() << std::endl;
  }
  setModelClean();
}

gp_Pln Draft::derivePlaneFromShape(const TopoDS_Shape &shapeIn)
{
  //just planar surfaces for now.
  BRepAdaptor_Surface sAdapt(TopoDS::Face(shapeIn));
  return sAdapt.Plane();
}

void Draft::generatedMatch(BRepOffsetAPI_DraftAngle &dMaker, const SeerShape &targetShapeIn)
{
  using boost::uuids::uuid;
  std::vector<uuid> targetShapeIds = targetShapeIn.getAllShapeIds();
  
  for (const auto &cId : targetShapeIds)
  {
    const ShapeIdRecord &record = targetShapeIn.findShapeIdRecord(cId);
    const TopTools_ListOfShape &shapes = dMaker.Generated(record.shape);
    if (shapes.Extent() < 1)
      continue;
    assert(shapes.Extent() == 1); //want to know about a situation where we have more than 1.
    uuid freshId;
    if (seerShape->hasEvolveRecordIn(cId))
      freshId = seerShape->evolve(cId).front();
    else
    {
      freshId = gu::createRandomId();
      seerShape->insertEvolve(cId, freshId);
    }
    
    seerShape->updateShapeIdRecord(shapes.First(), freshId);
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

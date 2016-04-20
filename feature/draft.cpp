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

#include <boost/uuid/string_generator.hpp>

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
#include <feature/shapecheck.h>
#include <library/plabel.h>
#include <project/serial/xsdcxxoutput/featuredraft.h>
#include <feature/seershape.h>
#include <feature/draft.h>

using namespace ftr;

QIcon Draft::icon;

Draft::Draft() : Base()
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionDraft.svg");
  
  name = QObject::tr("Draft");
}

Draft::~Draft() //for forward declare with osg::ref_ptr
{
}

std::shared_ptr<Parameter> Draft::buildAngleParameter()
{
   //some kind of default distance?
  std::shared_ptr<Parameter> out(new Parameter(ParameterNames::Angle, 10.0));
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

void Draft::updateModel(const UpdateMap& mapIn)
{
  setFailure();
  try
  {
    if (mapIn.count(InputTypes::target) < 1)
      throw std::runtime_error("no parent for blend");
    
   const SeerShape &targetSeerShape = mapIn.at(InputTypes::target)->getSeerShape();
    
    //starting from the stand point that this feature has failed.
    //set the shape and container to the parent target.
    seerShape->partialAssign(targetSeerShape);
    
    
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
    
    TopoDS_Shape neutralShape = targetSeerShape.findShapeIdRecord(neutralPick.faceId).shape;
    gp_Pln plane = derivePlaneFromShape(neutralShape);
    
    double localAngle = osg::DegreesToRadians(angle->getValue());
    gp_Dir direction = plane.Axis().Direction();
    bool labelDone = false; //set label position to first pick.
    for (const auto &p : targetPicks)
    {
      if (!targetSeerShape.hasShapeIdRecord(p.faceId))
      {
	std::cout << boost::uuids::to_string(id) << " Draft missing: " << boost::uuids::to_string(p.faceId) << std::endl;
	continue;
      }
      const TopoDS_Face &face = TopoDS::Face(targetSeerShape.findShapeIdRecord(p.faceId).shape);
      dMaker.Add(face, direction, localAngle, plane);
      if (!dMaker.AddDone())
      {
	std::cout << "Add face to draft failed. Removing" << std::endl;
	dMaker.Remove(face);
      }
      if (!labelDone)
      {
	labelDone = true;
	label->setMatrix(osg::Matrixd::translate(calculateUVPoint(face, p.u, p.v)));
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
    seerShape->updateShapeIdRecord(shapes.First(), record.id);
  }
}

void Draft::serialWrite(const QDir &dIn)
{
  using boost::uuids::to_string;
  
  prj::srl::DraftPicks targetPicksOut;
  for (const auto &targetPick : targetPicks)
  {
    prj::srl::DraftPick targetPickOut
    (
      to_string(targetPick.faceId),
      targetPick.u,
      targetPick.v
    );
    targetPicksOut.array().push_back(targetPickOut);
  }
  
  prj::srl::DraftPick neutralPickOut
  (
    to_string(neutralPick.faceId),
    neutralPick.u,
    neutralPick.v
  );
  
  prj::srl::FeatureDraft chamferOut
  (
    Base::serialOut(),
    targetPicksOut,
    neutralPickOut,
    angle->serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::draft(stream, chamferOut, infoMap);
}

void Draft::serialRead(const prj::srl::FeatureDraft &sDraftIn)
{
  boost::uuids::string_generator gen;
  
  Base::serialIn(sDraftIn.featureBase());
  
  for (const auto &targetPickIn : sDraftIn.targetPicks().array())
  {
    DraftPick pick;
    pick.faceId = gen(targetPickIn.faceId());
    pick.u = targetPickIn.u();
    pick.v = targetPickIn.v();
    targetPicks.push_back(pick);
  }
  
  neutralPick.faceId = gen(sDraftIn.neutralPick().faceId());
  neutralPick.u = sDraftIn.neutralPick().u();
  neutralPick.v = sDraftIn.neutralPick().v();
  
  angle = buildAngleParameter();
  angle->serialIn(sDraftIn.angle());
  angle->connectValue(boost::bind(&Draft::setModelDirty, this));
  
  label = new lbr::PLabel(angle.get());
  label->valueHasChanged();
  overlaySwitch->addChild(label.get());
}

void Draft::calculateUVParameter(const TopoDS_Face &faceIn, const osg::Vec3d &pointIn, double &uOut, double &vOut)
{
  TopoDS_Shape aVertex = BRepBuilderAPI_MakeVertex(gp_Pnt(pointIn.x(), pointIn.y(), pointIn.z())).Vertex();
  BRepExtrema_DistShapeShape dist(faceIn, aVertex);
  if (dist.NbSolution() < 1)
  {
    std::cout << "Warning: no solution for distshapeshape in Draft::calculateUV" << std::endl;
    return;
  }
  if (dist.SupportTypeShape1(1) != BRepExtrema_IsInFace)
  {
    std::cout << "Warning: wrong support type in Draft::calculateUV" << std::endl;
    return;
  }
  
  dist.ParOnFaceS1(1, uOut, vOut);
}

osg::Vec3d Draft::calculateUVPoint(const TopoDS_Face &faceIn, double uIn, double vIn)
{
  Handle_Geom_Surface surface = BRep_Tool::Surface(faceIn);
  gp_Pnt occPoint;
  surface->D0(uIn, vIn, occPoint);
  return gu::toOsg(occPoint);
}

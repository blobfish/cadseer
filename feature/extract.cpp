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

#include <TopoDS.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepLib.hxx>

#include <globalutilities.h>
#include <tools/occtools.h>
#include <feature/seershape.h>
#include <feature/shapecheck.h>
#include <feature/extract.h>
#include <project/serial/xsdcxxoutput/featureextract.h>

using namespace ftr;

using boost::uuids::uuid;

QIcon Extract::icon;

Extract::Extract() : Base()
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionExtract.svg");
  
  name = QObject::tr("Extract");
  mainSwitch->setUserValue(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
}

std::shared_ptr<prm::Parameter> Extract::buildAngleParameter(double deg)
{
  std::shared_ptr<prm::Parameter> out(new prm::Parameter(prm::Names::Angle, 0.0));
  //set the value after constraints have been set.
  
  prm::Boundary lower(0.0, prm::Boundary::End::Closed);
  prm::Boundary upper(180.0, prm::Boundary::End::Open);
  prm::Interval interval(lower, upper);
  
  prm::Constraint c;
  c.intervals.push_back(interval);
  out->setConstraint(c);
  
  out->setValue(deg);
  
  return out;
}

void Extract::sync(const Picks &psIn)
{
  picks = psIn;
}

void Extract::sync(const Extract::AccruePicks &apsIn)
{
  //figure out what is new and not.
  std::vector<uuid> ci; //current ids
  std::vector<uuid> ai; //argument ids
  for (const auto &ap : accruePicks)
    ci.push_back(ap.id);
  for (const auto &ap : apsIn)
    ai.push_back(ap.id);
  gu::uniquefy(ci);
  gu::uniquefy(ai);
  
  std::vector<uuid> idsr; //ids to remove
  std::vector<uuid> idsa; //ids to add.
  std::vector<uuid> idsu; //ids to update.
  std::set_difference(ci.begin(), ci.end(), ai.begin(), ai.end(), std::back_inserter(idsr));
  std::set_difference(ai.begin(), ai.end(), ci.begin(), ci.end(), std::back_inserter(idsa));
  std::set_intersection(ai.begin(), ai.end(), ci.begin(), ci.end(), std::back_inserter(idsu));
  
  for (auto &ap : accruePicks)
  {
    if (std::find(idsr.begin(), idsr.end(), ap.id) != idsr.end()) //member of ids to remove.
    {
      //remove label from scenegraph.
      if (ap.label)
      {
        //shouldn't be added to more than one parent, but just in case.
        for (unsigned int i = 0; i < ap.label->getNumParents(); ++i)
        {
          osg::Group *parent = ap.label->getParent(i);
          parent->removeChild(ap.label);
        }
      }
      
      //remove the parameter from the vector.
      auto it = std::find(parameterVector.begin(), parameterVector.end(), ap.parameter.get());
      if (it != parameterVector.end())
        parameterVector.erase(it);
      //shouldn't have to worry about the signal connection between
      //parameter and this feature. parameter does the call back and
      //that will be deleted if gone from apsIn.
    }
    else if (std::find(idsu.begin(), idsu.end(), ap.id) != idsu.end()) //member of ids to update
    {
      for (const auto &apsInPick : apsIn)
      {
        if (apsInPick.id == ap.id)
        {
          ap.picks = apsInPick.picks;
          ap.accrueType = apsInPick.accrueType;
          //we assume parameter and labe are the same.
          assert(ap.parameter == apsInPick.parameter);
          assert(ap.label == apsInPick.label);
          break;
        }
      }
    }
  }
  
  for (auto &ap : apsIn)
  {
    if (std::find(idsa.begin(), idsa.end(), ap.id) != idsa.end()) //member of ids to add.
    {
      accruePicks.push_back(ap);
      auto &b = accruePicks.back();
      if(!b.parameter)
        b.parameter = buildAngleParameter(); //assume angle parameter.
      b.parameter->connectValue(boost::bind(&Extract::setModelDirty, this));
      //else the parameter should already be connected if it exists.
      parameterVector.push_back(b.parameter.get());
      
      if (!b.label)
        b.label = new lbr::PLabel(b.parameter.get());
      b.label->valueHasChanged();
      overlaySwitch->addChild(b.label.get());
    }
  }
}

TopoDS_Shape sew(const occt::FaceVector &faces)
{
  //move to occ tools eventually.
  //all parameters are the default values from occ documentation.
  BRepBuilderAPI_Sewing builder(0.000001, true, true, true, false);
  for (const auto& f : faces)
    builder.Add(f);
  builder.Perform(); //Perform Sewing
//   builder.Dump();
  
  //sewing function is very liberal. Meaning that it really doesn't care if
  //faces are connected or not. It will put a dozen disconnected faces
  //into a compound and return the compound as a result. So we do some post
  //operation analysis to decide what is a success or failure. The criteria
  //for success and failure is up for debate.
  
  //for now we are just looking for a shell.
  TopoDS_Shape out;
  TopoDS_Shape output = builder.SewedShape();
  if (output.IsNull())
    return out;
  
  TopoDS_Shape nc = occt::getFirstNonCompound(output);
  if (nc.IsNull())
    return out;
  
  if (nc.ShapeType() == TopAbs_SHELL)
  {
    out = nc;
    //try to make a solid.
    if (nc.Closed())
    {
      BRepBuilderAPI_MakeSolid solidMaker(TopoDS::Shell(nc));
      if (solidMaker.IsDone())
      {
        TopoDS_Solid temp = solidMaker.Solid();
        //contrary to the occ docs the return value OrientCloseSolid doesn't
        //indicate whether the shell was open or not. It returns true with an
        //open shell and we end up with an invalid solid.
        if (BRepLib::OrientClosedSolid(temp))
          out = temp;
      }
    }
  }
  return out;
}

void Extract::updateModel(const UpdatePayload &payloadIn)
{
  setFailure();
  lastUpdateLog.clear();
  try
  {
    if (payloadIn.updateMap.count(InputType::target) != 1)
      throw std::runtime_error("wrong number of parents for extract");
    
    const SeerShape &targetSeerShape = payloadIn.updateMap.equal_range(InputType::target).first->second->getSeerShape();
    
    occt::FaceVector faces;
    for (const auto &ap : accruePicks)
    {
      bool labelDone = false; //set label position to first pick.
      for (const auto &p : ap.picks)
      {
        std::vector<uuid> resolvedIds = targetSeerShape.resolvePick(p.shapeHistory);
        if (resolvedIds.empty())
        {
          std::ostringstream s; s << "Extract: can't find target id. Skipping id: " << gu::idToString(p.id) << std::endl;
          lastUpdateLog += s.str();
          continue;
        }
        
        for (const auto &resolvedId : resolvedIds)
        {
          TopoDS_Shape shape = targetSeerShape.getOCCTShape(resolvedId);
          assert(!shape.IsNull());
          if (shape.ShapeType() == TopAbs_FACE)
          {
            TopoDS_Face f = TopoDS::Face(shape);
            if (ap.accrueType == AccrueType::Tangent)
              faces = occt::walkTangentFaces(targetSeerShape.getRootOCCTShape(), f, static_cast<double>(*(ap.parameter)));
            else if (ap.accrueType == AccrueType::None)
              faces.push_back(f);
            
            if (!labelDone)
            {
              labelDone = true;
              ap.label->setMatrix(osg::Matrixd::translate(p.getPoint(f)));
            }
          }
        }
      }
    }
    TopoDS_Shape out;
    if (!faces.empty())
      out = sew(faces);
    
    if (out.IsNull())
    {
      occt::ShapeVector sv;
      for (const auto &p : picks)
      {
        std::vector<uuid> resolvedIds = targetSeerShape.resolvePick(p.shapeHistory);
        for (const auto &lid : resolvedIds)
          sv.push_back(targetSeerShape.getOCCTShape(lid));
      }
      out = static_cast<TopoDS_Compound>(occt::ShapeVectorCast(sv));
    }
    
    if (out.IsNull())
      throw std::runtime_error("null shape");
    
    ShapeCheck check(out);
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    //these seem to be taking care of it.
    seerShape->setOCCTShape(out);
    seerShape->shapeMatch(targetSeerShape);
    seerShape->uniqueTypeMatch(targetSeerShape);
    seerShape->outerWireMatch(targetSeerShape);
    seerShape->derivedMatch();
    seerShape->dumpNils("extract feature");
    seerShape->dumpDuplicates("extract feature");
    seerShape->ensureNoNils();
    seerShape->ensureNoDuplicates();
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in extract update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in extract update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in extract update." << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Extract::serialWrite(const QDir &dIn)
{
  prj::srl::AccruePicks aPicksOut;
  for (const auto &ap : accruePicks)
  {
    aPicksOut.array().push_back
    (
      prj::srl::AccruePick
      (
        ::ftr::serialOut(ap.picks),
        accrueMap.left.at(ap.accrueType),
        ap.parameter->serialOut()
      )
    );
  }
  
  prj::srl::Picks ePicksOut = ::ftr::serialOut(picks);
  prj::srl::FeatureExtract extractOut
  (
    Base::serialOut(),
    aPicksOut,
    ePicksOut
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::extract(stream, extractOut, infoMap);
}

void Extract::serialRead(const prj::srl::FeatureExtract &sExtractIn)
{
  Base::serialIn(sExtractIn.featureBase());
  picks = ::ftr::serialIn(sExtractIn.picks());
  AccruePicks aps;
  for (const auto &ap : sExtractIn.accruePicks().array())
  {
    AccruePick c;
    c.picks = ::ftr::serialIn(ap.picks());
    c.accrueType = accrueMap.right.at(ap.accrueType());
    if (c.accrueType == AccrueType::Tangent)
      c.parameter = buildAngleParameter(0.0);
    if (c.parameter)
      c.parameter->serialIn(ap.parameter());
    aps.push_back(c);
  }
  this->sync(aps);
}

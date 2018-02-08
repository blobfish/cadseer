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
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepLib.hxx>
#include <BRepTools_Quilt.hxx>

#include <globalutilities.h>
#include <tools/occtools.h>
#include <annex/seershape.h>
#include <feature/shapecheck.h>
#include <project/serial/xsdcxxoutput/featureextract.h>
#include <feature/updatepayload.h>
#include <tools/featuretools.h>
#include <feature/extract.h>

using namespace ftr;

using boost::uuids::uuid;

QIcon Extract::icon;

Extract::Extract() : Base(), sShape(new ann::SeerShape())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionExtract.svg");
  
  name = QObject::tr("Extract");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
}

Extract::~Extract(){}

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
      auto it = std::find(parameters.begin(), parameters.end(), ap.parameter.get());
      if (it != parameters.end())
        parameters.erase(it);
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
      parameters.push_back(b.parameter.get());
      
      if (!b.label)
        b.label = new lbr::PLabel(b.parameter.get());
      b.label->valueHasChanged();
      overlaySwitch->addChild(b.label.get());
    }
  }
}

void Extract::updateModel(const UpdatePayload &payloadIn)
{
  setFailure();
  lastUpdateLog.clear();
  sShape->reset();
  try
  {
    std::vector<const Base*> targetFeatures = payloadIn.getFeatures(InputType::target);
    if (targetFeatures.size() != 1)
      throw std::runtime_error("wrong number of parents");
    if (!targetFeatures.front()->hasAnnex(ann::Type::SeerShape))
      throw std::runtime_error("parent doesn't have seer shape");
    const ann::SeerShape &targetSeerShape = targetFeatures.front()->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
    if (targetSeerShape.isNull())
      throw std::runtime_error("target seer shape is null");
    
    //no new failure state.
    
    occt::ShapeVector outShapes;
    for (const auto &ap : accruePicks)
    {
      occt::FaceVector faces;
      bool labelDone = false; //set label position to first pick.
      auto resolvedPicks = tls::resolvePicks(targetFeatures, ap.picks, payloadIn.shapeHistory);
      for (const auto &resolved : resolvedPicks)
      {
        if (resolved.resultId.is_nil()) //accrue doesn't work with whole features.
          continue;
        
        TopoDS_Shape shape = targetSeerShape.getOCCTShape(resolved.resultId);
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
            ap.label->setMatrix(osg::Matrixd::translate(resolved.pick.getPoint(f)));
          }
        }
      }
      
      BRepTools_Quilt quilter;
      for (const auto &f : faces)
        quilter.Add(f);
      occt::ShellVector quilts = occt::ShapeVectorCast(TopoDS::Compound(quilter.Shells()));
      for (const auto &s : quilts)
        outShapes.push_back(s);
    }
    
    if (accruePicks.empty())
    {
      auto resolvedPicks = tls::resolvePicks(targetFeatures, picks, payloadIn.shapeHistory);
      for (const auto &resolved : resolvedPicks)
      {
        if (resolved.resultId.is_nil())
        {
          occt::ShapeVector children = targetSeerShape.useGetNonCompoundChildren();
          for (const auto &child : children)
            outShapes.push_back(child);
          continue;
        }
        assert(targetSeerShape.hasShapeIdRecord(resolved.resultId));
        outShapes.push_back(targetSeerShape.getOCCTShape(resolved.resultId));
      }
    }
    TopoDS_Compound out = static_cast<TopoDS_Compound>(occt::ShapeVectorCast(outShapes));
    
    if (out.IsNull())
      throw std::runtime_error("null shape");
    
    ShapeCheck check(out);
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    //these seem to be taking care of it.
    sShape->setOCCTShape(out);
    sShape->shapeMatch(targetSeerShape);
    sShape->uniqueTypeMatch(targetSeerShape);
    sShape->outerWireMatch(targetSeerShape);
    sShape->derivedMatch();
    sShape->dumpNils("extract feature");
    sShape->dumpDuplicates("extract feature");
    sShape->ensureNoNils();
    sShape->ensureNoDuplicates();
    
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
        ap.parameter->serialOut(),
        ap.label->serialOut()
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
    {
      c.parameter = buildAngleParameter(0.0);
      c.label = new lbr::PLabel(c.parameter.get());
    }
    if (c.parameter)
      c.parameter->serialIn(ap.parameter());
    if (c.label)
      c.label->serialIn(ap.plabel());
    
    aps.push_back(c);
  }
  this->sync(aps);
}

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
#include <BRepClass3d.hxx>
#include <BRepLib.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>

#include <globalutilities.h>
#include <tools/occtools.h>
#include <tools/idtools.h>
#include <annex/seershape.h>
#include <project/serial/xsdcxxoutput/featuresew.h>
#include <feature/updatepayload.h>
#include <feature/sew.h>

using namespace ftr;

using boost::uuids::uuid;

QIcon Sew::icon;

Sew::Sew():
Base(),
sShape(new ann::SeerShape())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionSew.svg");
  
  name = QObject::tr("Sew");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
  
  solidId = gu::createRandomId();
  shellId = gu::createRandomId();
}

Sew::~Sew(){}

void Sew::updateModel(const UpdatePayload &payloadIn)
{
  setFailure();
  lastUpdateLog.clear();
  sShape->reset();
  try
  {
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    //we use inputs of type 'target' to apply preference to target feature.
    //we don't value any inputs of a sew more than others so they are all tools.
    std::vector<const Base*> tfs = payloadIn.getFeatures(InputType::tool);
    std::vector<std::reference_wrapper<const ann::SeerShape>> seerShapes;
    //filter out invalid inputs.
    for (const Base *f : tfs)
    {
      assert(f->hasAnnex(ann::Type::SeerShape)); //shouldn't happen.
      if (!f->hasAnnex(ann::Type::SeerShape))
      {
        std::ostringstream stream;
        stream << "WARNING: removing target without seer shape" << std::endl;
        lastUpdateLog += stream.str();
        continue;
      }
      const ann::SeerShape &tss = f->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
      if (tss.isNull())
        continue; //no warning. null seer shapes are normal with skipping used.
      seerShapes.push_back(tss);
    }
    if (seerShapes.empty())
      throw std::runtime_error("no parents");
    
    BRepBuilderAPI_Sewing builder(0.000001, true, true, true, false);
    for (const auto &ss : seerShapes)
    {
      occt::ShapeVector shapes = ss.get().useGetNonCompoundChildren();
      for (const auto &shape : shapes)
      {
        if (shape.ShapeType() == TopAbs_SHELL || shape.ShapeType() == TopAbs_FACE)
          builder.Add(shape);
      }
    }
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
      throw std::runtime_error("output of sew is null");
    
    TopoDS_Shape nc = occt::getFirstNonCompound(output);
    if (nc.IsNull() || nc.ShapeType() != TopAbs_SHELL)
      throw std::runtime_error("can't find acceptable output shape");
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
    
    sShape->setOCCTShape(out);
    for (const auto &ss : seerShapes)
      sShape->shapeMatch(ss);
//     sShape->uniqueTypeMatch(ss);
    assignSolidShell();
    for (const auto &ss : seerShapes)
      sewModifiedMatch(builder, ss);
    for (const auto &ss : seerShapes)
      sShape->outerWireMatch(ss);
    sShape->derivedMatch();
    sShape->dumpNils("sew feature");
    sShape->dumpDuplicates("sew feature");
    sShape->ensureNoNils();
    sShape->ensureNoDuplicates();
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in sew update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in sew update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in sew update. " << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Sew::assignSolidShell()
{
  occt::ShapeVector solids = sShape->useGetChildrenOfType(sShape->getRootOCCTShape(), TopAbs_SOLID);
  if (!solids.empty())
  {
    assert(solids.size() == 1);
    if (solids.size() > 1)
      std::cout << "WARNING: have more than 1 solid in Sew::assignSolidShell" << std::endl;
    sShape->updateShapeIdRecord(solids.front(), solidId);
    if (!sShape->hasEvolveRecordOut(solidId))
      sShape->insertEvolve(gu::createNilId(), solidId);
    TopoDS_Shell shell = BRepClass3d::OuterShell(TopoDS::Solid(solids.front()));
    sShape->updateShapeIdRecord(shell, shellId);
    if (!sShape->hasEvolveRecordOut(shellId))
      sShape->insertEvolve(gu::createNilId(), shellId);
  }
  else
  {
    occt::ShapeVector shells = sShape->useGetChildrenOfType(sShape->getRootOCCTShape(), TopAbs_SHELL);
    if (!shells.empty())
    {
      assert(shells.size() == 1);
      if (shells.size() > 1)
        std::cout << "WARNING: have more than 1 shell in Sew::assignSolidShell" << std::endl;
      sShape->updateShapeIdRecord(shells.front(), shellId);
      if (!sShape->hasEvolveRecordOut(shellId))
        sShape->insertEvolve(gu::createNilId(), shellId);
    }
  }
}

void Sew::sewModifiedMatch(const BRepBuilderAPI_Sewing &builder, const ann::SeerShape &target)
{
  occt::ShapeVector shapes = target.getAllShapes();
  for (const auto &ts : shapes)
  {
    uuid tid = target.findShapeIdRecord(ts).id;
    
    auto goEvolve = [&](const TopoDS_Shape &result)
    {
      if (!sShape->hasShapeIdRecord(result))
      {
        //no warning this is pretty common.
        return;
      }
      
      //make sure the shape has nil for an id. Don't overwrite.
      uuid nid = sShape->findShapeIdRecord(result).id;
      if (!nid.is_nil())
        return;
      
      if (sShape->hasEvolveRecordIn(tid))
        nid = sShape->evolve(tid).front(); //should only be 1 to 1
      else
      {
        nid = gu::createRandomId();
        sShape->insertEvolve(tid, nid);
      }
      sShape->updateShapeIdRecord(result, nid);
    };
    
    //some shapes are in both modified and modifiedSubShape lists.
    if (builder.IsModified(ts))
      goEvolve(builder.Modified(ts));
    else if (builder.IsModifiedSubShape(ts))
      goEvolve(builder.ModifiedSubShape(ts));
  }
}

void Sew::serialWrite(const QDir &dIn)
{
  prj::srl::FeatureSew so
  (
    Base::serialOut(),
    gu::idToString(solidId),
    gu::idToString(shellId)
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::sew(stream, so, infoMap);
}

void Sew::serialRead(const prj::srl::FeatureSew &si)
{
  Base::serialIn(si.featureBase());
  solidId = gu::stringToId(si.solidId());
  shellId = gu::stringToId(si.shellId());
}



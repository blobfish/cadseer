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

#include <BRepAlgoAPI_Defeaturing.hxx>

#include <osg/Switch>

#include <globalutilities.h>
#include <tools/occtools.h>
#include <tools/idtools.h>
#include <tools/featuretools.h>
#include <feature/shapecheck.h>
#include <project/serial/xsdcxxoutput/featureremovefaces.h>
#include <feature/updatepayload.h>
#include <feature/inputtype.h>
#include <annex/seershape.h>
#include <feature/removefaces.h>

using namespace ftr;

using boost::uuids::uuid;

QIcon RemoveFaces::icon;

RemoveFaces::RemoveFaces():
Base(),
sShape(new ann::SeerShape())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionRemoveFaces.svg");
  
  name = QObject::tr("RemoveFaces");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
}

RemoveFaces::~RemoveFaces(){}


void RemoveFaces::setPicks(const Picks &pIn)
{
  picks = pIn;
  setModelDirty();
}

void RemoveFaces::updateModel(const UpdatePayload &payloadIn)
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
    
    std::vector<tls::Resolved> resolved = tls::resolvePicks(tfs, picks, payloadIn.shapeHistory);
    BRepAlgoAPI_Defeaturing algo;
    algo.SetRunParallel(true);
    algo.TrackHistory(true);
    algo.SetShape(tss.getRootOCCTShape());
    for (const auto &r : resolved)
    {
      if (r.resultId.is_nil())
        continue;
      assert(tss.hasShapeIdRecord(r.resultId));
      if (!tss.hasShapeIdRecord(r.resultId))
        continue;
      const TopoDS_Shape &fs = tss.getOCCTShape(r.resultId);
      assert(fs.ShapeType() == TopAbs_FACE);
      if (fs.ShapeType() != TopAbs_FACE)
      {
        std::cerr << "WARNING: shape is not a face in remove face" << std::endl;
        continue;
      }
      algo.AddFaceToRemove(fs);
    }
    algo.Build();
    if (!algo.IsDone())
    {
      std::ostringstream stream;
      algo.DumpErrors(stream);
      throw std::runtime_error(stream.str());
    }
    if (algo.HasWarnings())
    {
      std::ostringstream stream;
      algo.DumpWarnings(stream);
      throw std::runtime_error(stream.str());
    }
    
    sShape->setOCCTShape(algo.Shape());
    sShape->shapeMatch(tss);
    sShape->uniqueTypeMatch(tss);
    sShape->modifiedMatch(algo, tss);
    sShape->outerWireMatch(tss);
    sShape->derivedMatch();
    sShape->dumpNils("remove faces feature");
    sShape->dumpDuplicates("remove faces feature");
    sShape->ensureNoNils();
    sShape->ensureNoDuplicates();
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in remove faces update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in remove faces update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in remove faces update. " << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void RemoveFaces::serialWrite(const QDir &dIn)
{
  prj::srl::FeatureRemoveFaces srf
  (
    Base::serialOut(),
    ftr::serialOut(picks)
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::removeFaces(stream, srf, infoMap);
}

void RemoveFaces::serialRead(const prj::srl::FeatureRemoveFaces &srf)
{
  Base::serialIn(srf.featureBase());
  picks = ftr::serialIn(srf.picks());
}

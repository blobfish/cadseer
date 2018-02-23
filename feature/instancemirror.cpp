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

#include <gp_Ax2.hxx>
#include <TopoDS.hxx>
#include <BRepBuilderAPI_Transform.hxx>

#include <globalutilities.h>
#include <tools/occtools.h>
#include <library/csysdragger.h>
#include <library/plabel.h>
#include <annex/seershape.h>
#include <annex/csysdragger.h>
#include <annex/instancemapper.h>
#include <feature/datumplane.h>
#include <feature/parameter.h>
#include <project/serial/xsdcxxoutput/featureinstancemirror.h>
#include <tools/featuretools.h>
#include <feature/inputtype.h>
#include <feature/updatepayload.h>
#include <feature/instancemirror.h>

using namespace ftr;

using boost::uuids::uuid;

QIcon InstanceMirror::icon;

InstanceMirror::InstanceMirror() :
Base(),
csys(new prm::Parameter(prm::Names::CSys, osg::Matrixd::identity())),
includeSource(new prm::Parameter(QObject::tr("Include Source"), true)),
sShape(new ann::SeerShape()),
iMapper(new ann::InstanceMapper()),
csysDragger(new ann::CSysDragger(this, csys.get()))
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionInstanceMirror.svg");
  
  name = QObject::tr("Instance Mirror");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  csys->connectValue(boost::bind(&InstanceMirror::setModelDirty, this));
  includeSource->connectValue(boost::bind(&InstanceMirror::setModelDirty, this));
  
  csysDragger->dragger->unlinkToMatrix(getMainTransform());
  csysDragger->dragger->hide(lbr::CSysDragger::SwitchIndexes::LinkIcon);
  //don't worry about adding dragger. update takes care of it.
//   overlaySwitch->addChild(csysDragger->dragger);
  
  includeSourceLabel = new lbr::PLabel(includeSource.get());
  includeSourceLabel->showName = true;
  includeSourceLabel->valueHasChanged();
  overlaySwitch->addChild(includeSourceLabel.get());
  
  parameters.push_back(csys.get());
  parameters.push_back(includeSource.get());
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
  annexes.insert(std::make_pair(ann::Type::InstanceMapper, iMapper.get()));
  annexes.insert(std::make_pair(ann::Type::CSysDragger, csysDragger.get()));
}

InstanceMirror::~InstanceMirror()
{

}

void InstanceMirror::setShapePick(const Pick &pIn)
{
  shapePick = pIn;
  setModelDirty();
}

void InstanceMirror::setPlanePick(const Pick &pIn)
{
  planePick = pIn;
  setModelDirty();
}

void InstanceMirror::setCSys(const osg::Matrixd &mIn)
{
  csys->setValue(mIn);
}

bool InstanceMirror::getIncludeSource()
{
  return static_cast<bool>(*includeSource);
}

void InstanceMirror::setIncludeSource(bool in)
{
  includeSource->setValue(in);
}

void InstanceMirror::updateModel(const UpdatePayload &payloadIn)
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
    
    //no new failure state.
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    //get the shapes to mirror.
    occt::ShapeVector tShapes;
    if (shapePick.id.is_nil())
    {
      tShapes = tss.useGetNonCompoundChildren();
    }
    else
    {
      auto resolvedPicks = tls::resolvePicks(tfs.front(), shapePick, payloadIn.shapeHistory);
      for (const auto &resolved : resolvedPicks)
      {
        if (resolved.resultId.is_nil())
          continue;
        assert(tss.hasShapeIdRecord(resolved.resultId));
        if (!tss.hasShapeIdRecord(resolved.resultId))
          continue;
        tShapes.push_back(tss.findShapeIdRecord(resolved.resultId).shape);
      }
    }
    if (tShapes.empty())
      throw std::runtime_error("No shapes found.");
    
    
    //get the plane
    osg::Matrixd workSystem = static_cast<osg::Matrixd>(*csys);
    std::vector<const Base*> mfs = payloadIn.getFeatures(InstanceMirror::mirrorPlane);
    if (mfs.size() == 1)
    {
      //we have a mirror plane selection so make sure the csysdragger is hidden.
      overlaySwitch->removeChild(csysDragger->dragger.get()); //ok if not present.
      
      if (mfs.front()->getType() == Type::DatumPlane)
      {
        const DatumPlane *dp = dynamic_cast<const DatumPlane *>(mfs.front());
        workSystem = dp->getSystem();
      }
      else if (mfs.front()->hasAnnex(ann::Type::SeerShape))
      {
        const ann::SeerShape &mss = mfs.front()->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
        if (planePick.id.is_nil())
          throw std::runtime_error("plane pick id is nil");

        TopoDS_Shape dsShape;
        auto resolvedPicks = tls::resolvePicks(mfs.front(), planePick, payloadIn.shapeHistory);
        for (const auto &resolved : resolvedPicks)
        {
          if (resolved.resultId.is_nil())
            continue;
          assert(mss.hasShapeIdRecord(resolved.resultId));
          if (!mss.hasShapeIdRecord(resolved.resultId))
            continue;
          dsShape = mss.findShapeIdRecord(resolved.resultId).shape;
          break;
        }

        if (dsShape.IsNull())
          throw std::runtime_error("couldn't find occt shape");
        if(dsShape.ShapeType() == TopAbs_FACE)
        {
          osg::Vec3d norm = gu::toOsg(occt::getNormal(TopoDS::Face(dsShape), planePick.u, planePick.v));
          osg::Vec3d origin = planePick.getPoint(TopoDS::Face(dsShape));
          osg::Matrixd cs = static_cast<osg::Matrixd>(*csys); // current system
          osg::Vec3d cz = gu::getZVector(cs);
          if (norm.isNaN())
            throw std::runtime_error("invalid normal from face");
          if ((norm != cz) || (origin != cs.getTrans()))
          {
            workSystem = cs * osg::Matrixd::rotate(cz, norm);
            workSystem.setTrans(origin);
          }
        }
        else
          throw std::runtime_error("unsupported occt shape type");
      }
      else
        throw std::runtime_error("feature doesn't have seer shape");
    }
    else
    {
      //make sure dragger is visible
      if(!overlaySwitch->containsNode(csysDragger->dragger.get()))
        overlaySwitch->addChild(csysDragger->dragger.get());
    }
    
    csys->setValueQuiet(workSystem);
    csysDragger->draggerUpdate(); //keep dragger in sync with parameter.
    
    gp_Trsf mt; //mirror transform.
    mt.SetMirror(gu::toOcc(workSystem));
    BRepBuilderAPI_Transform bt(mt);
    
    occt::ShapeVector out;
    for (const auto &tShape : tShapes)
    {
      if (static_cast<bool>(includeSource))
        out.push_back(tShape);
      bt.Perform(tShape);
      out.push_back(bt.Shape());
    }
    
    TopoDS_Compound result = occt::ShapeVectorCast(out);
    sShape->setOCCTShape(result);
    
    for (const auto &s : tShapes)
    {
      iMapper->startMapping(tss, tss.findShapeIdRecord(s).id,  payloadIn.shapeHistory);
      std::size_t count = 0;
      for (const auto &si : out)
      {
        iMapper->mapIndex(*sShape, si, count);
        count++;
      }
    }
    
    occt::BoundingBox bb(tShapes);
    includeSourceLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(bb.getCenter()) + osg::Vec3d(0.0, 0.0, -bb.getHeight() / 2.0)));
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in instance mirror update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in instance mirror update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in instance mirror update. " << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void InstanceMirror::serialWrite(const QDir &dIn)
{
  prj::srl::FeatureInstanceMirror so
  (
    Base::serialOut(),
    iMapper->serialOut(),
    csysDragger->serialOut(),
    csys->serialOut(),
    includeSource->serialOut(),
    shapePick.serialOut(),
    planePick.serialOut(),
    includeSourceLabel->serialOut(),
    overlaySwitch->containsNode(csysDragger->dragger.get())
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::instanceMirror(stream, so, infoMap);
}

void InstanceMirror::serialRead(const prj::srl::FeatureInstanceMirror &sim)
{
  Base::serialIn(sim.featureBase());
  iMapper->serialIn(sim.instanceMapper());
  csysDragger->serialIn(sim.csysDragger());
  csys->serialIn(sim.csys());
  includeSource->serialIn(sim.includeSource());
  shapePick.serialIn(sim.shapePick());
  planePick.serialIn(sim.planePick());
  includeSourceLabel->serialIn(sim.includeSourceLabel());
  if (sim.draggerVisible())
    overlaySwitch->addChild(csysDragger->dragger.get());
}

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

#include <globalutilities.h>
#include <library/plabel.h>
#include <library/csysdragger.h>
#include <annex/seershape.h>
#include <annex/csysdragger.h>
#include <annex/instancemapper.h>
#include <feature/datumplane.h>
#include <feature/parameter.h>
#include <project/serial/xsdcxxoutput/featureinstancelinear.h>
#include <tools/featuretools.h>
#include <feature/updatepayload.h>
#include <feature/instancelinear.h>

using namespace ftr;

using boost::uuids::uuid;

QIcon InstanceLinear::icon;

InstanceLinear::InstanceLinear() :
Base(),
xOffset(new prm::Parameter(QObject::tr("X Offset"), 20.0)),
yOffset(new prm::Parameter(QObject::tr("Y Offset"), 20.0)),
zOffset(new prm::Parameter(QObject::tr("Z Offset"), 20.0)),
xCount(new prm::Parameter(QObject::tr("X Count"), 2)),
yCount(new prm::Parameter(QObject::tr("Y Count"), 2)),
zCount(new prm::Parameter(QObject::tr("Z Count"), 2)),
csys(new prm::Parameter(prm::Names::CSys, osg::Matrixd::identity())),
includeSource(new prm::Parameter(QObject::tr("Include Source"), true)),
sShape(new ann::SeerShape()),
iMapper(new ann::InstanceMapper()),
csysDragger(new ann::CSysDragger(this, csys.get()))
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionInstanceLinear.svg");
  
  name = QObject::tr("Instance Linear");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  xOffset->setConstraint(prm::Constraint::buildNonZero());
  yOffset->setConstraint(prm::Constraint::buildNonZero());
  zOffset->setConstraint(prm::Constraint::buildNonZero());
  
  xCount->setConstraint(prm::Constraint::buildNonZeroPositive());
  yCount->setConstraint(prm::Constraint::buildNonZeroPositive());
  zCount->setConstraint(prm::Constraint::buildNonZeroPositive());
  
  xOffset->connectValue(boost::bind(&InstanceLinear::setModelDirty, this));
  yOffset->connectValue(boost::bind(&InstanceLinear::setModelDirty, this));
  zOffset->connectValue(boost::bind(&InstanceLinear::setModelDirty, this));
  
  xCount->connectValue(boost::bind(&InstanceLinear::setModelDirty, this));
  yCount->connectValue(boost::bind(&InstanceLinear::setModelDirty, this));
  zCount->connectValue(boost::bind(&InstanceLinear::setModelDirty, this));
  
  csys->connectValue(boost::bind(&InstanceLinear::setModelDirty, this));
  
  includeSource->connectValue(boost::bind(&InstanceLinear::setModelDirty, this));
  
  xOffsetLabel = new lbr::PLabel(xOffset.get());
  xOffsetLabel->showName = true;
  xOffsetLabel->valueHasChanged();
  overlaySwitch->addChild(xOffsetLabel.get());
  yOffsetLabel = new lbr::PLabel(yOffset.get());
  yOffsetLabel->showName = true;
  yOffsetLabel->valueHasChanged();
  overlaySwitch->addChild(yOffsetLabel.get());
  zOffsetLabel = new lbr::PLabel(zOffset.get());
  zOffsetLabel->showName = true;
  zOffsetLabel->valueHasChanged();
  overlaySwitch->addChild(zOffsetLabel.get());
  
  xCountLabel = new lbr::PLabel(xCount.get());
  xCountLabel->showName = true;
  xCountLabel->valueHasChanged();
  overlaySwitch->addChild(xCountLabel.get());
  yCountLabel = new lbr::PLabel(yCount.get());
  yCountLabel->showName = true;
  yCountLabel->valueHasChanged();
  overlaySwitch->addChild(yCountLabel.get());
  zCountLabel = new lbr::PLabel(zCount.get());
  zCountLabel->showName = true;
  zCountLabel->valueHasChanged();
  overlaySwitch->addChild(zCountLabel.get());
  
  //keeping the dragger arrows so use can select and define vector.
  csysDragger->dragger->unlinkToMatrix(getMainTransform());
  csysDragger->dragger->hide(lbr::CSysDragger::SwitchIndexes::LinkIcon);
  csysDragger->dragger->hide(lbr::CSysDragger::SwitchIndexes::Origin);
  overlaySwitch->addChild(csysDragger->dragger);
  
  includeSourceLabel = new lbr::PLabel(includeSource.get());
  includeSourceLabel->showName = true;
  includeSourceLabel->valueHasChanged();
  overlaySwitch->addChild(includeSourceLabel.get());
  
  parameters.push_back(xOffset.get());
  parameters.push_back(yOffset.get());
  parameters.push_back(zOffset.get());
  parameters.push_back(xCount.get());
  parameters.push_back(yCount.get());
  parameters.push_back(zCount.get());
  parameters.push_back(csys.get());
  parameters.push_back(includeSource.get());
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
  annexes.insert(std::make_pair(ann::Type::InstanceMapper, iMapper.get()));
  annexes.insert(std::make_pair(ann::Type::CSysDragger, csysDragger.get()));
}

InstanceLinear::~InstanceLinear() {}

void InstanceLinear::setPick(const Pick &pIn)
{
  pick = pIn;
  setModelDirty();
}

void InstanceLinear::setCSys(const osg::Matrixd &mIn)
{
  csys->setValue(mIn);
  csysDragger->draggerUpdate();
}

bool InstanceLinear::getIncludeSource()
{
  return static_cast<bool>(*includeSource);
}

void InstanceLinear::setIncludeSource(bool in)
{
  includeSource->setValue(in);
}

void InstanceLinear::updateModel(const UpdatePayload &payloadIn)
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
    
    occt::ShapeVector tShapes;
    if (pick.id.is_nil())
    {
      tShapes = tss.useGetNonCompoundChildren();
    }
    else
    {
      auto resolvedPicks = tls::resolvePicks(tfs.front(), pick, payloadIn.shapeHistory);
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
    
    occt::ShapeVector out;
    osg::Vec3d xProjection = gu::getXVector(static_cast<osg::Matrixd>(*csys)) * static_cast<double>(*xOffset);
    osg::Vec3d yProjection = gu::getYVector(static_cast<osg::Matrixd>(*csys)) * static_cast<double>(*yOffset);
    osg::Vec3d zProjection = gu::getZVector(static_cast<osg::Matrixd>(*csys)) * static_cast<double>(*zOffset);
    for (const auto &tShape : tShapes)
    {
      osg::Matrixd shapeBase = gu::toOsg(tShape.Location().Transformation());
      osg::Vec3d baseTrans = shapeBase.getTrans();
      
      for (int z = 0; z < static_cast<int>(*zCount); ++z)
      {
        for (int y = 0; y < static_cast<int>(*yCount); ++y)
        {
          for (int x = 0; x < static_cast<int>(*xCount); ++x)
          {
            if ((x == 0) && (y == 0) && (z == 0) && (!static_cast<bool>(*includeSource)))
              continue;
            osg::Vec3d no = baseTrans; //new origin
            no += zProjection * static_cast<double>(z);
            no += yProjection * static_cast<double>(y);
            no += xProjection * static_cast<double>(x);
            
            gp_Trsf nt = tShape.Location().Transformation(); //new Transformation
            nt.SetTranslationPart(gu::toOcc(no));
            TopLoc_Location loc(nt);
            
            out.push_back(tShape.Located(loc));
          }
        }
      }
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
    
    //update label locations.
    //the origin of the system doesn't matter, so just put at shape center.
    occt::BoundingBox bb(tShapes);
    osg::Vec3d origin = gu::toOsg(bb.getCenter());
    osg::Matrixd tsys = static_cast<osg::Matrixd>(*csys);
    tsys.setTrans(origin);
    csys->setValueQuiet(tsys);
    csysDragger->draggerUpdate();
    
    xOffsetLabel->setMatrix(tsys * osg::Matrixd::translate(xProjection * 0.5));
    yOffsetLabel->setMatrix(tsys * osg::Matrixd::translate(yProjection * 0.5));
    zOffsetLabel->setMatrix(tsys * osg::Matrixd::translate(zProjection * 0.5));
    
    //if we do instancing only on the x axis, y and z count = 1, then
    //the y and z labels overlap at zero. so we cheat.
    auto cheat = [](int c) -> int{return std::max(c - 1, 1);};
    xCountLabel->setMatrix(tsys * osg::Matrixd::translate(xProjection * cheat(static_cast<int>(*xCount))));
    yCountLabel->setMatrix(tsys * osg::Matrixd::translate(yProjection * cheat(static_cast<int>(*yCount))));
    zCountLabel->setMatrix(tsys * osg::Matrixd::translate(zProjection * cheat(static_cast<int>(*zCount))));
    
    includeSourceLabel->setMatrix(osg::Matrixd::translate(origin + osg::Vec3d(0.0, 0.0, -bb.getHeight() / 2.0)));
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in instance linear update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in instance linear update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in instance linear update. " << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void InstanceLinear::serialWrite(const QDir &dIn)
{
  prj::srl::FeatureInstanceLinear so
  (
    Base::serialOut(),
    iMapper->serialOut(),
    csysDragger->serialOut(),
    xOffset->serialOut(),
    yOffset->serialOut(),
    zOffset->serialOut(),
    xCount->serialOut(),
    yCount->serialOut(),
    zCount->serialOut(),
    csys->serialOut(),
    includeSource->serialOut(),
    pick.serialOut(),
    xOffsetLabel->serialOut(),
    yOffsetLabel->serialOut(),
    zOffsetLabel->serialOut(),
    xCountLabel->serialOut(),
    yCountLabel->serialOut(),
    zCountLabel->serialOut(),
    includeSourceLabel->serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::instanceLinear(stream, so, infoMap);
}

void InstanceLinear::serialRead(const prj::srl::FeatureInstanceLinear &sil)
{
  Base::serialIn(sil.featureBase());
  iMapper->serialIn(sil.instanceMapper());
  csysDragger->serialIn(sil.csysDragger());
  xOffset->serialIn(sil.xOffset());
  yOffset->serialIn(sil.yOffset());
  zOffset->serialIn(sil.zOffset());
  xCount->serialIn(sil.xCount());
  yCount->serialIn(sil.yCount());
  zCount->serialIn(sil.zCount());
  csys->serialIn(sil.csys());
  includeSource->serialIn(sil.includeSource());
  pick.serialIn(sil.pick());
  xOffsetLabel->serialIn(sil.xOffsetLabel());
  yOffsetLabel->serialIn(sil.yOffsetLabel());
  zOffsetLabel->serialIn(sil.zOffsetLabel());
  xCountLabel->serialIn(sil.xCountLabel());
  yCountLabel->serialIn(sil.yCountLabel());
  zCountLabel->serialIn(sil.zCountLabel());
  includeSourceLabel->serialIn(sil.includeSourceLabel());
}
    

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
#include <feature/instancemirror.h>

using namespace ftr;

using boost::uuids::uuid;

QIcon InstanceMirror::icon;

InstanceMirror::InstanceMirror() :
Base(),
sShape(new ann::SeerShape()),
iMapper(new ann::InstanceMapper()),
csysDragger(new ann::CSysDragger(this, &csys)),
csys(prm::Names::CSys, osg::Matrixd::identity()),
includeSource(QObject::tr("Include Source"), true)
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionInstanceMirror.svg");
  
  name = QObject::tr("Instance Mirror");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  csys.connectValue(boost::bind(&InstanceMirror::setModelDirty, this));
  includeSource.connectValue(boost::bind(&InstanceMirror::setModelDirty, this));
  
  csysDragger->dragger->unlinkToMatrix(getMainTransform());
  csysDragger->dragger->hide(lbr::CSysDragger::SwitchIndexes::LinkIcon);
  //don't worry about adding dragger. update takes care of it.
//   overlaySwitch->addChild(csysDragger->dragger);
  
  includeSourceLabel = new lbr::PLabel(&includeSource);
  includeSourceLabel->showName = true;
  includeSourceLabel->valueHasChanged();
  overlaySwitch->addChild(includeSourceLabel.get());
  
  parameterVector.push_back(&csys);
  parameterVector.push_back(&includeSource);
  
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
  csys.setValue(mIn);
}

void InstanceMirror::updateModel(const UpdatePayload &payloadIn)
{
  setFailure();
  lastUpdateLog.clear();
  try
  {
    if (payloadIn.updateMap.count(InputType::target) != 1)
      throw std::runtime_error("no parent");
    
    const Base *targetFeature = payloadIn.updateMap.equal_range(InputType::target).first->second;
    if (!targetFeature->hasAnnex(ann::Type::SeerShape))
      throw std::runtime_error("parent doesn't have seer shape.");
    const ann::SeerShape &targetSeerShape = targetFeature->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
    
    
    //get the shapes to mirror.
    occt::ShapeVector tShapes;
    uuid shapeResolvedId = gu::createNilId();
    if (shapePick.id.is_nil())
    {
      tShapes = targetSeerShape.useGetNonCompoundChildren();
    }
    else
    {
      if (targetSeerShape.hasShapeIdRecord(shapePick.id))
        shapeResolvedId = shapePick.id;
      else
      {
        for (const auto &hid : shapePick.shapeHistory.getAllIds())
        {
          if (targetSeerShape.hasShapeIdRecord(hid))
          {
            shapeResolvedId = hid;
            break;
          }
        }
      }
      if (shapeResolvedId.is_nil())
      {
        for (const auto &hid : shapePick.shapeHistory.getAllIds())
        {
          if (!payloadIn.shapeHistory.hasShape(hid))
            continue;
          shapeResolvedId = payloadIn.shapeHistory.devolve(targetFeature->getId(), hid);
          if (shapeResolvedId.is_nil())
            shapeResolvedId = payloadIn.shapeHistory.evolve(targetFeature->getId(), hid);
          if (!shapeResolvedId.is_nil())
            break;
        }
      }
      if (shapeResolvedId.is_nil())
        throw std::runtime_error("can't find shape to instance nil.");
      tShapes.push_back(targetSeerShape.getOCCTShape(shapeResolvedId));
    }
    assert(!tShapes.empty()); //exception should by pass this.
    if (tShapes.empty())
      throw std::runtime_error("WARNING: No shape found.");
    
    
    //get the plane
    osg::Matrixd workSystem = static_cast<osg::Matrixd>(csys);
    if (payloadIn.updateMap.count(InstanceMirror::mirrorPlane) == 1)
    {
      //we have a mirror plane selection so make sure the csysdragger is hidden.
      overlaySwitch->removeChild(csysDragger->dragger.get()); //ok if not present.
      
      const Base *mirrorFeature = payloadIn.updateMap.equal_range(InstanceMirror::mirrorPlane).first->second;
      if (mirrorFeature->getType() == Type::DatumPlane)
      {
        const DatumPlane *dp = dynamic_cast<const DatumPlane *>(mirrorFeature);
        workSystem = dp->getSystem();
      }
      else if (mirrorFeature->hasAnnex(ann::Type::SeerShape))
      {
        const ann::SeerShape &mss = mirrorFeature->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
        if (planePick.id.is_nil())
          throw std::runtime_error("plane pick id is nil");

        TopoDS_Shape dsShape;
        std::vector<uuid> pickIds = planePick.shapeHistory.getAllIds();
        assert(pickIds.front() == planePick.id); //double check.
        for (const auto &pid : pickIds) //test all the pick ids against feature first.
        {
          if (mss.hasShapeIdRecord(pid))
          {
            dsShape = mss.getOCCTShape(pid);
            break;
          }
        }
        if (dsShape.IsNull())
        {
          for (const auto &pid : pickIds) // now use picks against current history.
          {
            uuid tid = payloadIn.shapeHistory.devolve(mirrorFeature->getId(), pid);
            if (tid.is_nil())
              tid = payloadIn.shapeHistory.evolve(mirrorFeature->getId(), pid);
            if (tid.is_nil())
              continue;
            dsShape = mss.getOCCTShape(tid);
            break;
          }
        }
        if (dsShape.IsNull())
          throw std::runtime_error("couldn't find occt shape");
        if(dsShape.ShapeType() == TopAbs_FACE)
        {
          osg::Vec3d norm = gu::toOsg(occt::getNormal(TopoDS::Face(dsShape), planePick.u, planePick.v));
          osg::Vec3d origin = planePick.getPoint(TopoDS::Face(dsShape));
          osg::Matrixd cs = static_cast<osg::Matrixd>(csys); // current system
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
    
    csys.setValueQuiet(workSystem);
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
    iMapper->startMapping(targetSeerShape, shapeResolvedId,  payloadIn.shapeHistory);
    std::size_t count = 0;
    for (const auto &si : out)
    {
      iMapper->mapIndex(*sShape, si, count);
      count++;
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
    csys.serialOut(),
    includeSource.serialOut(),
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
  csys.serialIn(sim.csys());
  includeSource.serialIn(sim.includeSource());
  shapePick.serialIn(sim.shapePick());
  planePick.serialIn(sim.planePick());
  includeSourceLabel->serialIn(sim.includeSourceLabel());
  if (sim.draggerVisible())
    overlaySwitch->addChild(csysDragger->dragger.get());
}

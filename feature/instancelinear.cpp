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
#include <feature/instancelinear.h>

using namespace ftr;

using boost::uuids::uuid;

QIcon InstanceLinear::icon;

InstanceLinear::InstanceLinear() :
Base(),
sShape(new ann::SeerShape()),
iMapper(new ann::InstanceMapper()),
csysDragger(new ann::CSysDragger(this, &csys)),
xOffset(QObject::tr("X Offset"), 20.0),
yOffset(QObject::tr("Y Offset"), 20.0),
zOffset(QObject::tr("Z Offset"), 20.0),
xCount(QObject::tr("X Count"), 2),
yCount(QObject::tr("Y Count"), 2),
zCount(QObject::tr("Z Count"), 2),
csys(prm::Names::CSys, osg::Matrixd::identity())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionInstance.svg");
  
  name = QObject::tr("Instance");
  mainSwitch->setUserValue(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  xOffset.setConstraint(prm::Constraint::buildNonZero());
  yOffset.setConstraint(prm::Constraint::buildNonZero());
  zOffset.setConstraint(prm::Constraint::buildNonZero());
  
  xCount.setConstraint(prm::Constraint::buildNonZeroPositive());
  yCount.setConstraint(prm::Constraint::buildNonZeroPositive());
  zCount.setConstraint(prm::Constraint::buildNonZeroPositive());
  
  xOffset.connectValue(boost::bind(&InstanceLinear::setModelDirty, this));
  yOffset.connectValue(boost::bind(&InstanceLinear::setModelDirty, this));
  zOffset.connectValue(boost::bind(&InstanceLinear::setModelDirty, this));
  
  xCount.connectValue(boost::bind(&InstanceLinear::setModelDirty, this));
  yCount.connectValue(boost::bind(&InstanceLinear::setModelDirty, this));
  zCount.connectValue(boost::bind(&InstanceLinear::setModelDirty, this));
  
  csys.connectValue(boost::bind(&InstanceLinear::setModelDirty, this));
  
  xOffsetLabel = new lbr::PLabel(&xOffset);
  xOffsetLabel->showName = true;
  xOffsetLabel->valueHasChanged();
  overlaySwitch->addChild(xOffsetLabel.get());
  yOffsetLabel = new lbr::PLabel(&yOffset);
  yOffsetLabel->showName = true;
  yOffsetLabel->valueHasChanged();
  overlaySwitch->addChild(yOffsetLabel.get());
  zOffsetLabel = new lbr::PLabel(&zOffset);
  zOffsetLabel->showName = true;
  zOffsetLabel->valueHasChanged();
  overlaySwitch->addChild(zOffsetLabel.get());
  
  xCountLabel = new lbr::PLabel(&xCount);
  xCountLabel->showName = true;
  xCountLabel->valueHasChanged();
  overlaySwitch->addChild(xCountLabel.get());
  yCountLabel = new lbr::PLabel(&yCount);
  yCountLabel->showName = true;
  yCountLabel->valueHasChanged();
  overlaySwitch->addChild(yCountLabel.get());
  zCountLabel = new lbr::PLabel(&zCount);
  zCountLabel->showName = true;
  zCountLabel->valueHasChanged();
  overlaySwitch->addChild(zCountLabel.get());
  
  //keeping the dragger arrows so use can select and define vector.
  csysDragger->dragger->unlinkToMatrix(getMainTransform());
  csysDragger->dragger->hide(lbr::CSysDragger::SwitchIndexes::LinkIcon);
  csysDragger->dragger->hide(lbr::CSysDragger::SwitchIndexes::Origin);
  overlaySwitch->addChild(csysDragger->dragger);
  
  parameterVector.push_back(&xOffset);
  parameterVector.push_back(&yOffset);
  parameterVector.push_back(&zOffset);
  parameterVector.push_back(&xCount);
  parameterVector.push_back(&yCount);
  parameterVector.push_back(&zCount);
  parameterVector.push_back(&csys);
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
  annexes.insert(std::make_pair(ann::Type::InstanceMapper, iMapper.get()));
  annexes.insert(std::make_pair(ann::Type::CSysDragger, csysDragger.get()));
}

InstanceLinear::~InstanceLinear() {}

/*
void InstanceLinear::setGenre(std::unique_ptr<InstanceGenre> gIn)
{
  if (genre)
  {
    std::vector<prm::Parameter*> ops = genre->getParameters(); //old parameters
    for (const auto &op : ops)
    {
      auto it = std::find(parameterVector.begin(), parameterVector.end(), op);
      if (it != parameterVector.end())
        parameterVector.erase(it);
      //shouldn't need to disconnect, as the parameter calls the feature and the parameter will be deleted.
    }
    
    std::vector<osg::Node*> oogs = genre->getOverlayGeometry(); //old overlay geometries
    for (const auto &oog : oogs)
      overlaySwitch->removeChild(oog);
    
    std::vector<ann::Base*> anxs = genre->getAnnexes();
    for (const auto &a : anxs)
    {
      //this code assumes only one type of annex per feature.
      auto it = annexes.find(a->getType());
      if (it != annexes.end())
        annexes.erase(it);
    }
  }
  
  genre = std::move(gIn);
  std::vector<prm::Parameter*> nps = genre->getParameters(); //new parameters
  for (const auto &np : nps)
  {
    np->connectValue(boost::bind(&Base::setModelDirty, this));
    parameterVector.push_back(np);
  }
  
  std::vector<osg::Node*> nogs = genre->getOverlayGeometry(); //new overlay geometries
  for (const auto &nog : nogs)
    overlaySwitch->addChild(nog);
  
  std::vector<ann::Base*> anxs = genre->getAnnexes();
  for (const auto &a : anxs)
    annexes.insert(std::make_pair(a->getType(), a));
}
*/

void InstanceLinear::setPick(const Pick &pIn)
{
  pick = pIn;
  setModelDirty();
}

void InstanceLinear::updateModel(const UpdatePayload &payloadIn)
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
    
    occt::ShapeVector tShapes;
    uuid resolvedId = gu::createNilId();
    if (pick.id.is_nil())
    {
      tShapes = targetSeerShape.useGetNonCompoundChildren();
    }
    else
    {
      if (targetSeerShape.hasShapeIdRecord(pick.id))
        resolvedId = pick.id;
      else
      {
        for (const auto &hid : pick.shapeHistory.getAllIds())
        {
          if (targetSeerShape.hasShapeIdRecord(hid))
          {
            resolvedId = hid;
            break;
          }
        }
      }
      if (resolvedId.is_nil())
      {
        for (const auto &hid : pick.shapeHistory.getAllIds())
        {
          if (!payloadIn.shapeHistory.hasShape(hid))
            continue;
          resolvedId = payloadIn.shapeHistory.devolve(targetFeature->getId(), hid);
          if (resolvedId.is_nil())
            resolvedId = payloadIn.shapeHistory.evolve(targetFeature->getId(), hid);
          if (!resolvedId.is_nil())
            break;
        }
      }
      if (resolvedId.is_nil())
        throw std::runtime_error("can't find shape to instance nil.");
      tShapes.push_back(targetSeerShape.getOCCTShape(resolvedId));
    }
    assert(!tShapes.empty()); //exception should by pass this.
    if (tShapes.empty())
      throw std::runtime_error("WARNING: No shape found.");
    
    occt::ShapeVector out;
    osg::Vec3d xProjection = gu::getXVector(static_cast<osg::Matrixd>(csys)) * static_cast<double>(xOffset);
    osg::Vec3d yProjection = gu::getYVector(static_cast<osg::Matrixd>(csys)) * static_cast<double>(yOffset);
    osg::Vec3d zProjection = gu::getZVector(static_cast<osg::Matrixd>(csys)) * static_cast<double>(zOffset);
    for (const auto &tShape : tShapes)
    {
      osg::Matrixd shapeBase = gu::toOsg(tShape.Location().Transformation());
      osg::Vec3d baseTrans = shapeBase.getTrans();
      
      
      for (int z = 0; z < static_cast<int>(zCount); ++z)
      {
        for (int y = 0; y < static_cast<int>(yCount); ++y)
        {
          for (int x = 0; x < static_cast<int>(xCount); ++x)
          {
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
    iMapper->startMapping(targetSeerShape, resolvedId,  payloadIn.shapeHistory);
    std::size_t count = 0;
    for (const auto &si : out)
    {
      iMapper->mapIndex(*sShape, si, count);
      count++;
    }
    
    //update label locations.
    //the origin of the system doesn't matter, so just put at shape center.
    occt::BoundingBox bb(tShapes);
    osg::Vec3d origin = gu::toOsg(bb.getCenter());
    osg::Matrixd tsys = static_cast<osg::Matrixd>(csys);
    tsys.setTrans(origin);
    csys.setValueQuiet(tsys);
    csysDragger->draggerUpdate();
    
    xOffsetLabel->setMatrix(tsys * osg::Matrixd::translate(xProjection * 0.5));
    yOffsetLabel->setMatrix(tsys * osg::Matrixd::translate(yProjection * 0.5));
    zOffsetLabel->setMatrix(tsys * osg::Matrixd::translate(zProjection * 0.5));
    
    //if we do instancing only on the x axis, y and z count = 1, then
    //the y and z labels overlap at zero. so we cheat.
    auto cheat = [](int c) -> int{return std::max(c - 1, 1);};
    xCountLabel->setMatrix(tsys * osg::Matrixd::translate(xProjection * cheat(static_cast<int>(xCount))));
    yCountLabel->setMatrix(tsys * osg::Matrixd::translate(yProjection * cheat(static_cast<int>(yCount))));
    zCountLabel->setMatrix(tsys * osg::Matrixd::translate(zProjection * cheat(static_cast<int>(zCount))));
    
    
    
    
    
    
    
    /*
    if (genre->getType() == InstanceType::Mirror)
    {
      InstanceMirror *im = dynamic_cast<InstanceMirror*>(genre.get());
      assert(im);
      osg::Matrixd workSystem = static_cast<osg::Matrixd>(im->csys);
      if (payloadIn.updateMap.count(InstanceMirror::mirrorPlane) == 1)
      {
        const Base *mirrorFeature = payloadIn.updateMap.equal_range(InstanceMirror::mirrorPlane).first->second;
        if (mirrorFeature->getType() == Type::DatumPlane)
        {
          const DatumPlane *dp = dynamic_cast<const DatumPlane *>(mirrorFeature);
          workSystem = dp->getSystem();
        }
        else if (mirrorFeature->hasAnnex(ann::Type::SeerShape))
        {
          const ann::SeerShape &mss = mirrorFeature->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
          if (im->pick.id.is_nil())
            throw std::runtime_error("pick id is nil for mirror");

          TopoDS_Shape dsShape;
          std::vector<uuid> pickIds = pick.shapeHistory.getAllIds();
          assert(pickIds.front() == pick.id); //double check.
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
            osg::Vec3d norm = gu::toOsg(occt::getNormal(TopoDS::Face(dsShape), im->pick.u, im->pick.v));
            osg::Matrixd cs = static_cast<osg::Matrixd>(im->csys); // current system
            osg::Vec3d cz = gu::getZVector(cs);
            if (norm.isNaN())
              throw std::runtime_error("invalid normal from face");
            if ((norm != cz))
              workSystem = cs * osg::Matrixd::rotate(cz, norm);
          }
          else
            throw std::runtime_error("unsupported occt shape type");
        }
        else
          throw std::runtime_error("feature doesn't have seer shape");
      }
      
      im->csys.setValueQuiet(workSystem);
      im->csysDragger->draggerUpdate();
    }
    */
    
    
    

    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in instance update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in instance update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in instance update. " << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void InstanceLinear::serialWrite(const QDir &dIn)
{
}

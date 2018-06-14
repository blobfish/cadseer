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

#include <gp_Ax3.hxx>
#include <BRepPrimAPI_MakeTorus.hxx>

#include <osg/Switch>

#include <globalutilities.h>
#include <tools/idtools.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <library/lineardimension.h>
#include <library/ipgroup.h>
#include <library/csysdragger.h>
#include <annex/seershape.h>
#include <annex/csysdragger.h>
#include <project/serial/xsdcxxoutput/featuretorus.h>
#include <feature/parameter.h>
#include <feature/torus.h>

using namespace ftr;

using boost::uuids::uuid;

QIcon Torus::icon;

Torus::Torus():
Base(),
radius1(new prm::Parameter(prm::Names::Radius1, prf::manager().rootPtr->features().torus().get().radius1())),
radius2(new prm::Parameter(prm::Names::Radius2, prf::manager().rootPtr->features().torus().get().radius2())),
csys(new prm::Parameter(prm::Names::CSys, osg::Matrixd::identity())),
csysDragger(new ann::CSysDragger(this, csys.get())),
sShape(new ann::SeerShape())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionTorus.svg");
  
  name = QObject::tr("Torus");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  initializeMaps();
  
  radius1->setConstraint(prm::Constraint::buildZeroPositive());
  radius2->setConstraint(prm::Constraint::buildZeroPositive());
  
  parameters.push_back(radius1.get());
  parameters.push_back(radius2.get());
  parameters.push_back(csys.get());
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
  annexes.insert(std::make_pair(ann::Type::CSysDragger, csysDragger.get()));
  overlaySwitch->addChild(csysDragger->dragger);
  
  radius1->connectValue(boost::bind(&Torus::setModelDirty, this));
  radius2->connectValue(boost::bind(&Torus::setModelDirty, this));
  csys->connectValue(boost::bind(&Torus::setModelDirty, this));
  
  setupIPGroup();
}

Torus::~Torus(){}



void Torus::setRadius1(const double& radius1In)
{
  radius1->setValue(radius1In);
}

void Torus::setRadius2(const double& radius2In)
{
  radius2->setValue(radius2In);
}

void Torus::setCSys(const osg::Matrixd &csysIn)
{
  osg::Matrixd oldSystem = static_cast<osg::Matrixd>(*csys);
  if (!csys->setValue(csysIn))
    return; // already at this csys
    
  //apply the same transformation to dragger, so dragger moves with it.
  osg::Matrixd diffMatrix = osg::Matrixd::inverse(oldSystem) * csysIn;
  csysDragger->draggerUpdate(csysDragger->dragger->getMatrix() * diffMatrix);
}

double Torus::getRadius1() const
{
  return static_cast<double>(*radius1);
}

double Torus::getRadius2() const
{
  return static_cast<double>(*radius2);
}

osg::Matrixd Torus::getCSys() const
{
  return static_cast<osg::Matrixd>(*csys);
}

void Torus::setupIPGroup()
{
  radius1IP = new lbr::IPGroup(radius1.get());
  radius1IP->setMatrixDims(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(0.0, 0.0, -1.0)));
  radius1IP->setMatrixDragger(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(0.0, 1.0, 0.0)));
  radius1IP->setRotationAxis(osg::Vec3d(1.0, 0.0, 0.0), osg::Vec3d(0.0, 0.0, 1.0));
  overlaySwitch->addChild(radius1IP.get());
  csysDragger->dragger->linkToMatrix(radius1IP.get());
  
  radius2IP = new lbr::IPGroup(radius2.get());
  radius2IP->setMatrixDragger(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(-1.0, 0.0, 0.0)));
  radius2IP->setRotationAxis(osg::Vec3d(0.0, 1.0, 0.0), osg::Vec3d(1.0, 0.0, 1.0));
  overlaySwitch->addChild(radius2IP.get());
  csysDragger->dragger->linkToMatrix(radius2IP.get());
  
  updateIPGroup();
}

void Torus::updateIPGroup()
{
  radius1IP->setMatrix(static_cast<osg::Matrixd>(*csys));
  
  osg::Matrixd dm; //dragger matrix
  dm.setRotate(osg::Quat(osg::PI_2, osg::Vec3d(-1.0, 0.0, 0.0)));
  dm.setTrans(osg::Vec3d (0.0, static_cast<double>(*radius1), 0.0));
  radius2IP->setMatrixDragger(dm);
  radius2IP->setMatrixDims(osg::Matrixd::translate(osg::Vec3d(0.0, static_cast<double>(*radius1), 0.0)));
  radius2IP->setMatrix(static_cast<osg::Matrixd>(*csys));
  
  radius1IP->valueHasChanged();
  radius1IP->constantHasChanged();
  radius2IP->valueHasChanged();
  radius2IP->constantHasChanged();
}

void Torus::initializeMaps()
{
  //result 
  for (unsigned int index = 0; index < 8; ++index)
  {
    uuid tempId = gu::createRandomId();
    offsetIds.push_back(tempId);
    
    ann::EvolveRecord evolveRecord;
    evolveRecord.outId = tempId;
    sShape->insertEvolve(evolveRecord);
  }
}

void Torus::updateModel(const UpdatePayload&)
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
    
    if (!(static_cast<double>(*radius1) > static_cast<double>(*radius2)))
      throw std::runtime_error("radius1 must be bigger than radius2");
    
    BRepPrimAPI_MakeTorus tMaker(static_cast<double>(*radius1), static_cast<double>(*radius2));
    tMaker.Build();
    assert(tMaker.IsDone());
    if (!tMaker.IsDone())
      throw std::runtime_error("BRepPrimAPI_MakeTorus failed");
    
    TopoDS_Shape out = tMaker.Shape();
    gp_Trsf nt; //new transformation
    nt.SetTransformation(gp_Ax3(gu::toOcc(static_cast<osg::Matrixd>(*csys))));
    nt.Invert();
    TopLoc_Location nl(nt); //new location
    out.Location(nt);
    sShape->setOCCTShape(out);
    
    updateResult();
    
    mainTransform->setMatrix(osg::Matrixd::identity());
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in torus update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in torus update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in torus update. " << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  updateIPGroup();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Torus::updateResult()
{
  auto sv = sShape->getAllShapes();
  assert(sv.size() == 8);
  std::size_t i = 0;
  for (const auto &s : sv)
  {
    sShape->updateShapeIdRecord(s, offsetIds.at(i));
    i++; 
  }
  sShape->setRootShapeId(offsetIds.at(0));
}

void Torus::serialWrite(const QDir &dIn)
{
  prj::srl::OffsetIds oids; //offset ids.
  for (const auto &idOut : offsetIds)
    oids.id().push_back(gu::idToString(idOut));
  
  prj::srl::FeatureTorus to //torus out.
  (
    Base::serialOut(),
    radius1->serialOut(),
    radius2->serialOut(),
    csys->serialOut(),
    csysDragger->serialOut(),
    oids
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::torus(stream, to, infoMap);
}

void Torus::serialRead(const prj::srl::FeatureTorus &ti)
{
  Base::serialIn(ti.featureBase());
  radius1->serialIn(ti.radius1());
  radius2->serialIn(ti.radius2());
  csys->serialIn(ti.csys());
  csysDragger->serialIn(ti.csysDragger());
  
  for (const auto &idIn : ti.offsetIds().id())
    offsetIds.push_back(gu::stringToId(idIn));
  
  updateIPGroup();
}

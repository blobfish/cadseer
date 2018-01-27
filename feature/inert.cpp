/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2015  Thomas S. Anderson blobfish.at.gmx.com
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

#include <QDir>

#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <gp_Ax3.hxx>
#include <gp_Trsf.hxx>
#include <TopLoc_Location.hxx>

#include <project/serial/xsdcxxoutput/featureinert.h>
#include <globalutilities.h>
#include <tools/occtools.h>
#include <library/csysdragger.h>
#include <annex/seershape.h>
#include <annex/csysdragger.h>
#include <feature/parameter.h>
#include <feature/updatepayload.h>
#include <feature/inert.h>

using namespace ftr;
using namespace boost::uuids;

QIcon Inert::icon;

Inert::Inert(const TopoDS_Shape &shapeIn) :
Base(),
csys(new prm::Parameter(prm::Names::CSys, osg::Matrixd::identity())),
csysDragger(new ann::CSysDragger(this, csys.get())),
sShape(new ann::SeerShape())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionInert.svg");
  
  name = QObject::tr("Inert");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  sShape->setOCCTShape(shapeIn);
  sShape->ensureNoNils();
  
  parameters.push_back(csys.get());
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
  annexes.insert(std::make_pair(ann::Type::CSysDragger, csysDragger.get()));
  overlaySwitch->addChild(csysDragger->dragger);
  
  //sync transformations.
  csys->setValue(gu::toOsg(sShape->getRootOCCTShape().Location().Transformation()));
  csysDragger->draggerUpdate(); //set dragger to parameter.
  
  csys->connectValue(boost::bind(&Inert::setModelDirty, this));
}

Inert::~Inert(){}


void Inert::setCSys(const osg::Matrixd &mIn)
{
  csys->setValue(mIn);
}

osg::Matrixd Inert::getCSys() const
{
  return static_cast<osg::Matrixd>(*csys);
}

void Inert::updateModel(const UpdatePayload&)
{
  try
  {
    setFailure();
    lastUpdateLog.clear();
    
    //store a map of offset to id for restoration.
    std::vector<uuid> oldIds;
    occt::ShapeVector shapes = occt::mapShapes(sShape->getRootOCCTShape());
    for (const auto &s : shapes)
    {
      if (sShape->hasShapeIdRecord(s))
      {
        oldIds.push_back(sShape->findShapeIdRecord(s).id);
      }
      else
      {
        std::cerr << "WARNING: shape is not in shapeId container in Inert::updateModel" << std::endl;
        oldIds.push_back(gu::createNilId()); //place holder will be skipped again.
      }
    }
    uuid oldRootId = sShape->getRootShapeId();
    
    TopoDS_Shape tempShape(sShape->getRootOCCTShape());
    gp_Ax3 tempAx3(gu::toOcc(osg::Matrixd::inverse(static_cast<osg::Matrixd>(*csys))));
    gp_Trsf tempTrsf;
    tempTrsf.SetTransformation(tempAx3);
    TopLoc_Location freshLocation(tempTrsf);
    tempShape.Location(freshLocation);
    sShape->setOCCTShape(tempShape);
    
    sShape->updateShapeIdRecord(sShape->getRootOCCTShape(), oldRootId);
    sShape->setRootShapeId(oldRootId);
    
    shapes = occt::mapShapes(sShape->getRootOCCTShape());
    std::size_t count = 0;
    for (const auto &s : shapes)
    {
      if (sShape->hasShapeIdRecord(s))
      {
        sShape->updateShapeIdRecord(s, oldIds.at(count));
      }
      else
      {
        std::cerr << "WARNING: shape is not in moved shapeId container in Inert::updateModel" << std::endl;
      }
      count++;
    }
    
    sShape->dumpNils("inert");
    sShape->dumpDuplicates("inert");
    
    sShape->ensureNoNils();
    sShape->ensureNoDuplicates();
    
    mainTransform->setMatrix(osg::Matrixd::identity());
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in inert update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in inert update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in inert update." << std::endl;
    lastUpdateLog += s.str();
  }
  
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Inert::serialWrite(const QDir &dIn)
{
  prj::srl::FeatureInert inertOut
  (
    Base::serialOut(),
    csys->serialOut(),
    csysDragger->serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::inert(stream, inertOut, infoMap);
}

void Inert::serialRead(const prj::srl::FeatureInert& inert)
{
  Base::serialIn(inert.featureBase());
  csys->serialIn(inert.csys());
  csysDragger->serialIn(inert.csysDragger());
}

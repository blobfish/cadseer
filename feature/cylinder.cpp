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

#include <assert.h>

#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <globalutilities.h>
#include <tools/idtools.h>
#include <library/lineardimension.h>
#include <library/ipgroup.h>
#include <library/csysdragger.h>
#include <project/serial/xsdcxxoutput/featurecylinder.h>
#include <annex/csysdragger.h>
#include <feature/cylinderbuilder.h>
#include <feature/seershape.h>
#include <feature/cylinder.h>

using namespace ftr;
using boost::uuids::uuid;

enum class FeatureTag
{
  Root,         //!< compound
  Solid,        //!< solid
  Shell,        //!< shell
  FaceBottom,   //!< bottom of cone
  FaceCylindrical,  //!< conical face
  FaceTop,      //!< might be empty
  WireBottom,   //!< wire on base face
  WireCylindrical,  //!< wire along conical face
  WireTop,      //!< wire along top
  EdgeBottom,   //!< bottom edge.
  EdgeCylindrical,  //!< edge on conical face
  EdgeTop,      //!< top edge
  VertexBottom, //!< bottom vertex
  VertexTop     //!< top vertex
};

static const std::map<FeatureTag, std::string> featureTagMap = 
{
  {FeatureTag::Root, "Root"},
  {FeatureTag::Solid, "Solid"},
  {FeatureTag::Shell, "Shell"},
  {FeatureTag::FaceBottom, "FaceBase"},
  {FeatureTag::FaceCylindrical, "FaceCylindrical"},
  {FeatureTag::FaceTop, "FaceTop"},
  {FeatureTag::WireBottom, "WireBottom"},
  {FeatureTag::WireCylindrical, "WireCylindrical"},
  {FeatureTag::WireTop, "WireTop"},
  {FeatureTag::EdgeBottom, "EdgeBottom"},
  {FeatureTag::EdgeCylindrical, "EdgeCylindrical"},
  {FeatureTag::EdgeTop, "EdgeTop"},
  {FeatureTag::VertexBottom, "VertexBottom"},
  {FeatureTag::VertexTop, "VertexTop"}
};

QIcon Cylinder::icon;

Cylinder::Cylinder() : Base(),
  radius(prm::Names::Radius, prf::manager().rootPtr->features().cylinder().get().radius()),
  height(prm::Names::Height, prf::manager().rootPtr->features().cylinder().get().height()),
  csys(prm::Names::CSys, osg::Matrixd::identity()),
  csysDragger(new ann::CSysDragger(this, &csys))
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionCylinder.svg");
  
  name = QObject::tr("Cylinder");
  mainSwitch->setUserValue(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  initializeMaps();
  
  radius.setConstraint(prm::Constraint::buildNonZeroPositive());
  height.setConstraint(prm::Constraint::buildNonZeroPositive());
  
  parameterVector.push_back(&radius);
  parameterVector.push_back(&height);
  parameterVector.push_back(&csys);
  
  annexes.insert(std::make_pair(ann::Type::CSysDragger, csysDragger.get()));
  overlaySwitch->addChild(csysDragger->dragger);
  
  radius.connectValue(boost::bind(&Cylinder::setModelDirty, this));
  height.connectValue(boost::bind(&Cylinder::setModelDirty, this));
  csys.connectValue(boost::bind(&Cylinder::setModelDirty, this));
  
  setupIPGroup();
}

Cylinder::~Cylinder()
{

}

void Cylinder::setupIPGroup()
{
  heightIP = new lbr::IPGroup(&height);
  heightIP->setMatrixDims(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(1.0, 0.0, 0.0)));
  heightIP->setRotationAxis(osg::Vec3d(0.0, 0.0, 1.0), osg::Vec3d(0.0, -1.0, 0.0));
  overlaySwitch->addChild(heightIP.get());
  csysDragger->dragger->linkToMatrix(heightIP.get());
  
  radiusIP = new lbr::IPGroup(&radius);
  radiusIP->setMatrixDims(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(0.0, 1.0, 0.0)));
  radiusIP->setMatrixDragger(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(-1.0, 0.0, 0.0)));
  radiusIP->setDimsFlipped(true);
  radiusIP->setRotationAxis(osg::Vec3d(0.0, 0.0, 1.0), osg::Vec3d(-1.0, 0.0, 0.0));
  overlaySwitch->addChild(radiusIP.get());
  csysDragger->dragger->linkToMatrix(radiusIP.get());
  
  updateIPGroup();
}

void Cylinder::updateIPGroup()
{
  //height of radius dragger
  osg::Matrixd freshMatrix;
  freshMatrix.setRotate(osg::Quat(osg::PI_2, osg::Vec3d(-1.0, 0.0, 0.0)));
  freshMatrix.setTrans(osg::Vec3d (0.0, 0.0, static_cast<double>(height) / 2.0));
  radiusIP->setMatrixDragger(freshMatrix);
  
  heightIP->setMatrix(static_cast<osg::Matrixd>(csys));
  radiusIP->setMatrix(static_cast<osg::Matrixd>(csys));
  
  heightIP->mainDim->setSqueeze(static_cast<double>(radius));
  heightIP->mainDim->setExtensionOffset(static_cast<double>(radius));
  
  radiusIP->mainDim->setSqueeze(static_cast<double>(height) / 2.0);
  radiusIP->mainDim->setExtensionOffset(static_cast<double>(height) / 2.0);
  
  heightIP->valueHasChanged();
  heightIP->constantHasChanged();
  radiusIP->valueHasChanged();
  radiusIP->constantHasChanged();
}

void Cylinder::setRadius(const double& radiusIn)
{
  radius.setValue(radiusIn);
}

void Cylinder::setHeight(const double& heightIn)
{
  height.setValue(heightIn);
}

void Cylinder::setParameters(const double& radiusIn, const double& heightIn)
{
  setRadius(radiusIn);
  setHeight(heightIn);
}

void Cylinder::setCSys(const osg::Matrixd &csysIn)
{
  osg::Matrixd oldSystem = static_cast<osg::Matrixd>(csys);
  if (!csys.setValue(csysIn))
    return; // already at this csys
    
  //apply the same transformation to dragger, so dragger moves with it.
  osg::Matrixd diffMatrix = osg::Matrixd::inverse(oldSystem) * csysIn;
  csysDragger->draggerUpdate(csysDragger->dragger->getMatrix() * diffMatrix);
}

void Cylinder::getParameters(double& radiusOut, double& heightOut) const
{
  radiusOut = static_cast<double>(radius);
  heightOut = static_cast<double>(height);
}

void Cylinder::updateModel(const UpdatePayload&)
{
  setFailure();
  lastUpdateLog.clear();
  try
  {
    CylinderBuilder cylinderMaker
    (
      static_cast<double>(radius),
      static_cast<double>(height),
      gu::toOcc(static_cast<osg::Matrixd>(csys))
    );
    seerShape->setOCCTShape(cylinderMaker.getSolid());
    updateResult(cylinderMaker);
    mainTransform->setMatrix(osg::Matrixd::identity());
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in cylinder update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in cylinder update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in cylinder update." << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  updateIPGroup();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

//the quantity of cone shapes can change so generating maps from first update can lead to missing
//ids and shapes. So here we will generate the maps with all necessary rows.
void Cylinder::initializeMaps()
{
  //result 
  std::vector<uuid> tempIds; //save ids for later.
  for (unsigned int index = 0; index < 14; ++index)
  {
    uuid tempId = gu::createRandomId();
    tempIds.push_back(tempId);
    
    EvolveRecord evolveRecord;
    evolveRecord.outId = tempId;
    seerShape->insertEvolve(evolveRecord);
  }
  
  //helper lamda
  auto insertIntoFeatureMap = [this](const uuid &idIn, FeatureTag featureTagIn)
  {
    FeatureTagRecord record;
    record.id = idIn;
    record.tag = featureTagMap.at(featureTagIn);
    seerShape->insertFeatureTag(record);
  };
  
  //first we do the compound that is root. this is not in box maker.
  insertIntoFeatureMap(tempIds.at(0), FeatureTag::Root);
  insertIntoFeatureMap(tempIds.at(1), FeatureTag::Solid);
  insertIntoFeatureMap(tempIds.at(2), FeatureTag::Shell);
  insertIntoFeatureMap(tempIds.at(3), FeatureTag::FaceBottom);
  insertIntoFeatureMap(tempIds.at(4), FeatureTag::FaceCylindrical);
  insertIntoFeatureMap(tempIds.at(5), FeatureTag::FaceTop);
  insertIntoFeatureMap(tempIds.at(6), FeatureTag::WireBottom);
  insertIntoFeatureMap(tempIds.at(7), FeatureTag::WireCylindrical);
  insertIntoFeatureMap(tempIds.at(8), FeatureTag::WireTop);
  insertIntoFeatureMap(tempIds.at(9), FeatureTag::EdgeBottom);
  insertIntoFeatureMap(tempIds.at(10), FeatureTag::EdgeCylindrical);
  insertIntoFeatureMap(tempIds.at(11), FeatureTag::EdgeTop);
  insertIntoFeatureMap(tempIds.at(12), FeatureTag::VertexBottom);
  insertIntoFeatureMap(tempIds.at(13), FeatureTag::VertexTop);
  
//   std::cout << std::endl << std::endl <<
//     "result Container: " << std::endl << resultContainer << std::endl << std::endl <<
//     "feature Container:" << std::endl << featureContainer << std::endl << std::endl <<
//     "evolution Container:" << std::endl << evolutionContainer << std::endl << std::endl;
}

void Cylinder::updateResult(const CylinderBuilder &cylinderBuilderIn)
{
  //helper lamda
  auto updateShapeByTag = [this](const TopoDS_Shape &shapeIn, FeatureTag featureTagIn)
  {
    uuid localId = seerShape->featureTagId(featureTagMap.at(featureTagIn));
    seerShape->updateShapeIdRecord(shapeIn, localId);
  };
  
  updateShapeByTag(seerShape->getRootOCCTShape(), FeatureTag::Root);
  updateShapeByTag(cylinderBuilderIn.getSolid(), FeatureTag::Solid);
  updateShapeByTag(cylinderBuilderIn.getShell(), FeatureTag::Shell);
  updateShapeByTag(cylinderBuilderIn.getFaceBottom(), FeatureTag::FaceBottom);
  updateShapeByTag(cylinderBuilderIn.getFaceCylindrical(), FeatureTag::FaceCylindrical);
  updateShapeByTag(cylinderBuilderIn.getFaceTop(), FeatureTag::FaceTop);
  updateShapeByTag(cylinderBuilderIn.getWireBottom(), FeatureTag::WireBottom);
  updateShapeByTag(cylinderBuilderIn.getWireCylindrical(), FeatureTag::WireCylindrical);
  updateShapeByTag(cylinderBuilderIn.getWireTop(), FeatureTag::WireTop);
  updateShapeByTag(cylinderBuilderIn.getEdgeBottom(), FeatureTag::EdgeBottom);
  updateShapeByTag(cylinderBuilderIn.getEdgeCylindrical(), FeatureTag::EdgeCylindrical);
  updateShapeByTag(cylinderBuilderIn.getEdgeTop(), FeatureTag::EdgeTop);
  updateShapeByTag(cylinderBuilderIn.getVertexBottom(), FeatureTag::VertexBottom);
  updateShapeByTag(cylinderBuilderIn.getVertexTop(), FeatureTag::VertexTop);
  
  seerShape->setRootShapeId(seerShape->featureTagId(featureTagMap.at(FeatureTag::Root)));
  
}

void Cylinder::serialWrite(const QDir &dIn)
{
  prj::srl::FeatureCylinder cylinderOut
  (
    Base::serialOut(),
    radius.serialOut(),
    height.serialOut(),
    csys.serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::cylinder(stream, cylinderOut, infoMap);
}

void Cylinder::serialRead(const prj::srl::FeatureCylinder& sCylinderIn)
{
  Base::serialIn(sCylinderIn.featureBase());
  radius.serialIn(sCylinderIn.radius());
  height.serialIn(sCylinderIn.height());
  csys.serialIn(sCylinderIn.csys());
  
  csysDragger->draggerUpdate(); //set dragger to parameter.
  csysDragger->dragger->setUserValue(gu::idAttributeTitle, gu::idToString(getId()));
  
  updateIPGroup();
}

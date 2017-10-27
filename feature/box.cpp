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
#include <string>
#include <map>

#include <globalutilities.h>
#include <tools/idtools.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <library/ipgroup.h>
#include <feature/boxbuilder.h>
#include <feature/seershape.h>
#include <project/serial/xsdcxxoutput/featurebox.h>
#include <feature/box.h>

using namespace ftr;
using boost::uuids::uuid;

//this is probably overkill for primitives such as box as it
//would probably be consistent using offsets. but this should
//be able to be used for all features. so consistency.

enum class FeatureTag
{
  Root,         //!< compound
  Solid,        //!< solid
  Shell,        //!< shell
  FaceXP,       //!< x positive face
  FaceXN,       //!< x negative face
  FaceYP,       //!< y positive face
  FaceYN,       //!< y negative face
  FaceZP,       //!< z positive face
  FaceZN,       //!< z negative face
  WireXP,       //!< x positive wire
  WireXN,       //!< x negative wire
  WireYP,       //!< y positive wire
  WireYN,       //!< y negative wire
  WireZP,       //!< z positive wire
  WireZN,       //!< z negative wire
  EdgeXPYP,     //!< edge at intersection of x positive face and y positive face.
  EdgeXPZP,     //!< edge at intersection of x positive face and z positive face.
  EdgeXPYN,     //!< edge at intersection of x positive face and y negative face.
  EdgeXPZN,     //!< edge at intersection of x positive face and z negative face.
  EdgeXNYN,     //!< edge at intersection of x negative face and y negative face.
  EdgeXNZP,     //!< edge at intersection of x negative face and z positive face.
  EdgeXNYP,     //!< edge at intersection of x negative face and y positive face.
  EdgeXNZN,     //!< edge at intersection of x negative face and z negative face.
  EdgeYPZP,     //!< edge at intersection of y positive face and z positive face.
  EdgeYPZN,     //!< edge at intersection of y positive face and z negative face.
  EdgeYNZP,     //!< edge at intersection of y negative face and z positive face.
  EdgeYNZN,     //!< edge at intersection of y negative face and z negative face.
  VertexXPYPZP, //!< vertex at intersection of faces x+, y+, z+
  VertexXPYNZP, //!< vertex at intersection of faces x+, y-, z+
  VertexXPYNZN, //!< vertex at intersection of faces x+, y-, z-
  VertexXPYPZN, //!< vertex at intersection of faces x+, y+, z-
  VertexXNYNZP, //!< vertex at intersection of faces x-, y-, z+
  VertexXNYPZP, //!< vertex at intersection of faces x-, y+, z+
  VertexXNYPZN, //!< vertex at intersection of faces x-, y+, z-
  VertexXNYNZN  //!< vertex at intersection of faces x-, y-, z-
};

static const std::map<FeatureTag, std::string> featureTagMap = 
{
  {FeatureTag::Root, "Root"},
  {FeatureTag::Solid, "Solid"},
  {FeatureTag::Shell, "Shell"},
  {FeatureTag::FaceXP, "FaceXP"},
  {FeatureTag::FaceXN, "FaceXN"},
  {FeatureTag::FaceYP, "FaceYP"},
  {FeatureTag::FaceYN, "FaceYN"},
  {FeatureTag::FaceZP, "FaceZP"},
  {FeatureTag::FaceZN, "FaceZN"},
  {FeatureTag::WireXP, "WireXP"},
  {FeatureTag::WireXN, "WireXN"},
  {FeatureTag::WireYP, "WireYP"},
  {FeatureTag::WireYN, "WireYN"},
  {FeatureTag::WireZP, "WireZP"},
  {FeatureTag::WireZN, "WireZN"},
  {FeatureTag::EdgeXPYP, "EdgeXPYP"},
  {FeatureTag::EdgeXPZP, "EdgeXPZP"},
  {FeatureTag::EdgeXPYN, "EdgeXPYN"},
  {FeatureTag::EdgeXPZN, "EdgeXPZN"},
  {FeatureTag::EdgeXNYN, "EdgeXNYN"},
  {FeatureTag::EdgeXNZP, "EdgeXNZP"},
  {FeatureTag::EdgeXNYP, "EdgeXNYP"},
  {FeatureTag::EdgeXNZN, "EdgeXNZN"},
  {FeatureTag::EdgeYPZP, "EdgeYPZP"},
  {FeatureTag::EdgeYPZN, "EdgeYPZN"},
  {FeatureTag::EdgeYNZP, "EdgeYNZP"},
  {FeatureTag::EdgeYNZN, "EdgeYNZN"},
  {FeatureTag::VertexXPYPZP, "VertexXPYPZP"},
  {FeatureTag::VertexXPYNZP, "VertexXPYNZP"},
  {FeatureTag::VertexXPYNZN, "VertexXPYNZN"},
  {FeatureTag::VertexXPYPZN, "VertexXPYPZN"},
  {FeatureTag::VertexXNYNZP, "VertexXNYNZP"},
  {FeatureTag::VertexXNYPZP, "VertexXNYPZP"},
  {FeatureTag::VertexXNYPZN, "VertexXNYPZN"},
  {FeatureTag::VertexXNYNZN, "VertexXNYNZN"}
};

QIcon Box::icon;

Box::Box() :
  CSysBase(),
  length(prm::Names::Length, prf::manager().rootPtr->features().box().get().length()),
  width(prm::Names::Width, prf::manager().rootPtr->features().box().get().width()),
  height(prm::Names::Height, prf::manager().rootPtr->features().box().get().height())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionBox.svg");
  
  name = QObject::tr("Box");
  mainSwitch->setUserValue(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  initializeMaps();
  
  length.setConstraint(prm::Constraint::buildNonZeroPositive());
  width.setConstraint(prm::Constraint::buildNonZeroPositive());
  height.setConstraint(prm::Constraint::buildNonZeroPositive());
  
  parameterVector.push_back(&length);
  parameterVector.push_back(&width);
  parameterVector.push_back(&height);
  
  length.connectValue(boost::bind(&Box::setModelDirty, this));
  width.connectValue(boost::bind(&Box::setModelDirty, this));
  height.connectValue(boost::bind(&Box::setModelDirty, this));
  
  setupIPGroup();
}

Box::~Box()
{

}

void Box::setupIPGroup()
{
  lengthIP = new lbr::IPGroup(&length);
  lengthIP->setMatrixDims(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(0.0, 0.0, -1.0)));
  lengthIP->noAutoRotateDragger();
  lengthIP->setRotationAxis(osg::Vec3d(1.0, 0.0, 0.0), osg::Vec3d(0.0, 0.0, 1.0));
  overlaySwitch->addChild(lengthIP.get());
  dragger->linkToMatrix(lengthIP.get());
  
  widthIP = new lbr::IPGroup(&width);
//   widthIP->setMatrixDims(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(0.0, 0.0, -1.0)));
  widthIP->noAutoRotateDragger();
  widthIP->setRotationAxis(osg::Vec3d(0.0, 1.0, 0.0), osg::Vec3d(0.0, 0.0, -1.0));
  overlaySwitch->addChild(widthIP.get());
  dragger->linkToMatrix(widthIP.get());
  
  heightIP = new lbr::IPGroup(&height);
  heightIP->setMatrixDims(osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(1.0, 0.0, 0.0)));
  heightIP->noAutoRotateDragger();
  heightIP->setRotationAxis(osg::Vec3d(0.0, 0.0, 1.0), osg::Vec3d(0.0, 1.0, 0.0));
  overlaySwitch->addChild(heightIP.get());
  dragger->linkToMatrix(heightIP.get());
  
  updateIPGroup();
}

void Box::updateIPGroup()
{
  lengthIP->setMatrix(gu::toOsg(system));
  widthIP->setMatrix(gu::toOsg(system));
  heightIP->setMatrix(gu::toOsg(system));
  
  osg::Matrix lMatrix;
  lMatrix.setRotate(osg::Quat(osg::PI_2, osg::Vec3d(0.0, 1.0, 0.0)));
  lMatrix.setTrans(osg::Vec3d(0.0, static_cast<double>(width) / 2.0, static_cast<double>(height) / 2.0));
  lengthIP->setMatrixDragger(lMatrix);
  
  osg::Matrix wMatrix;
  wMatrix.setRotate(osg::Quat(osg::PI_2, osg::Vec3d(-1.0, 0.0, 0.0)));
  wMatrix.setTrans(osg::Vec3d(static_cast<double>(length) / 2.0, 0.0, static_cast<double>(height) / 2.0));
  widthIP->setMatrixDragger(wMatrix);
  
  osg::Matrix hMatrix;
  //no need to rotate
  hMatrix.setTrans(osg::Vec3d(static_cast<double>(length) / 2.0, static_cast<double>(width) / 2.0, 0.0));
  heightIP->setMatrixDragger(hMatrix);
  
  lengthIP->valueHasChanged();
  lengthIP->constantHasChanged();
  widthIP->valueHasChanged();
  widthIP->constantHasChanged();
  heightIP->valueHasChanged();
  heightIP->constantHasChanged();
}

void Box::setLength(const double &lengthIn)
{
  length.setValue(lengthIn);
}

void Box::setWidth(const double &widthIn)
{
  width.setValue(widthIn);
}

void Box::setHeight(const double &heightIn)
{
  height.setValue(heightIn);
}

void Box::setParameters(const double &lengthIn, const double &widthIn, const double &heightIn)
{
  setLength(lengthIn);
  setWidth(widthIn);
  setHeight(heightIn);
}

void Box::getParameters(double &lengthOut, double &widthOut, double &heightOut) const
{
  lengthOut = static_cast<double>(length);
  widthOut = static_cast<double>(width);
  heightOut = static_cast<double>(height);
}

void Box::updateModel(const UpdatePayload &payloadIn)
{
  setFailure();
  
  CSysBase::updateModel(payloadIn);
  
  try
  {
    BoxBuilder boxMaker(static_cast<double>(length), static_cast<double>(width), static_cast<double>(height), system);
    seerShape->setOCCTShape(boxMaker.getSolid());
    updateResult(boxMaker);
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::cout << std::endl << "Error in box update. " << e.GetMessageString() << std::endl;
  }
  setModelClean();
  
  updateIPGroup();
}

void Box::initializeMaps()
{
  //result 
  std::vector<uuid> tempIds; //save ids for later.
  for (unsigned int index = 0; index < 35; ++index)
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
  
  insertIntoFeatureMap(tempIds.at(0), FeatureTag::Root);
  insertIntoFeatureMap(tempIds.at(1), FeatureTag::Solid);
  insertIntoFeatureMap(tempIds.at(2), FeatureTag::Shell);
  insertIntoFeatureMap(tempIds.at(3), FeatureTag::FaceXP);
  insertIntoFeatureMap(tempIds.at(4), FeatureTag::FaceXN);
  insertIntoFeatureMap(tempIds.at(5), FeatureTag::FaceYP);
  insertIntoFeatureMap(tempIds.at(6), FeatureTag::FaceYN);
  insertIntoFeatureMap(tempIds.at(7), FeatureTag::FaceZP);
  insertIntoFeatureMap(tempIds.at(8), FeatureTag::FaceZN);
  insertIntoFeatureMap(tempIds.at(9), FeatureTag::WireXP);
  insertIntoFeatureMap(tempIds.at(10), FeatureTag::WireXN);
  insertIntoFeatureMap(tempIds.at(11), FeatureTag::WireYP);
  insertIntoFeatureMap(tempIds.at(12), FeatureTag::WireYN);
  insertIntoFeatureMap(tempIds.at(13), FeatureTag::WireZP);
  insertIntoFeatureMap(tempIds.at(14), FeatureTag::WireZN);
  insertIntoFeatureMap(tempIds.at(15), FeatureTag::EdgeXPYP);
  insertIntoFeatureMap(tempIds.at(16), FeatureTag::EdgeXPZP);
  insertIntoFeatureMap(tempIds.at(17), FeatureTag::EdgeXPYN);
  insertIntoFeatureMap(tempIds.at(18), FeatureTag::EdgeXPZN);
  insertIntoFeatureMap(tempIds.at(19), FeatureTag::EdgeXNYN);
  insertIntoFeatureMap(tempIds.at(20), FeatureTag::EdgeXNZP);
  insertIntoFeatureMap(tempIds.at(21), FeatureTag::EdgeXNYP);
  insertIntoFeatureMap(tempIds.at(22), FeatureTag::EdgeXNZN);
  insertIntoFeatureMap(tempIds.at(23), FeatureTag::EdgeYPZP);
  insertIntoFeatureMap(tempIds.at(24), FeatureTag::EdgeYPZN);
  insertIntoFeatureMap(tempIds.at(25), FeatureTag::EdgeYNZP);
  insertIntoFeatureMap(tempIds.at(26), FeatureTag::EdgeYNZN);
  insertIntoFeatureMap(tempIds.at(27), FeatureTag::VertexXPYPZP);
  insertIntoFeatureMap(tempIds.at(28), FeatureTag::VertexXPYNZP);
  insertIntoFeatureMap(tempIds.at(29), FeatureTag::VertexXPYNZN);
  insertIntoFeatureMap(tempIds.at(30), FeatureTag::VertexXPYPZN);
  insertIntoFeatureMap(tempIds.at(31), FeatureTag::VertexXNYNZP);
  insertIntoFeatureMap(tempIds.at(32), FeatureTag::VertexXNYPZP);
  insertIntoFeatureMap(tempIds.at(33), FeatureTag::VertexXNYPZN);
  insertIntoFeatureMap(tempIds.at(34), FeatureTag::VertexXNYNZN);
  
//   std::cout << std::endl << std::endl <<
//     "result Container: " << std::endl << resultContainer << std::endl << std::endl <<
//     "feature Container:" << std::endl << featureContainer << std::endl << std::endl <<
//     "evolution Container:" << std::endl << evolutionContainer << std::endl << std::endl;
}
  
void Box::updateResult(const BoxBuilder& boxMakerIn)
{
  //helper lamda
  auto updateShapeByTag = [this](const TopoDS_Shape &shapeIn, FeatureTag featureTagIn)
  {
    uuid localId = seerShape->featureTagId(featureTagMap.at(featureTagIn));
    seerShape->updateShapeIdRecord(shapeIn, localId);
  };
  
  updateShapeByTag(seerShape->getRootOCCTShape(), FeatureTag::Root);
  updateShapeByTag(boxMakerIn.getSolid(), FeatureTag::Solid);
  updateShapeByTag(boxMakerIn.getShell(), FeatureTag::Shell);
  updateShapeByTag(boxMakerIn.getFaceXP(), FeatureTag::FaceXP);
  updateShapeByTag(boxMakerIn.getFaceXN(), FeatureTag::FaceXN);
  updateShapeByTag(boxMakerIn.getFaceYP(), FeatureTag::FaceYP);
  updateShapeByTag(boxMakerIn.getFaceYN(), FeatureTag::FaceYN);
  updateShapeByTag(boxMakerIn.getFaceZP(), FeatureTag::FaceZP);
  updateShapeByTag(boxMakerIn.getFaceZN(), FeatureTag::FaceZN);
  updateShapeByTag(boxMakerIn.getWireXP(), FeatureTag::WireXP);
  updateShapeByTag(boxMakerIn.getWireXN(), FeatureTag::WireXN);
  updateShapeByTag(boxMakerIn.getWireYP(), FeatureTag::WireYP);
  updateShapeByTag(boxMakerIn.getWireYN(), FeatureTag::WireYN);
  updateShapeByTag(boxMakerIn.getWireZP(), FeatureTag::WireZP);
  updateShapeByTag(boxMakerIn.getWireZN(), FeatureTag::WireZN);
  updateShapeByTag(boxMakerIn.getEdgeXPYP(), FeatureTag::EdgeXPYP);
  updateShapeByTag(boxMakerIn.getEdgeXPZP(), FeatureTag::EdgeXPZP);
  updateShapeByTag(boxMakerIn.getEdgeXPYN(), FeatureTag::EdgeXPYN);
  updateShapeByTag(boxMakerIn.getEdgeXPZN(), FeatureTag::EdgeXPZN);
  updateShapeByTag(boxMakerIn.getEdgeXNYN(), FeatureTag::EdgeXNYN);
  updateShapeByTag(boxMakerIn.getEdgeXNZP(), FeatureTag::EdgeXNZP);
  updateShapeByTag(boxMakerIn.getEdgeXNYP(), FeatureTag::EdgeXNYP);
  updateShapeByTag(boxMakerIn.getEdgeXNZN(), FeatureTag::EdgeXNZN);
  updateShapeByTag(boxMakerIn.getEdgeYPZP(), FeatureTag::EdgeYPZP);
  updateShapeByTag(boxMakerIn.getEdgeYPZN(), FeatureTag::EdgeYPZN);
  updateShapeByTag(boxMakerIn.getEdgeYNZP(), FeatureTag::EdgeYNZP);
  updateShapeByTag(boxMakerIn.getEdgeYNZN(), FeatureTag::EdgeYNZN);
  updateShapeByTag(boxMakerIn.getVertexXPYPZP(), FeatureTag::VertexXPYPZP);
  updateShapeByTag(boxMakerIn.getVertexXPYNZP(), FeatureTag::VertexXPYNZP);
  updateShapeByTag(boxMakerIn.getVertexXPYNZN(), FeatureTag::VertexXPYNZN);
  updateShapeByTag(boxMakerIn.getVertexXPYPZN(), FeatureTag::VertexXPYPZN);
  updateShapeByTag(boxMakerIn.getVertexXNYNZP(), FeatureTag::VertexXNYNZP);
  updateShapeByTag(boxMakerIn.getVertexXNYPZP(), FeatureTag::VertexXNYPZP);
  updateShapeByTag(boxMakerIn.getVertexXNYPZN(), FeatureTag::VertexXNYPZN);
  updateShapeByTag(boxMakerIn.getVertexXNYNZN(), FeatureTag::VertexXNYNZN);
  
  seerShape->setRootShapeId(seerShape->featureTagId(featureTagMap.at(FeatureTag::Root)));
  
//   std::cout << std::endl << "update result:" << std::endl << resultContainer << std::endl;
}

void Box::serialWrite(const QDir &dIn)
{
  prj::srl::FeatureBox boxOut
  (
    CSysBase::serialOut(),
    length.serialOut(),
    width.serialOut(),
    height.serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::box(stream, boxOut, infoMap);
}

void Box::serialRead(const prj::srl::FeatureBox& sBox)
{
  CSysBase::serialIn(sBox.featureCSysBase());
  length.serialIn(sBox.length());
  width.serialIn(sBox.width());
  height.serialIn(sBox.height());
  
  updateIPGroup();
}

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

#include <boost/uuid/random_generator.hpp>

#include "../globalutilities.h"
#include "boxbuilder.h"
#include "box.h"

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

Box::Box() : CSysBase(), length(10.0), width(10.0), height(10.0)
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionBox.svg");
  
  name = QObject::tr("Box");
  
  initializeMaps();
}

void Box::setLength(const double &lengthIn)
{
  if (lengthIn == length)
    return;
  assert(lengthIn > Precision::Confusion());
  setModelDirty();
  length = lengthIn;
}

void Box::setWidth(const double &widthIn)
{
  if (widthIn == width)
    return;
  assert(widthIn > Precision::Confusion());
  setModelDirty();
  width = widthIn;
}

void Box::setHeight(const double &heightIn)
{
  if (heightIn == height)
    return;
  assert(heightIn > Precision::Confusion());
  setModelDirty();
  height = heightIn;
}

void Box::setParameters(const double &lengthIn, const double &widthIn, const double &heightIn)
{
  //dirty is called in setters.
  //asserts called in setters.
  setLength(lengthIn);
  setWidth(widthIn);
  setHeight(heightIn);
}

void Box::getParameters(double &lengthOut, double &widthOut, double &heightOut) const
{
  lengthOut = length;
  widthOut = width;
  heightOut = height;
}

void Box::update(const UpdateMap& mapIn)
{
  //clear shape so if we fail the feature will be empty.
  shape = TopoDS_Shape();
  setFailure();
  
  try
  {
    BoxBuilder boxMaker(length, width, height, system);
    TopoDS_Compound wrapper = compoundWrap(boxMaker.getSolid());
    shape = wrapper;
    updateResult(boxMaker);
    setModelClean();
    setSuccess();
  }
  catch (Standard_Failure)
  {
    Handle_Standard_Failure e = Standard_Failure::Caught();
    std::cout << std::endl << "Error in box update. " << e->GetMessageString() << std::endl;
  }
}

//the quantity of cone shapes can change so generating maps from first update can lead to missing
//ids and shapes. So here we will generate the maps with all necessary rows.
void Box::initializeMaps()
{
  //result 
  std::vector<uuid> tempIds; //save ids for later.
  for (unsigned int index = 0; index < 35; ++index)
  {
    uuid tempId = boost::uuids::basic_random_generator<boost::mt19937>()();
    tempIds.push_back(tempId);
    
    ResultRecord resultRecord;
    resultRecord.id = tempId;
    resultRecord.shape = TopoDS_Shape();
    resultContainer.insert(resultRecord);
    
    EvolutionRecord evolutionRecord;
    evolutionRecord.outId = tempId;
    evolutionContainer.insert(evolutionRecord);
  }
  
  //helper lamda
  auto insertIntoFeatureMap = [this](const uuid &idIn, FeatureTag featureTagIn)
  {
    FeatureRecord record;
    record.id = idIn;
    record.tag = featureTagMap.at(featureTagIn);
    featureContainer.insert(record);
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
    uuid id = findFeatureByTag(featureContainer, featureTagMap.at(featureTagIn)).id;
    updateShapeById(resultContainer, id, shapeIn);
  };
  
  updateShapeByTag(shape, FeatureTag::Root);
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
  
//   std::cout << std::endl << "update result:" << std::endl << resultContainer << std::endl;
}


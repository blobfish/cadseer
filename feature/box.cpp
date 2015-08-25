/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013  tanderson <email>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <assert.h>
#include <string>
#include <map>

#include <BRepPrimAPI_MakeBox.hxx>
#include <TopoDS_Iterator.hxx>

#include "../globalutilities.h"
#include "boxbuilder.h"
#include "box.h"

using namespace Feature;
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

Box::Box() : Base(), length(10.0), width(10.0), height(10.0)
{
  
}

void Box::setLength(const double &lengthIn)
{
  if (lengthIn == length)
    return;
  assert(lengthIn > Precision::Confusion());
  setDirty();
  length = lengthIn;
}

void Box::setWidth(const double &widthIn)
{
  if (widthIn == width)
    return;
  assert(widthIn > Precision::Confusion());
  setDirty();
  width = widthIn;
}

void Box::setHeight(const double &heightIn)
{
  if (heightIn == height)
    return;
  assert(heightIn > Precision::Confusion());
  setDirty();
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
  try
  {
    BoxBuilder boxMaker(length, width, height);
    TopoDS_Compound wrapper = compoundWrap(boxMaker.getSolid());
    shape = wrapper;
    if (resultContainer.empty()) //very first time, need to build ids.
    {
      createResult();
      createFeature(boxMaker);
      createEvolution(boxMaker);
    }
    else
      updateResult(boxMaker);
      //don't need to update evolution as a primitive like this
      //won't every have a different set of out ids.
    setClean();
  }
  catch (Standard_Failure)
  {
    Handle_Standard_Failure e = Standard_Failure::Caught();
    std::cout << std::endl << "Error in box update. " << e->GetMessageString() << std::endl;
  }
}
  
void Box::createResult()
{
  assert(resultContainer.empty());
  buildResultContainer(shape, resultContainer);
//   std::cout << std::endl << "creation result:" << std::endl << resultContainer << std::endl;
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

void Box::createFeature(const BoxBuilder& boxMakerIn)
{
  assert(!resultContainer.empty());
  assert(featureContainer.empty());
  
  //helper lamda
  auto insertIntoFeatureMap = [this](const TopoDS_Shape &shapeIn, FeatureTag featureTagIn)
  {
    FeatureRecord record;
    record.id = findResultByShape(resultContainer, shapeIn).id;
    record.tag = featureTagMap.at(featureTagIn);
    featureContainer.insert(record);
  };
  
  //first we do the compound that is root. this is not in box maker.
  insertIntoFeatureMap(shape, FeatureTag::Root);
  insertIntoFeatureMap(boxMakerIn.getSolid(), FeatureTag::Solid);
  insertIntoFeatureMap(boxMakerIn.getShell(), FeatureTag::Shell);
  insertIntoFeatureMap(boxMakerIn.getFaceXP(), FeatureTag::FaceXP);
  insertIntoFeatureMap(boxMakerIn.getFaceXN(), FeatureTag::FaceXN);
  insertIntoFeatureMap(boxMakerIn.getFaceYP(), FeatureTag::FaceYP);
  insertIntoFeatureMap(boxMakerIn.getFaceYN(), FeatureTag::FaceYN);
  insertIntoFeatureMap(boxMakerIn.getFaceZP(), FeatureTag::FaceZP);
  insertIntoFeatureMap(boxMakerIn.getFaceZN(), FeatureTag::FaceZN);
  insertIntoFeatureMap(boxMakerIn.getWireXP(), FeatureTag::WireXP);
  insertIntoFeatureMap(boxMakerIn.getWireXN(), FeatureTag::WireXN);
  insertIntoFeatureMap(boxMakerIn.getWireYP(), FeatureTag::WireYP);
  insertIntoFeatureMap(boxMakerIn.getWireYN(), FeatureTag::WireYN);
  insertIntoFeatureMap(boxMakerIn.getWireZP(), FeatureTag::WireZP);
  insertIntoFeatureMap(boxMakerIn.getWireZN(), FeatureTag::WireZN);
  insertIntoFeatureMap(boxMakerIn.getEdgeXPYP(), FeatureTag::EdgeXPYP);
  insertIntoFeatureMap(boxMakerIn.getEdgeXPZP(), FeatureTag::EdgeXPZP);
  insertIntoFeatureMap(boxMakerIn.getEdgeXPYN(), FeatureTag::EdgeXPYN);
  insertIntoFeatureMap(boxMakerIn.getEdgeXPZN(), FeatureTag::EdgeXPZN);
  insertIntoFeatureMap(boxMakerIn.getEdgeXNYN(), FeatureTag::EdgeXNYN);
  insertIntoFeatureMap(boxMakerIn.getEdgeXNZP(), FeatureTag::EdgeXNZP);
  insertIntoFeatureMap(boxMakerIn.getEdgeXNYP(), FeatureTag::EdgeXNYP);
  insertIntoFeatureMap(boxMakerIn.getEdgeXNZN(), FeatureTag::EdgeXNZN);
  insertIntoFeatureMap(boxMakerIn.getEdgeYPZP(), FeatureTag::EdgeYPZP);
  insertIntoFeatureMap(boxMakerIn.getEdgeYPZN(), FeatureTag::EdgeYPZN);
  insertIntoFeatureMap(boxMakerIn.getEdgeYNZP(), FeatureTag::EdgeYNZP);
  insertIntoFeatureMap(boxMakerIn.getEdgeYNZN(), FeatureTag::EdgeYNZN);
  insertIntoFeatureMap(boxMakerIn.getVertexXPYPZP(), FeatureTag::VertexXPYPZP);
  insertIntoFeatureMap(boxMakerIn.getVertexXPYNZP(), FeatureTag::VertexXPYNZP);
  insertIntoFeatureMap(boxMakerIn.getVertexXPYNZN(), FeatureTag::VertexXPYNZN);
  insertIntoFeatureMap(boxMakerIn.getVertexXPYPZN(), FeatureTag::VertexXPYPZN);
  insertIntoFeatureMap(boxMakerIn.getVertexXNYNZP(), FeatureTag::VertexXNYNZP);
  insertIntoFeatureMap(boxMakerIn.getVertexXNYPZP(), FeatureTag::VertexXNYPZP);
  insertIntoFeatureMap(boxMakerIn.getVertexXNYPZN(), FeatureTag::VertexXNYPZN);
  insertIntoFeatureMap(boxMakerIn.getVertexXNYNZN(), FeatureTag::VertexXNYNZN);
  
//   std::cout << std::endl << featureContainer << std::endl;
}

void Box::createEvolution(const Feature::BoxBuilder&)
{
  evolutionContainer.clear();
  
  auto insertIntoEvolutionMap = [this](const uuid &idIn)
  {
    EvolutionRecord record;
    record.outId = idIn;
    evolutionContainer.insert(record);
  };
  
  
  for (const auto &current : resultContainer)
  {
    insertIntoEvolutionMap(current.id);
  }
  
//   std::cout << std::endl << "evolution: " << std::endl << evolutionContainer << std::endl;
}

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

#include <boost/uuid/random_generator.hpp>

#include <feature/cylinderbuilder.h>
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

Cylinder::Cylinder() : CSysBase(), radius(5.0), height(20.0)
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionCylinder.svg");
  
  name = QObject::tr("Cylinder");
  
  initializeMaps();
}

void Cylinder::setRadius(const double& radiusIn)
{
  if (radius == radiusIn)
    return;
  assert(radiusIn > Precision::Confusion());
  setModelDirty();
  radius = radiusIn;
}

void Cylinder::setHeight(const double& heightIn)
{
  if (height == heightIn)
    return;
  assert(heightIn > Precision::Confusion());
  setModelDirty();
  height = heightIn;
}

void Cylinder::setParameters(const double& radiusIn, const double& heightIn)
{
  //dirty and asserts in setters.
  setRadius(radiusIn);
  setHeight(heightIn);
}

void Cylinder::getParameters(double& radiusOut, double& heightOut) const
{
  radiusOut = radius;
  heightOut = height;
}

void Cylinder::update(const UpdateMap& mapIn)
{
  //clear shape so if we fail the feature will be empty.
  shape = TopoDS_Shape();
  setFailure();
  
  try
  {
    CylinderBuilder cylinderMaker(radius, height, system);
    shape = compoundWrap(cylinderMaker.getSolid());
    updateResult(cylinderMaker);
    setModelClean();
    setSuccess();
  }
  catch (Standard_Failure)
  {
    Handle_Standard_Failure e = Standard_Failure::Caught();
    std::cout << std::endl << "Error in cylinder update. " << e->GetMessageString() << std::endl;
  }
}

//the quantity of cone shapes can change so generating maps from first update can lead to missing
//ids and shapes. So here we will generate the maps with all necessary rows.
void Cylinder::initializeMaps()
{
  //result 
  std::vector<uuid> tempIds; //save ids for later.
  for (unsigned int index = 0; index < 14; ++index)
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
    uuid id = findFeatureByTag(featureContainer, featureTagMap.at(featureTagIn)).id;
    updateShapeById(resultContainer, id, shapeIn);
  };
  
  updateShapeByTag(shape, FeatureTag::Root);
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
  
//   std::cout << std::endl << "update result:" << std::endl << resultContainer << std::endl;
}

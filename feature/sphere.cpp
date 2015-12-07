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

#include <boost/uuid/random_generator.hpp>

#include <BRepPrimAPI_MakeSphere.hxx>

#include <feature/sphere.h>

using namespace ftr;
using namespace boost::uuids;

enum class FeatureTag
{
  Root,         //!< compound
  Solid,        //!< solid
  Shell,        //!< shell
  Face,
  Wire,
  Edge,
  VertexBottom,
  VertexTop
};

static const std::map<FeatureTag, std::string> featureTagMap = 
{
  {FeatureTag::Root, "Root"},
  {FeatureTag::Solid, "Solid"},
  {FeatureTag::Shell, "Shell"},
  {FeatureTag::Face, "Face"},
  {FeatureTag::Wire, "Wire"},
  {FeatureTag::Edge, "Edge"},
  {FeatureTag::VertexBottom, "VertexBottom"},
  {FeatureTag::VertexTop, "VertexTop"}
};

QIcon Sphere::icon;

Sphere::Sphere() : CSysBase(), radius(5.0)
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionSphere.svg");
  
  name = QObject::tr("Sphere");
  
  initializeMaps();
}

void Sphere::setRadius(const double& radiusIn)
{
  if (radius == radiusIn)
    return;
  assert(radiusIn > Precision::Confusion());
  setModelDirty();
  radius = radiusIn;
}

void Sphere::updateModel(const UpdateMap& mapIn)
{
  //clear shape so if we fail the feature will be empty.
  shape = TopoDS_Shape();
  setFailure();
  
  CSysBase::updateModel(mapIn);
  
  try
  {
    BRepPrimAPI_MakeSphere sphereMaker(system, radius);
    sphereMaker.Build();
    assert(sphereMaker.IsDone());
    shape = compoundWrap(sphereMaker.Shape());
    updateResult(sphereMaker);
    setSuccess();
  }
  catch (Standard_Failure)
  {
    Handle_Standard_Failure e = Standard_Failure::Caught();
    std::cout << std::endl << "Error in sphere update. " << e->GetMessageString() << std::endl;
  }
  setModelClean();
}

void Sphere::initializeMaps()
{
  //result 
  std::vector<uuid> tempIds; //save ids for later.
  for (unsigned int index = 0; index < 8; ++index)
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
  insertIntoFeatureMap(tempIds.at(3), FeatureTag::Face);
  insertIntoFeatureMap(tempIds.at(4), FeatureTag::Wire);
  insertIntoFeatureMap(tempIds.at(5), FeatureTag::Edge);
  insertIntoFeatureMap(tempIds.at(6), FeatureTag::VertexBottom);
  insertIntoFeatureMap(tempIds.at(7), FeatureTag::VertexTop);
  
//   std::cout << std::endl << std::endl <<
//     "result Container: " << std::endl << resultContainer << std::endl << std::endl <<
//     "feature Container:" << std::endl << featureContainer << std::endl << std::endl <<
//     "evolution Container:" << std::endl << evolutionContainer << std::endl << std::endl;
}

void Sphere::updateResult(BRepPrimAPI_MakeSphere &sphereMaker)
{
  //helper lamda
  auto updateShapeByTag = [this](const TopoDS_Shape &shapeIn, FeatureTag featureTagIn)
  {
    uuid id = findFeatureByTag(featureContainer, featureTagMap.at(featureTagIn)).id;
    updateShapeById(resultContainer, id, shapeIn);
  };
  
  updateShapeByTag(shape, FeatureTag::Root);
  updateShapeByTag(sphereMaker.Shape(), FeatureTag::Solid);
  
  BRepPrim_Sphere &sphereSubMaker = sphereMaker.Sphere();
  
  updateShapeByTag(sphereSubMaker.Shell(), FeatureTag::Shell);
  updateShapeByTag(sphereSubMaker.LateralFace(), FeatureTag::Face);
  updateShapeByTag(sphereSubMaker.LateralWire(), FeatureTag::Wire);
  updateShapeByTag(sphereSubMaker.StartEdge(), FeatureTag::Edge);
  updateShapeByTag(sphereSubMaker.BottomStartVertex(), FeatureTag::VertexBottom);
  updateShapeByTag(sphereSubMaker.TopStartVertex(), FeatureTag::VertexTop);
  
//   std::cout << std::endl << "update result:" << std::endl << resultContainer << std::endl;

}

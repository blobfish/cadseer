/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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

#include <TopoDS_Iterator.hxx>

#include <feature/box.h>
#include <feature/cylinder.h>
#include <feature/cone.h>
#include <feature/sphere.h>
#include <feature/union.h>
#include <feature/subtract.h>
#include <feature/intersect.h>
#include <feature/blend.h>
#include <feature/chamfer.h>
#include <feature/inert.h>
#include <project/serial/xsdcxxoutput/featurebox.h>
#include <project/serial/xsdcxxoutput/featurecylinder.h>
#include <project/serial/xsdcxxoutput/featuresphere.h>
#include <project/serial/xsdcxxoutput/featurecone.h>
#include <project/serial/xsdcxxoutput/featureunion.h>
#include <project/serial/xsdcxxoutput/featureintersect.h>
#include <project/serial/xsdcxxoutput/featuresubtract.h>
#include <project/serial/xsdcxxoutput/featureinert.h>
#include <project/serial/xsdcxxoutput/featureblend.h>
#include <project/serial/xsdcxxoutput/featurechamfer.h>

#include "featureload.h"

using namespace prj;

FeatureLoad::FeatureLoad(const std::string& directoryIn, const TopoDS_Shape &masterShapeIn):
directory(directoryIn), fileExtension(".fetr")
{
  for (TopoDS_Iterator it(masterShapeIn); it.More(); it.Next())
    shapeVector.push_back(it.Value());
  
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Box), std::bind(&FeatureLoad::loadBox, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Cylinder), std::bind(&FeatureLoad::loadCylinder, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Sphere), std::bind(&FeatureLoad::loadSphere, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Cone), std::bind(&FeatureLoad::loadCone, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Union), std::bind(&FeatureLoad::loadUnion, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Intersect), std::bind(&FeatureLoad::loadIntersect, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Subtract), std::bind(&FeatureLoad::loadSubtract, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Inert), std::bind(&FeatureLoad::loadInert, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Blend), std::bind(&FeatureLoad::loadBlend, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Chamfer), std::bind(&FeatureLoad::loadChamfer, this, std::placeholders::_1, std::placeholders::_2)));
}

FeatureLoad::~FeatureLoad()
{

}

std::shared_ptr< ftr::Base > FeatureLoad::load(const std::string& idIn, const std::string& typeIn, std::size_t shapeOffsetIn)
{
  auto it = functionMap.find(typeIn);
  assert(it != functionMap.end());
  
  std::ostringstream nameStream;
  nameStream << directory << idIn << fileExtension;
  
  try
  {
    return it->second(nameStream.str(), shapeOffsetIn);
  }
  catch (const xsd::cxx::xml::invalid_utf16_string&)
  {
    std::cerr << "invalid UTF-16 text in DOM model" << std::endl;
  }
  catch (const xsd::cxx::xml::invalid_utf8_string&)
  {
    std::cerr << "invalid UTF-8 text in object model" << std::endl;
  }
  catch (const xml_schema::Exception& e)
  {
    std::cerr << e << std::endl;
  }
  
  return std::shared_ptr< ftr::Base >();
}

std::shared_ptr< ftr::Base > FeatureLoad::loadBox(const std::string& fileNameIn, std::size_t shapeOffsetIn)
{
  auto box = srl::box(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(box);
  
  std::shared_ptr<ftr::Box> freshBox = std::shared_ptr<ftr::Box>(new ftr::Box());
  freshBox->setShape(shapeVector.at(shapeOffsetIn));
  freshBox->serialRead(*box);
  
  return freshBox;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadCylinder(const std::string& fileNameIn, std::size_t shapeOffsetIn)
{
  auto sCylinder = srl::cylinder(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sCylinder);
  
  std::shared_ptr<ftr::Cylinder> freshCylinder = std::shared_ptr<ftr::Cylinder>(new ftr::Cylinder());
  freshCylinder->setShape(shapeVector.at(shapeOffsetIn));
  freshCylinder->serialRead(*sCylinder);
  
  return freshCylinder;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadSphere(const std::string& fileNameIn, std::size_t shapeOffsetIn)
{
  auto sSphere = srl::sphere(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sSphere);
  
  std::shared_ptr<ftr::Sphere> freshSphere = std::shared_ptr<ftr::Sphere>(new ftr::Sphere());
  freshSphere->setShape(shapeVector.at(shapeOffsetIn));
  freshSphere->serialRead(*sSphere);
  
  return freshSphere;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadCone(const std::string& fileNameIn, std::size_t shapeOffsetIn)
{
  auto sCone = srl::cone(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sCone);
  
  std::shared_ptr<ftr::Cone> freshCone = std::shared_ptr<ftr::Cone>(new ftr::Cone());
  freshCone->setShape(shapeVector.at(shapeOffsetIn));
  freshCone->serialRead(*sCone);
  
  return freshCone;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadUnion(const std::string& fileNameIn, std::size_t shapeOffsetIn)
{
  auto sUnion = srl::fUnion(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sUnion);
  
  std::shared_ptr<ftr::Union> freshUnion = std::shared_ptr<ftr::Union>(new ftr::Union());
  freshUnion->setShape(shapeVector.at(shapeOffsetIn));
  freshUnion->serialRead(*sUnion);
  
  return freshUnion;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadIntersect(const std::string& fileNameIn, std::size_t shapeOffsetIn)
{
  auto sIntersect = srl::intersect(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sIntersect);
  
  std::shared_ptr<ftr::Intersect> freshIntersect = std::shared_ptr<ftr::Intersect>(new ftr::Intersect());
  freshIntersect->setShape(shapeVector.at(shapeOffsetIn));
  freshIntersect->serialRead(*sIntersect);
  
  return freshIntersect;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadSubtract(const std::string& fileNameIn, std::size_t shapeOffsetIn)
{
  auto sSubtract = srl::subtract(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sSubtract);
  
  std::shared_ptr<ftr::Subtract> freshSubtract = std::shared_ptr<ftr::Subtract>(new ftr::Subtract());
  freshSubtract->setShape(shapeVector.at(shapeOffsetIn));
  freshSubtract->serialRead(*sSubtract);
  
  return freshSubtract;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadInert(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sInert = srl::inert(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sInert);
  
  std::shared_ptr<ftr::Inert> freshInert = std::shared_ptr<ftr::Inert>
    (new ftr::Inert(shapeVector.at(shapeOffsetIn)));
  freshInert->serialRead(*sInert);
  
  return freshInert;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadBlend(const std::string& fileNameIn, std::size_t shapeOffsetIn)
{
  auto sBlend = srl::blend(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sBlend);
  
  std::shared_ptr<ftr::Blend> freshBlend = std::shared_ptr<ftr::Blend>(new ftr::Blend());
  freshBlend->setShape(shapeVector.at(shapeOffsetIn));
  freshBlend->serialRead(*sBlend);
  
  return freshBlend;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadChamfer(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sChamfer = srl::chamfer(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sChamfer);
  
  std::shared_ptr<ftr::Chamfer> freshChamfer = std::shared_ptr<ftr::Chamfer>(new ftr::Chamfer());
  freshChamfer->setShape(shapeVector.at(shapeOffsetIn));
  freshChamfer->serialRead(*sChamfer);
  
  return freshChamfer;
}


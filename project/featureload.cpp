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

#include <annex/seershape.h>
#include <feature/box.h>
#include <feature/cylinder.h>
#include <feature/cone.h>
#include <feature/sphere.h>
#include <feature/union.h>
#include <feature/subtract.h>
#include <feature/intersect.h>
#include <feature/blend.h>
#include <feature/chamfer.h>
#include <feature/draft.h>
#include <feature/inert.h>
#include <feature/datumplane.h>
#include <feature/hollow.h>
#include <feature/oblong.h>
#include <feature/extract.h>
#include <feature/squash.h>
#include <feature/nest.h>
#include <feature/dieset.h>
#include <feature/strip.h>
#include <feature/quote.h>
#include <feature/refine.h>
#include <feature/instancelinear.h>
#include <feature/instancemirror.h>
#include <feature/instancepolar.h>
#include <feature/offset.h>
#include <feature/thicken.h>
#include <feature/sew.h>
#include <feature/trim.h>
#include <feature/removefaces.h>
#include <feature/torus.h>
#include <feature/thread.h>
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
#include <project/serial/xsdcxxoutput/featuredraft.h>
#include <project/serial/xsdcxxoutput/featuredatumplane.h>
#include <project/serial/xsdcxxoutput/featurehollow.h>
#include <project/serial/xsdcxxoutput/featureoblong.h>
#include <project/serial/xsdcxxoutput/featureextract.h>
#include <project/serial/xsdcxxoutput/featuresquash.h>
#include <project/serial/xsdcxxoutput/featurenest.h>
#include <project/serial/xsdcxxoutput/featuredieset.h>
#include <project/serial/xsdcxxoutput/featurestrip.h>
#include <project/serial/xsdcxxoutput/featurequote.h>
#include <project/serial/xsdcxxoutput/featurerefine.h>
#include <project/serial/xsdcxxoutput/featureinstancelinear.h>
#include <project/serial/xsdcxxoutput/featureinstancemirror.h>
#include <project/serial/xsdcxxoutput/featureinstancepolar.h>
#include <project/serial/xsdcxxoutput/featureoffset.h>
#include <project/serial/xsdcxxoutput/featurethicken.h>
#include <project/serial/xsdcxxoutput/featuresew.h>
#include <project/serial/xsdcxxoutput/featuretrim.h>
#include <project/serial/xsdcxxoutput/featureremovefaces.h>
#include <project/serial/xsdcxxoutput/featuretorus.h>
#include <project/serial/xsdcxxoutput/featurethread.h>

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
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Draft), std::bind(&FeatureLoad::loadDraft, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::DatumPlane), std::bind(&FeatureLoad::loadDatumPlane, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Hollow), std::bind(&FeatureLoad::loadHollow, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Oblong), std::bind(&FeatureLoad::loadOblong, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Extract), std::bind(&FeatureLoad::loadExtract, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Squash), std::bind(&FeatureLoad::loadSquash, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Nest), std::bind(&FeatureLoad::loadNest, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::DieSet), std::bind(&FeatureLoad::loadDieSet, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Strip), std::bind(&FeatureLoad::loadStrip, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Quote), std::bind(&FeatureLoad::loadQuote, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Refine), std::bind(&FeatureLoad::loadRefine, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::InstanceLinear), std::bind(&FeatureLoad::loadInstanceLinear, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::InstanceMirror), std::bind(&FeatureLoad::loadInstanceMirror, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::InstancePolar), std::bind(&FeatureLoad::loadInstancePolar, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Offset), std::bind(&FeatureLoad::loadOffset, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Thicken), std::bind(&FeatureLoad::loadThicken, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Sew), std::bind(&FeatureLoad::loadSew, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Trim), std::bind(&FeatureLoad::loadTrim, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::RemoveFaces), std::bind(&FeatureLoad::loadRemoveFaces, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Torus), std::bind(&FeatureLoad::loadTorus, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Thread), std::bind(&FeatureLoad::loadThread, this, std::placeholders::_1, std::placeholders::_2)));
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
  
  freshBox->getAnnex<ann::SeerShape>(ann::Type::SeerShape).setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshBox->serialRead(*box);
  
  return freshBox;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadCylinder(const std::string& fileNameIn, std::size_t shapeOffsetIn)
{
  auto sCylinder = srl::cylinder(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sCylinder);
  
  std::shared_ptr<ftr::Cylinder> freshCylinder = std::shared_ptr<ftr::Cylinder>(new ftr::Cylinder());
  freshCylinder->getAnnex<ann::SeerShape>(ann::Type::SeerShape).setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshCylinder->serialRead(*sCylinder);
  
  return freshCylinder;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadSphere(const std::string& fileNameIn, std::size_t shapeOffsetIn)
{
  auto sSphere = srl::sphere(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sSphere);
  
  std::shared_ptr<ftr::Sphere> freshSphere = std::shared_ptr<ftr::Sphere>(new ftr::Sphere());
  freshSphere->getAnnex<ann::SeerShape>(ann::Type::SeerShape).setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshSphere->serialRead(*sSphere);
  
  return freshSphere;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadCone(const std::string& fileNameIn, std::size_t shapeOffsetIn)
{
  auto sCone = srl::cone(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sCone);
  
  std::shared_ptr<ftr::Cone> freshCone = std::shared_ptr<ftr::Cone>(new ftr::Cone());
  freshCone->getAnnex<ann::SeerShape>(ann::Type::SeerShape).setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshCone->serialRead(*sCone);
  
  return freshCone;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadUnion(const std::string& fileNameIn, std::size_t shapeOffsetIn)
{
  auto sUnion = srl::fUnion(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sUnion);
  
  std::shared_ptr<ftr::Union> freshUnion = std::shared_ptr<ftr::Union>(new ftr::Union());
  freshUnion->getAnnex<ann::SeerShape>(ann::Type::SeerShape).setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshUnion->serialRead(*sUnion);
  
  return freshUnion;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadIntersect(const std::string& fileNameIn, std::size_t shapeOffsetIn)
{
  auto sIntersect = srl::intersect(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sIntersect);
  
  std::shared_ptr<ftr::Intersect> freshIntersect = std::shared_ptr<ftr::Intersect>(new ftr::Intersect());
  freshIntersect->getAnnex<ann::SeerShape>(ann::Type::SeerShape).setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshIntersect->serialRead(*sIntersect);
  
  return freshIntersect;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadSubtract(const std::string& fileNameIn, std::size_t shapeOffsetIn)
{
  auto sSubtract = srl::subtract(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sSubtract);
  
  std::shared_ptr<ftr::Subtract> freshSubtract = std::shared_ptr<ftr::Subtract>(new ftr::Subtract());
  freshSubtract->getAnnex<ann::SeerShape>(ann::Type::SeerShape).setOCCTShape(shapeVector.at(shapeOffsetIn));
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
  freshBlend->getAnnex<ann::SeerShape>(ann::Type::SeerShape).setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshBlend->serialRead(*sBlend);
  
  return freshBlend;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadChamfer(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sChamfer = srl::chamfer(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sChamfer);
  
  std::shared_ptr<ftr::Chamfer> freshChamfer = std::shared_ptr<ftr::Chamfer>(new ftr::Chamfer());
  freshChamfer->getAnnex<ann::SeerShape>(ann::Type::SeerShape).setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshChamfer->serialRead(*sChamfer);
  
  return freshChamfer;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadDraft(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sDraft = srl::draft(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sDraft);
  
  std::shared_ptr<ftr::Draft> freshDraft = std::shared_ptr<ftr::Draft>(new ftr::Draft());
  freshDraft->getAnnex<ann::SeerShape>(ann::Type::SeerShape).setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshDraft->serialRead(*sDraft);
  
  return freshDraft;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadDatumPlane(const std::string &fileNameIn, std::size_t)
{
  auto sDatumPlane = srl::datumPlane(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sDatumPlane);
  
  std::shared_ptr<ftr::DatumPlane> freshDatumPlane(new ftr::DatumPlane);
  freshDatumPlane->serialRead(*sDatumPlane);
  
  return freshDatumPlane;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadHollow(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sHollow = srl::hollow(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sHollow);
  
  std::shared_ptr<ftr::Hollow> freshHollow(new ftr::Hollow);
  freshHollow->getAnnex<ann::SeerShape>(ann::Type::SeerShape).setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshHollow->serialRead(*sHollow);
  
  return freshHollow;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadOblong(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sOblong = srl::oblong(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sOblong);
  
  std::shared_ptr<ftr::Oblong> freshOblong(new ftr::Oblong);
  freshOblong->getAnnex<ann::SeerShape>(ann::Type::SeerShape).setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshOblong->serialRead(*sOblong);
  
  return freshOblong;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadExtract(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sExtract = srl::extract(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sExtract);
  
  std::shared_ptr<ftr::Extract> freshExtract(new ftr::Extract);
  freshExtract->getAnnex<ann::SeerShape>(ann::Type::SeerShape).setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshExtract->serialRead(*sExtract);
  
  return freshExtract;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadSquash(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sSquash = srl::squash(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sSquash);
  
  std::shared_ptr<ftr::Squash> freshSquash(new ftr::Squash);
  freshSquash->getAnnex<ann::SeerShape>(ann::Type::SeerShape).setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshSquash->serialRead(*sSquash);
  
  return freshSquash;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadNest(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sNest = srl::nest(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sNest);
  
  std::shared_ptr<ftr::Nest> freshNest(new ftr::Nest);
  freshNest->getAnnex<ann::SeerShape>(ann::Type::SeerShape).setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshNest->serialRead(*sNest);
  
  return freshNest;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadDieSet(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sds = srl::dieset(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sds);
  
  std::shared_ptr<ftr::DieSet> freshDieSet(new ftr::DieSet);
  freshDieSet->getAnnex<ann::SeerShape>(ann::Type::SeerShape).setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshDieSet->serialRead(*sds);
  
  return freshDieSet;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadStrip(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto ss = srl::strip(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(ss);
  
  std::shared_ptr<ftr::Strip> freshStrip(new ftr::Strip);
  freshStrip->getAnnex<ann::SeerShape>(ann::Type::SeerShape).setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshStrip->serialRead(*ss);
  
  return freshStrip;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadQuote(const std::string &fileNameIn, std::size_t)
{
  auto sq = srl::quote(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sq);
  
  std::shared_ptr<ftr::Quote> freshQuote(new ftr::Quote);
  freshQuote->serialRead(*sq);
  
  return freshQuote;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadRefine(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sr = srl::refine(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sr);
  
  std::shared_ptr<ftr::Refine> freshRefine(new ftr::Refine);
  freshRefine->getAnnex<ann::SeerShape>(ann::Type::SeerShape).setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshRefine->serialRead(*sr);
  
  return freshRefine;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadInstanceLinear(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sr = srl::instanceLinear(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sr);
  
  std::shared_ptr<ftr::InstanceLinear> freshInstanceLinear(new ftr::InstanceLinear);
  freshInstanceLinear->getAnnex<ann::SeerShape>(ann::Type::SeerShape).setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshInstanceLinear->serialRead(*sr);
  
  return freshInstanceLinear;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadInstanceMirror(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sr = srl::instanceMirror(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sr);
  
  std::shared_ptr<ftr::InstanceMirror> freshInstanceMirror(new ftr::InstanceMirror);
  freshInstanceMirror->getAnnex<ann::SeerShape>(ann::Type::SeerShape).setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshInstanceMirror->serialRead(*sr);
  
  return freshInstanceMirror;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadInstancePolar(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sr = srl::instancePolar(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sr);
  
  std::shared_ptr<ftr::InstancePolar> freshInstancePolar(new ftr::InstancePolar);
  freshInstancePolar->getAnnex<ann::SeerShape>(ann::Type::SeerShape).setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshInstancePolar->serialRead(*sr);
  
  return freshInstancePolar;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadOffset(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sr = srl::offset(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sr);
  
  std::shared_ptr<ftr::Offset> offset(new ftr::Offset);
  offset->getAnnex<ann::SeerShape>(ann::Type::SeerShape).setOCCTShape(shapeVector.at(shapeOffsetIn));
  offset->serialRead(*sr);
  
  return offset;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadThicken(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sr = srl::thicken(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sr);
  
  std::shared_ptr<ftr::Thicken> thicken(new ftr::Thicken);
  thicken->getAnnex<ann::SeerShape>(ann::Type::SeerShape).setOCCTShape(shapeVector.at(shapeOffsetIn));
  thicken->serialRead(*sr);
  
  return thicken;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadSew(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sr = srl::sew(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sr);
  
  std::shared_ptr<ftr::Sew> sew(new ftr::Sew);
  sew->getAnnex<ann::SeerShape>(ann::Type::SeerShape).setOCCTShape(shapeVector.at(shapeOffsetIn));
  sew->serialRead(*sr);
  
  return sew;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadTrim(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sr = srl::trim(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sr);
  
  std::shared_ptr<ftr::Trim> trim(new ftr::Trim);
  trim->getAnnex<ann::SeerShape>(ann::Type::SeerShape).setOCCTShape(shapeVector.at(shapeOffsetIn));
  trim->serialRead(*sr);
  
  return trim;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadRemoveFaces(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sr = srl::removeFaces(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(sr);
  
  std::shared_ptr<ftr::RemoveFaces> rfs(new ftr::RemoveFaces);
  rfs->getAnnex<ann::SeerShape>(ann::Type::SeerShape).setOCCTShape(shapeVector.at(shapeOffsetIn));
  rfs->serialRead(*sr);
  
  return rfs;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadTorus(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto st = srl::torus(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(st);
  
  std::shared_ptr<ftr::Torus> tf(new ftr::Torus);
  tf->getAnnex<ann::SeerShape>(ann::Type::SeerShape).setOCCTShape(shapeVector.at(shapeOffsetIn));
  tf->serialRead(*st);
  
  return tf;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadThread(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto st = srl::thread(fileNameIn, ::xml_schema::Flags::dont_validate);
  assert(st);
  
  std::shared_ptr<ftr::Thread> tf(new ftr::Thread);
  tf->getAnnex<ann::SeerShape>(ann::Type::SeerShape).setOCCTShape(shapeVector.at(shapeOffsetIn));
  tf->serialRead(*st);
  
  return tf;
}

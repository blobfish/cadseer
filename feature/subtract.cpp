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

#include <BOPAlgo_Builder.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <BRepTools.hxx>
#include <BOPDS_DS.hxx>

#include <feature/booleanoperation.h>
#include <feature/shapecheck.h>
#include <project/serial/xsdcxxoutput/featuresubtract.h>
#include <annex/seershape.h>
#include <annex/intersectionmapper.h>
#include <feature/subtract.h>

using boost::uuids::uuid;

using namespace ftr;

QIcon Subtract::icon;

Subtract::Subtract() : Base(), sShape(new ann::SeerShape()), iMapper(new ann::IntersectionMapper())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionSubtract.svg");
  
  name = QObject::tr("Subtract");
  mainSwitch->setUserValue(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
  annexes.insert(std::make_pair(ann::Type::IntersectionMapper, iMapper.get()));
}

Subtract::~Subtract(){}

void Subtract::updateModel(const UpdatePayload &payloadIn)
{
  setFailure(); //assume failure until success.
  lastUpdateLog.clear();
  try
  {
    if
    (
      payloadIn.updateMap.count(InputType::target) != 1 ||
      payloadIn.updateMap.count(InputType::tool) < 1
    )
    {
      //we should have a status message in the base class.
      std::ostringstream stream;
      stream << "wrong number of arguments.   " <<
      "target count is: " << payloadIn.updateMap.count(InputType::target) << "   " <<
      "tool count is: " << payloadIn.updateMap.count(InputType::tool);
      throw std::runtime_error(stream.str());
    }
    
    //target
    const ann::SeerShape &targetSeerShape =
    payloadIn.updateMap.equal_range(InputType::target).first->second->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
    assert(!targetSeerShape.isNull());
    //tools
    occt::ShapeVector toolOCCTShapes;
    for (auto pairIt = payloadIn.updateMap.equal_range(InputType::tool); pairIt.first != pairIt.second; ++pairIt.first)
    {
      const ann::SeerShape &toolSeerShape = pairIt.first->second->getAnnex<ann::SeerShape>(ann::Type::SeerShape);
      assert(!toolSeerShape.isNull());
      toolOCCTShapes.push_back(toolSeerShape.getRootOCCTShape());
    }
    
    BooleanOperation subtracter(targetSeerShape.getRootOCCTShape(), toolOCCTShapes, BOPAlgo_CUT);
    subtracter.Build();
    if (!subtracter.IsDone())
      throw std::runtime_error("OCC subtraction failed");
    ShapeCheck check(subtracter.Shape());
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    sShape->setOCCTShape(subtracter.Shape());
    sShape->shapeMatch(targetSeerShape);
    for (auto pairIt = payloadIn.updateMap.equal_range(InputType::tool); pairIt.first != pairIt.second; ++pairIt.first)
      sShape->shapeMatch(pairIt.first->second->getAnnex<ann::SeerShape>(ann::Type::SeerShape));
    sShape->uniqueTypeMatch(targetSeerShape);

    iMapper->go(payloadIn, subtracter.getBuilder(), *sShape);
    
    sShape->outerWireMatch(targetSeerShape);
    for (auto pairIt = payloadIn.updateMap.equal_range(InputType::tool); pairIt.first != pairIt.second; ++pairIt.first)
      sShape->outerWireMatch(pairIt.first->second->getAnnex<ann::SeerShape>(ann::Type::SeerShape));
    sShape->derivedMatch();
    sShape->dumpNils(getTypeString()); //only if there are shapes with nil ids.
    sShape->dumpDuplicates(getTypeString());
    sShape->ensureNoNils();
    sShape->ensureNoDuplicates();
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in subtract update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in subtract update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in subtract update." << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Subtract::serialWrite(const QDir &dIn)
{
  prj::srl::FeatureSubtract subtractOut
  (
    Base::serialOut(),
    iMapper->serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::subtract(stream, subtractOut, infoMap);
}

void Subtract::serialRead(const prj::srl::FeatureSubtract& sSubtractIn)
{
  Base::serialIn(sSubtractIn.featureBase());
  iMapper->serialIn(sSubtractIn.intersectionMapper());
}


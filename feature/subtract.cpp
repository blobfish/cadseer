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

#include <BRepAlgoAPI_Cut.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

#include <feature/booleanidmapper.h>
#include <feature/booleanoperation.h>
#include <feature/shapecheck.h>
#include <project/serial/xsdcxxoutput/featuresubtract.h>
#include <feature/seershape.h>
#include <feature/subtract.h>

using namespace ftr;

QIcon Subtract::icon;

Subtract::Subtract() : BooleanBase()
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionSubtract.svg");
  
  name = QObject::tr("Subtract");
  mainSwitch->setUserValue(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
}

void Subtract::updateModel(const UpdatePayload &payloadIn)
{
  setFailure(); //assume failure until success.
  
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
    const SeerShape &targetSeerShape = payloadIn.updateMap.equal_range(InputType::target).first->second->getSeerShape();
    assert(!targetSeerShape.isNull());
    //tools
    gu::ShapeVector toolOCCTShapes;
    for (auto pairIt = payloadIn.updateMap.equal_range(InputType::tool); pairIt.first != pairIt.second; ++pairIt.first)
    {
      const SeerShape &toolSeerShape = pairIt.first->second->getSeerShape();
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
    
    seerShape->setOCCTShape(subtracter.Shape());
    seerShape->shapeMatch(targetSeerShape);
    for (auto pairIt = payloadIn.updateMap.equal_range(InputType::tool); pairIt.first != pairIt.second; ++pairIt.first)
      seerShape->shapeMatch(pairIt.first->second->getSeerShape());
    seerShape->uniqueTypeMatch(targetSeerShape);
    BooleanIdMapper idMapper(payloadIn.updateMap, subtracter.getBuilder(), iMapWrapper, seerShape.get());
    idMapper.go();
    seerShape->outerWireMatch(targetSeerShape);
    for (auto pairIt = payloadIn.updateMap.equal_range(InputType::tool); pairIt.first != pairIt.second; ++pairIt.first)
      seerShape->outerWireMatch(pairIt.first->second->getSeerShape());
    seerShape->derivedMatch();
    seerShape->dumpNils(getTypeString()); //only if there are shapes with nil ids.
    seerShape->dumpDuplicates(getTypeString());
    seerShape->ensureNoNils();
    seerShape->ensureNoDuplicates();
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::cout << std::endl << "Error in subtract update. " << e.GetMessageString() << std::endl;
  }
  catch (std::exception &e)
  {
    std::cout << std::endl << "Error in subtract update. " << e.what() << std::endl;
  }
  setModelClean();
}

void Subtract::serialWrite(const QDir &dIn)
{
  prj::srl::FeatureSubtract subtractOut
  (
    BooleanBase::serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::subtract(stream, subtractOut, infoMap);
}

void Subtract::serialRead(const prj::srl::FeatureSubtract& sSubtractIn)
{
  BooleanBase::serialIn(sSubtractIn.featureBooleanBase());
}


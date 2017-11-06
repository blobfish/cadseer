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
#include <stdexcept>

#include <QDir>

#include <BRepAlgoAPI_Fuse.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <BOPAlgo_Builder.hxx>

#include <project/serial/xsdcxxoutput/featureunion.h>
#include <feature/booleanoperation.h>
#include <feature/booleanidmapper.h>
#include <feature/shapecheck.h>
#include <feature/seershape.h>
#include <feature/union.h>

using namespace ftr;

QIcon Union::icon;

Union::Union() : BooleanBase()
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionUnion.svg");
  
  name = QObject::tr("Union");
  mainSwitch->setUserValue(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
}

void Union::updateModel(const UpdatePayload &payloadIn)
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
    const SeerShape &targetSeerShape = payloadIn.updateMap.equal_range(InputType::target).first->second->getSeerShape();
    assert(!targetSeerShape.isNull());
    //tools
    occt::ShapeVector toolOCCTShapes;
    for (auto pairIt = payloadIn.updateMap.equal_range(InputType::tool); pairIt.first != pairIt.second; ++pairIt.first)
    {
      const SeerShape &toolSeerShape = pairIt.first->second->getSeerShape();
      assert(!toolSeerShape.isNull());
      toolOCCTShapes.push_back(toolSeerShape.getRootOCCTShape());
    }
    
    BooleanOperation fuser(targetSeerShape.getRootOCCTShape(), toolOCCTShapes, BOPAlgo_FUSE);
    fuser.Build();
    if (!fuser.IsDone())
      throw std::runtime_error("OCC fuse failed");
    ShapeCheck check(fuser.Shape());
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    seerShape->setOCCTShape(fuser.Shape());
    seerShape->shapeMatch(targetSeerShape);
    for (auto pairIt = payloadIn.updateMap.equal_range(InputType::tool); pairIt.first != pairIt.second; ++pairIt.first)
      seerShape->shapeMatch(pairIt.first->second->getSeerShape());
    seerShape->uniqueTypeMatch(targetSeerShape);
    BooleanIdMapper idMapper(payloadIn.updateMap, fuser.getBuilder(), iMapWrapper, seerShape.get());
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
    std::ostringstream s; s << "OCC Error in union update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in union update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in union update." << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Union::serialWrite(const QDir &dIn)
{
  prj::srl::FeatureUnion unionOut
  (
    BooleanBase::serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::fUnion(stream, unionOut, infoMap);
}

void Union::serialRead(const prj::srl::FeatureUnion& sUnionIn)
{
  BooleanBase::serialIn(sUnionIn.featureBooleanBase());
}

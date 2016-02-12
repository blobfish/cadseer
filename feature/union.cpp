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

#include <boost/uuid/random_generator.hpp>

#include <BRepAlgoAPI_Fuse.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <BOPAlgo_Builder.hxx>

#include <project/serial/xsdcxxoutput/featureunion.h>
#include <feature/booleanoperation.h>
#include <feature/booleanidmapper.h>
#include <feature/shapeidmapper.h>
#include <feature/union.h>

using namespace ftr;

QIcon Union::icon;

static const std::vector<std::string> shapeStrings
({
     "Compound",
     "CompSolid",
     "Solid",
     "Shell",
     "Face",
     "Wire",
     "Edge",
     "Vertex",
     "Shape",
 });

Union::Union() : BooleanBase()
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionUnion.svg");
  
  name = QObject::tr("Union");
}

void Union::updateModel(const UpdateMap &mapIn)
{
  shape = TopoDS_Shape();
  setFailure(); //assume failure until success.
  
  try
  {
    if
    (
      mapIn.count(InputTypes::target) != 1 ||
      mapIn.count(InputTypes::tool) < 1
    )
    {
      //we should have a status message in the base class.
      std::ostringstream stream;
      stream << "wrong number of arguments.   " <<
        "target count is: " << mapIn.count(InputTypes::target) << "   " <<
        "tool count is: " << mapIn.count(InputTypes::tool);
      throw std::runtime_error(stream.str());
    }
      
    //UpdateMap is going to have to be a multimap for multiple tools.
    const TopoDS_Shape &targetShape = mapIn.at(InputTypes::target)->getShape();
    const TopoDS_Shape &toolShape = mapIn.at(InputTypes::tool)->getShape();
    assert(!targetShape.IsNull() && !toolShape.IsNull());
    
    BooleanOperation fuser(targetShape, toolShape, BOPAlgo_FUSE);
    fuser.Build();
    if (!fuser.IsDone())
      throw std::runtime_error("OCC fuse failed");
    shape = fuser.Shape();
    
    ResultContainerWrapper containerWrapper;
    ResultContainer &freshContainer = containerWrapper.container;
    freshContainer = createInitialContainer(shape);
    shapeMatch(mapIn.at(InputTypes::target)->getResultContainer(), freshContainer);
    shapeMatch(mapIn.at(InputTypes::tool)->getResultContainer(), freshContainer);
    uniqueTypeMatch(mapIn.at(InputTypes::target)->getResultContainer(), freshContainer);
    
//     std::cout << std::endl << "union fresh container before booleanidmapper:" << std::endl << freshContainer << std::endl;
    
    BooleanIdMapper idMapper(mapIn, fuser.getBuilder(), iMapWrapper, containerWrapper);
    idMapper.go();
    
//     std::cout << std::endl << "union fresh container after booleanidmapper:" << std::endl << freshContainer << std::endl;
    
    outerWireMatch(mapIn.at(InputTypes::target)->getResultContainer(), freshContainer);
    outerWireMatch(mapIn.at(InputTypes::tool)->getResultContainer(), freshContainer);
    derivedMatch(shape, freshContainer, derivedContainer);
    dumpNils(freshContainer, "Union feature");
    dumpDuplicates(freshContainer, "Union feature");
    ensureNoNils(freshContainer);
    ensureNoDuplicates(freshContainer);
    
    resultContainer = freshContainer;
    
    setSuccess();
  }
  catch (Standard_Failure)
  {
    Handle_Standard_Failure e = Standard_Failure::Caught();
    std::cout << std::endl << "Error in union update. " << e->GetMessageString() << std::endl;
  }
  catch (std::exception &e)
  {
    std::cout << std::endl << "Error in union update. " << e.what() << std::endl;
  }
  setModelClean();
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

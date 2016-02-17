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

#include <QDir>

#include <boost/uuid/random_generator.hpp>

#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <gp_Ax3.hxx>
#include <gp_Trsf.hxx>
#include <TopLoc_Location.hxx>

#include <project/serial/xsdcxxoutput/featureinert.h>
#include <feature/inert.h>

using namespace ftr;
using namespace boost::uuids;

QIcon Inert::icon;

Inert::Inert(const TopoDS_Shape &shapeIn) : CSysBase()
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionInert.svg");
  
  name = QObject::tr("Inert");
  
  if (shapeIn.ShapeType() == TopAbs_COMPOUND)
    shape = shapeIn;
  else
    shape = compoundWrap(shapeIn);
}

void Inert::updateModel(const UpdateMap &mapIn)
{
  
  CSysBase::updateModel(mapIn);
  
  gp_Ax3 tempAx3(system);
  gp_Trsf tempTrsf; tempTrsf.SetTransformation(tempAx3); tempTrsf.Invert();
  TopLoc_Location freshLocation(tempTrsf);
  
  if (gu::toOsg(shape.Location().Transformation()) != gu::toOsg(tempTrsf))
  {
    shape.Location(freshLocation);
    
    //here we are just generating new ids every update.
    //this is temp as we are going to have reconcile the ids
    //from before to after.
    evolutionContainer.get<EvolutionRecord::ByInId>().clear();
    resultContainer.get<ResultRecord::ById>().clear();
    TopTools_IndexedMapOfShape shapeMap;
    TopExp::MapShapes(shape, shapeMap);
    for (int index = 1; index <= shapeMap.Extent(); ++index)
    {
      uuid tempId = idGenerator();
      
      ResultRecord record;
      record.id = tempId;
      record.shape = shapeMap(index);
      resultContainer.insert(record);
      
      EvolutionRecord evolutionRecord;
      evolutionRecord.outId = tempId;
      evolutionContainer.insert(evolutionRecord);
    }
    //not using feature container;
  }
  
  setSuccess();
  setModelClean();
}

void Inert::serialWrite(const QDir &dIn)
{
  prj::srl::FeatureInert inertOut
  (
    CSysBase::serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::inert(stream, inertOut, infoMap);
}

void Inert::serialRead(const prj::srl::FeatureInert& inert)
{
  CSysBase::serialIn(inert.featureCSysBase());
  
  setModelClean();
  setVisualDirty();
}

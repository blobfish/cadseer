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

#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <gp_Ax3.hxx>
#include <gp_Trsf.hxx>
#include <TopLoc_Location.hxx>

#include <project/serial/xsdcxxoutput/featureinert.h>
#include <globalutilities.h>
#include <feature/seershape.h>
#include <feature/inert.h>

using namespace ftr;
using namespace boost::uuids;

QIcon Inert::icon;

Inert::Inert(const TopoDS_Shape &shapeIn) : CSysBase()
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionInert.svg");
  
  name = QObject::tr("Inert");
  mainSwitch->setUserValue(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  seerShape->setOCCTShape(shapeIn);
  seerShape->ensureNoNils();
}

void Inert::updateModel(const UpdatePayload &payloadIn)
{
  
  CSysBase::updateModel(payloadIn);
  
  gp_Ax3 tempAx3(system);
  gp_Trsf tempTrsf; tempTrsf.SetTransformation(tempAx3); tempTrsf.Invert();
  TopLoc_Location freshLocation(tempTrsf);
  
  if (gu::toOsg(seerShape->getRootOCCTShape().Location().Transformation()) != gu::toOsg(tempTrsf))
  {
    //store a map of offset to id for restoration.
    std::vector<uuid> oldIds;
    TopTools_IndexedMapOfShape osm; //old shape map
    TopExp::MapShapes(seerShape->getRootOCCTShape(), osm);
    for (int i = 1; i < osm.Size() + 1; ++i)
    {
      if (seerShape->hasShapeIdRecord(osm(i))) //probably degenerated edge.
        oldIds.push_back(seerShape->findShapeIdRecord(osm(i)).id);
      else
        oldIds.push_back(gu::createNilId()); //place holder will be skipped again.
    }
    uuid oldRootId = seerShape->getRootShapeId();
    
    TopoDS_Shape tempShape(seerShape->getRootOCCTShape());
    tempShape.Location(freshLocation);
    seerShape->setOCCTShape(tempShape);
    
    seerShape->updateShapeIdRecord(seerShape->getRootOCCTShape(), oldRootId);
    seerShape->setRootShapeId(oldRootId);
    TopTools_IndexedMapOfShape nsm; //new shape map
    TopExp::MapShapes(seerShape->getRootOCCTShape(), nsm);
    for (int i = 1; i < nsm.Size() + 1; ++i)
    {
      if (seerShape->hasShapeIdRecord(nsm(i))) //probably degenerated edge.
        seerShape->updateShapeIdRecord(nsm(i), oldIds.at(i - 1));
    }
    
    seerShape->dumpNils("inert");
    seerShape->dumpDuplicates("inert");
    
    seerShape->ensureNoNils();
    seerShape->ensureNoDuplicates();
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
  
  //we are not using the function calls because they send state messages.
  //At this point the feature has not been added to the project so other
  //objects, such as dagview, don't have knowledge of this feature.
  
  state.set(ftr::StateOffset::ModelDirty, false);
  state.set(ftr::StateOffset::VisualDirty, true);
}

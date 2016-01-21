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

#include <boost/uuid/string_generator.hpp>

#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopExp.hxx>
#include <Standard_StdAllocator.hxx>

#include <osg/KdTree>

#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <nodemaskdefs.h>
#include <modelviz/shapegeometry.h>
#include <globalutilities.h>
#include <project/serial/xsdcxxoutput/featurebase.h>
#include <feature/base.h>


using namespace ftr;

std::size_t Base::nextConstructionIndex = 0;

Base::Base()
{
  id = idGenerator();
  constructionIndex = nextConstructionIndex;
  nextConstructionIndex++;
  
  name = QObject::tr("Empty");
  
  mainSwitch = new osg::Switch();
  mainSwitch->setName("feature");
  mainSwitch->setNodeMask(NodeMaskDef::object);
  mainSwitch->setUserValue(gu::idAttributeTitle, boost::uuids::to_string(id));
  
  mainTransform = new osg::MatrixTransform();
  mainTransform->setMatrix(osg::Matrixd::identity());
  mainSwitch->addChild(mainTransform);
  
  lod = new osg::LOD();
  lod->setName("lod");
  lod->setNodeMask(NodeMaskDef::lod);
  lod->setRangeMode(osg::LOD::PIXEL_SIZE_ON_SCREEN);
  mainTransform->addChild(lod.get());
  
  overlaySwitch = new osg::Switch();
  overlaySwitch->setNodeMask(NodeMaskDef::overlaySwitch);
  overlaySwitch->setName("overlay");
  overlaySwitch->setUserValue(gu::idAttributeTitle, boost::uuids::to_string(id));
  
  state.set(ftr::StateOffset::ModelDirty, true);
  state.set(ftr::StateOffset::VisualDirty, true);
  state.set(ftr::StateOffset::Hidden3D, false);
  state.set(ftr::StateOffset::HiddenOverlay, false);
  state.set(ftr::StateOffset::Failure, false);
  state.set(ftr::StateOffset::Inactive, false);
  state.set(ftr::StateOffset::NonLeaf, false);
}

Base::~Base()
{
}

//the visual is dependent on the model.
//so if model is set dirty so is the visual.
void Base::setModelDirty()
{
  //ensure model and visual are in sync.
  if (isModelClean())
  {
    state.set(ftr::StateOffset::ModelDirty, true);
    stateChangedSignal(id, ftr::StateOffset::ModelDirty);
  }
  setVisualDirty();
}

void Base::setModelClean()
{
  if (isModelClean())
    return;
  state.set(ftr::StateOffset::ModelDirty, false);
  stateChangedSignal(id, ftr::StateOffset::ModelDirty);
}

void Base::setVisualClean()
{
  if (isVisualClean())
    return;
  state.set(ftr::StateOffset::VisualDirty, false);
  stateChangedSignal(id, ftr::StateOffset::VisualDirty);
}

void Base::setVisualDirty()
{
  if(isVisualDirty())
    return;
  state.set(ftr::StateOffset::VisualDirty, true);
  stateChangedSignal(id, ftr::StateOffset::VisualDirty);
}


TopoDS_Compound Base::compoundWrap(const TopoDS_Shape& shapeIn)
{
  TopoDS_Compound compound;
  BRep_Builder builder;
  builder.MakeCompound(compound);
  builder.Add(compound, shapeIn);
  return compound;
}

void Base::updateVisual()
{
  //clear all the children from the main transform.
  lod->removeChildren(0, mainTransform->getNumChildren());
  
  if (shape.IsNull())
    return;

  mdv::BuildConnector connectBuilder(shape, resultContainer);
  connector = connectBuilder.getConnector();
  connector.outputGraphviz();
  
  //get deflection values.
  double linear = prf::manager().rootPtr->visual().mesh().linearDeflection();
  double angular = prf::manager().rootPtr->visual().mesh().angularDeflection();

  ftr::ResultContainerWrapper wrapper;
  wrapper.container = resultContainer;
  mdv::ShapeGeometryBuilder sBuilder(shape, wrapper);
  sBuilder.go(linear, angular);
  assert(sBuilder.success);
  
  lod->setRadius(sBuilder.out->getBound().radius());
  lod->addChild(sBuilder.out.get(), 0.0, 1000000.0);
//   lod->addChild(geode,0.0f,100.0f);
//   lod->addChild(cessna,100.0f,10000.0f);
  
  osg::ref_ptr<osg::KdTreeBuilder> kdTreeBuilder = new osg::KdTreeBuilder();
  lod->accept(*kdTreeBuilder);
  
  setVisualClean();
}

void Base::show3D()
{
  assert(mainSwitch->getNumChildren());
  if (isVisible3D())
    return; //already on.
  if (isVisualDirty() && isModelClean() && isSuccess())
    updateVisual();
  mainSwitch->setAllChildrenOn();
//   if (isVisibleOverlay())
//     overlaySwitch->setAllChildrenOn();
  state.set(ftr::StateOffset::Hidden3D, false);
  stateChangedSignal(id, ftr::StateOffset::Hidden3D);
}

void Base::hide3D()
{
  assert(mainSwitch->getNumChildren());
  if (isHidden3D())
    return; //already off.
  mainSwitch->setAllChildrenOff();
//   if (isVisibleOverlay())
//     overlaySwitch->setAllChildrenOff();
  state.set(ftr::StateOffset::Hidden3D, true);
  stateChangedSignal(id, ftr::StateOffset::Hidden3D);
}

void Base::toggle3D()
{
  assert(mainSwitch->getNumChildren());
  if (isVisible3D())
    hide3D();
  else
    show3D();
  stateChangedSignal(id, ftr::StateOffset::Hidden3D);
}

void Base::showOverlay()
{
  if (isVisibleOverlay())
    return; //already on.
  overlaySwitch->setAllChildrenOn();
  state.set(ftr::StateOffset::HiddenOverlay, false);
  stateChangedSignal(id, ftr::StateOffset::Hidden3D);
}

void Base::hideOverlay()
{
  if (isHiddenOverlay())
    return; //already off.
  overlaySwitch->setAllChildrenOff();
  state.set(ftr::StateOffset::HiddenOverlay, true);
  stateChangedSignal(id, ftr::StateOffset::HiddenOverlay);
}

void Base::toggleOverlay()
{
  if (isVisibleOverlay())
    hideOverlay();
  else
    showOverlay();
}

void Base::setSuccess()
{
  if (isSuccess())
    return; //already success
  state.set(ftr::StateOffset::Failure, false);
  stateChangedSignal(id, ftr::StateOffset::Failure);
}

void Base::setFailure()
{
  if (isFailure())
    return; //already failure
  state.set(ftr::StateOffset::Failure, true);
  stateChangedSignal(id, ftr::StateOffset::Failure);
}

void Base::setActive()
{
  if (isActive())
    return; //already active.
  state.set(ftr::StateOffset::Inactive, false);
  stateChangedSignal(id, ftr::StateOffset::Inactive);
}

void Base::setInActive()
{
  if (isInactive())
    return; //already inactive.
  state.set(ftr::StateOffset::Inactive, true);
  stateChangedSignal(id, ftr::StateOffset::Inactive);
}

void Base::setLeaf()
{
  if (isLeaf())
    return; //already a leaf.
  state.set(ftr::StateOffset::NonLeaf, false);
  stateChangedSignal(id, ftr::StateOffset::NonLeaf);
}

void Base::setNonLeaf()
{
  if (isNonLeaf())
    return; //already nonLeaf.
  state.set(ftr::StateOffset::NonLeaf, true);
  stateChangedSignal(id, ftr::StateOffset::NonLeaf);
}

void Base::serialWrite(const QDir&)
{
  assert(0); //missing override in subclass?
}

prj::srl::FeatureBase Base::serialOut()
{
  //update the shape offset in result container. we use the
  //shape offset to map id to shape when reading data from disk.
  TopTools_IndexedMapOfShape shapeMap;
  TopExp::MapShapes(shape, shapeMap);
  for (std::size_t index = 1; index <= static_cast<std::size_t>(shapeMap.Extent()); ++index)
  {
    if (!hasResult(resultContainer, shapeMap(index)))
      continue; //things like degenerated edges exist in shape but not in result.
    ftr::updateOffset(resultContainer, shapeMap(index), index);
  }
  
  prj::srl::EvolutionContainer eContainerOut;
  typedef EvolutionContainer::index<EvolutionRecord::ByInId>::type EList;
  const EList &eList = evolutionContainer.get<EvolutionRecord::ByInId>();
  for (EList::const_iterator it = eList.begin(); it != eList.end(); ++it)
  {
    prj::srl::EvolutionRecord eRecord
    (
      boost::uuids::to_string(it->inId),
      boost::uuids::to_string(it->outId)
    );
    eContainerOut.evolutionRecord().push_back(eRecord);
  }
  
  prj::srl::ResultContainer rContainerOut;
  typedef ResultContainer::index<ResultRecord::ById>::type RList;
  const RList &rList = resultContainer.get<ResultRecord::ById>();
  for (RList::const_iterator it = rList.begin(); it != rList.end(); ++it)
  {
    prj::srl::ResultRecord rRecord
    (
      boost::uuids::to_string(it->id),
      it->shapeOffset
    );
    rContainerOut.resultRecord().push_back(rRecord);
  }
  
  prj::srl::FeatureContainer fContainerOut;
  typedef FeatureContainer::index<FeatureRecord::ById>::type FList;
  const FList &fList = featureContainer.get<FeatureRecord::ById>();
  for (FList::const_iterator it = fList.begin(); it != fList.end(); ++it)
  {
    prj::srl::FeatureRecord fRecord
    (
      boost::uuids::to_string(it->id),
      it->tag
    );
    fContainerOut.featureRecord().push_back(fRecord);
  }
  
  return prj::srl::FeatureBase
  (
    name.toStdString(),
    boost::uuids::to_string(id),
    eContainerOut,
    rContainerOut,
    fContainerOut
  ); 
}

void Base::serialIn(const prj::srl::FeatureBase& sBaseIn)
{
  boost::uuids::string_generator sg;
  
  name = QString::fromStdString(sBaseIn.name());
  id = sg(sBaseIn.id());
  mainSwitch->setUserValue(gu::idAttributeTitle, boost::uuids::to_string(id));
  
  evolutionContainer.get<EvolutionRecord::ByInId>().clear();
  for (const prj::srl::EvolutionRecord &sERecord : sBaseIn.evolutionContainer().evolutionRecord())
  {
    EvolutionRecord record;
    record.inId = sg(sERecord.idIn());
    record.outId = sg(sERecord.idOut());
    evolutionContainer.insert(record);
  }
  
  //fill in shape vector
  std::vector<TopoDS_Shape, Standard_StdAllocator<TopoDS_Shape> > shapeVector;
  TopTools_IndexedMapOfShape shapeMap;
  TopExp::MapShapes(shape, shapeMap);
  for (std::size_t index = 1; index <= static_cast<std::size_t>(shapeMap.Extent()); ++index)
    shapeVector.push_back(shapeMap(index));
  resultContainer.get<ResultRecord::ById>().clear();
  for (const prj::srl::ResultRecord &sRRecord : sBaseIn.resultContainer().resultRecord())
  {
    ResultRecord record;
    record.id = sg(sRRecord.id());
    //shapeOffset not needed at runtime.
    record.shapeOffset = sRRecord.shapeOffset();
    record.shape = shapeVector.at(sRRecord.shapeOffset() - 1);
    resultContainer.insert(record);
  }
  
  featureContainer.get<FeatureRecord::ById>().clear();
  for (const prj::srl::FeatureRecord &sFRecord : sBaseIn.featureContainer().featureRecord())
  {
    FeatureRecord record;
    record.id = sg(sFRecord.id());
    record.tag = sFRecord.tag();
    featureContainer.insert(record);
  }
}

std::string Base::getFileName() const
{
  return boost::uuids::to_string(id) + ".fetr";
}

QString Base::buildFilePathName(const QDir &dIn) const
{
  QString out = dIn.absolutePath();
  out += QDir::separator() + 
    QString::fromStdString(getFileName());
    
  return out;
}

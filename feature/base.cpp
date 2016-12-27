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
#include <QTextStream>

#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopExp.hxx>
#include <Standard_StdAllocator.hxx>

#include <osg/KdTree>

#include <tools/idtools.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <modelviz/nodemaskdefs.h>
#include <modelviz/shapegeometry.h>
#include <globalutilities.h>
#include <feature/seershape.h>
#include <feature/seershapeinfo.h>
#include <project/serial/xsdcxxoutput/featurebase.h>
#include <feature/base.h>


using namespace ftr;

std::size_t Base::nextConstructionIndex = 0;

Base::Base()
{
  id = gu::createRandomId();
  constructionIndex = nextConstructionIndex;
  nextConstructionIndex++;
  
  name = QObject::tr("Empty");
  
  mainSwitch = new osg::Switch();
  mainSwitch->setName("feature");
  mainSwitch->setNodeMask(mdv::object);
  mainSwitch->setUserValue(gu::idAttributeTitle, gu::idToString(id));
  
  mainTransform = new osg::MatrixTransform();
  mainTransform->setMatrix(osg::Matrixd::identity());
  mainSwitch->addChild(mainTransform);
  
  lod = new osg::LOD();
  lod->setName("lod");
  lod->setNodeMask(mdv::lod);
  lod->setRangeMode(osg::LOD::PIXEL_SIZE_ON_SCREEN);
  mainTransform->addChild(lod.get());
  
  overlaySwitch = new osg::Switch();
  overlaySwitch->setNodeMask(mdv::overlaySwitch);
  overlaySwitch->setName("overlay");
  overlaySwitch->setUserValue(gu::idAttributeTitle, gu::idToString(id));
  
  state.set(ftr::StateOffset::ModelDirty, true);
  state.set(ftr::StateOffset::VisualDirty, true);
  state.set(ftr::StateOffset::Hidden3D, false);
  state.set(ftr::StateOffset::HiddenOverlay, false);
  state.set(ftr::StateOffset::Failure, false);
  state.set(ftr::StateOffset::Inactive, false);
  state.set(ftr::StateOffset::NonLeaf, false);
  
  seerShape = std::shared_ptr<SeerShape>(new SeerShape());
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
  
  if (seerShape->isNull())
    return;

  //get deflection values.
  double linear = prf::manager().rootPtr->visual().mesh().linearDeflection();
  double angular = prf::manager().rootPtr->visual().mesh().angularDeflection();

  mdv::ShapeGeometryBuilder sBuilder(seerShape);
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
    
  prj::srl::FeatureBase out
  (
    name.toStdString(),
    gu::idToString(id)
  );
  
  if (hasSeerShape())
      out.seerShape() = seerShape->serialOut();
  
  return out;
}

void Base::serialIn(const prj::srl::FeatureBase& sBaseIn)
{
  name = QString::fromStdString(sBaseIn.name());
  id = gu::stringToId(sBaseIn.id());
  //didn't investigate why, but was getting exception when  using sBaseIn.id() in next 2 calls.
  mainSwitch->setUserValue(gu::idAttributeTitle, gu::idToString(id));
  overlaySwitch->setUserValue(gu::idAttributeTitle, gu::idToString(id));
  
  if (sBaseIn.seerShape().present())
    seerShape->serialIn(sBaseIn.seerShape().get());
}

const TopoDS_Shape& Base::getShape() const
{
  static TopoDS_Shape dummy;
  if (!seerShape || seerShape->isNull())
    return dummy;
  return seerShape->getRootOCCTShape();
}

void Base::setShape(const TopoDS_Shape &in)
{
    assert(seerShape);
    seerShape->setOCCTShape(in);
}

std::string Base::getFileName() const
{
  return gu::idToString(id) + ".fetr";
}

QString Base::buildFilePathName(const QDir &dIn) const
{
  QString out = dIn.absolutePath();
  out += QDir::separator() + 
    QString::fromStdString(getFileName());
    
  return out;
}

bool Base::hasParameter(const std::string &nameIn)
{
  return (pMap.count(nameIn) > 0);
}

Parameter* Base::getParameter(const std::string &nameIn)
{
  return pMap.at(nameIn);
}

QTextStream& Base::getInfo(QTextStream &stream) const
{
    auto boolString = [](bool input)
    {
        QString out;
        if (input)
            out  = "True";
        else
            out = "False";
        return out;
    };
    
    stream << "Feature name: " << name << "        id: " << QString::fromStdString(gu::idToString(id)) << endl
        << "Feature type: " << QString::fromStdString(getTypeString()) << endl
        << "Model is clean: " << boolString(isModelClean()) << endl
        << "Visual is clean: " << boolString(isVisualClean()) << endl
        << "Update was successful: " << boolString(isSuccess()) << endl
        << "Feature is active: " << boolString(isActive()) << endl
        << "Feture is leaf: " << boolString(isLeaf()) << endl;
        
    stream << endl << "Parameters:" << endl;
    for (const auto &p : pMap)
    {
        stream
            << "    Parameter name: " << QString::fromStdString(p.second->getName())
            << "    Value: " << QString::number(p.second->getValue(), 'f', 12)
            << "    Is linked: " << boolString(!(p.second->isConstant())) << endl;
    }
    
    return stream;
}

QTextStream&  Base::getShapeInfo(QTextStream &streamIn, const boost::uuids::uuid &idIn) const
{
    assert(hasSeerShape());
    SeerShapeInfo shapeInfo(*seerShape);
    shapeInfo.getShapeInfo(streamIn, idIn);
    
    return streamIn;
}

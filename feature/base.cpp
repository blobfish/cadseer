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
#include <message/message.h>
#include <message/observer.h>
#include <feature/seershape.h>
#include <feature/shapehistory.h>
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
  
  color = osg::Vec4(.1f, .7f, .1f, 1.0f);
  
  mainSwitch = new osg::Switch();
  mainSwitch->setName("feature");
  mainSwitch->setNodeMask(mdv::object);
  mainSwitch->setUserValue(gu::idAttributeTitle, gu::idToString(id));
  
  mainTransform = new osg::MatrixTransform();
  mainTransform->setName("mainTransform");
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
  overlaySwitch->setCullingActive(false);
  
  state.set(ftr::StateOffset::ModelDirty, true);
  state.set(ftr::StateOffset::VisualDirty, true);
  state.set(ftr::StateOffset::Hidden3D, false);
  state.set(ftr::StateOffset::HiddenOverlay, false);
  state.set(ftr::StateOffset::Failure, false);
  state.set(ftr::StateOffset::Inactive, false);
  state.set(ftr::StateOffset::NonLeaf, false);
  
  seerShape = std::shared_ptr<SeerShape>(new SeerShape());
  observer = std::move(std::unique_ptr<msg::Observer>(new msg::Observer()));
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
    
    ftr::Message fMessage(id, state, StateOffset::ModelDirty, true);
    msg::Message mMessage(msg::Response | msg::Feature | msg::Status);
    mMessage.payload = fMessage;
    observer->out(mMessage);
  }
  setVisualDirty();
}

void Base::setModelClean()
{
  if (isModelClean())
    return;
  state.set(ftr::StateOffset::ModelDirty, false);
  
  ftr::Message fMessage(id, state, StateOffset::ModelDirty, false);
  msg::Message mMessage(msg::Response | msg::Feature | msg::Status);
  mMessage.payload = fMessage;
  observer->out(mMessage);
}

void Base::setVisualClean()
{
  if (isVisualClean())
    return;
  state.set(ftr::StateOffset::VisualDirty, false);
  
  ftr::Message fMessage(id, state, StateOffset::VisualDirty, false);
  msg::Message mMessage(msg::Response | msg::Feature | msg::Status);
  mMessage.payload = fMessage;
  observer->out(mMessage);
}

void Base::setVisualDirty()
{
  if(isVisualDirty())
    return;
  state.set(ftr::StateOffset::VisualDirty, true);
  
  ftr::Message fMessage(id, state, StateOffset::VisualDirty, true);
  msg::Message mMessage(msg::Response | msg::Feature | msg::Status);
  mMessage.payload = fMessage;
  observer->out(mMessage);
}

void Base::setName(const QString &nameIn)
{
  name = nameIn;
  
  ftr::Message fMessage(id, name);
  msg::Message mMessage(msg::Response | msg::Edit | msg::Feature | msg::Name);
  mMessage.payload = fMessage;
  observer->out(mMessage);
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
  lod->removeChildren(0, lod->getNumChildren());
  
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
  
  applyColor();
  
  setVisualClean();
}

void Base::setColor(const osg::Vec4 &colorIn)
{
  color = colorIn;
  applyColor();
}

void Base::applyColor()
{
  if (!hasSeerShape())
    return;
  
  if (lod->getNumChildren() == 0)
    return;
  
  mdv::ShapeGeometry *shapeViz = dynamic_cast<mdv::ShapeGeometry*>
    (lod->getChild(0)->asSwitch()->getChild(0));
  if (shapeViz)
    shapeViz->setColor(color);
}

void Base::fillInHistory(ShapeHistory &historyIn)
{
  if (hasSeerShape())
    getSeerShape().fillInHistory(historyIn, id);
}

void Base::replaceId(const boost::uuids::uuid &staleId, const boost::uuids::uuid &freshId, const ShapeHistory &shapeHistory)
{
  if (hasSeerShape())
    seerShape->replaceId(staleId, freshId, shapeHistory);
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
  
  ftr::Message fMessage(id, state, StateOffset::Hidden3D, false);
  msg::Message mMessage(msg::Response | msg::Feature | msg::Status);
  mMessage.payload = fMessage;
  observer->out(mMessage);
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
  
  ftr::Message fMessage(id, state, StateOffset::Hidden3D, true);
  msg::Message mMessage(msg::Response | msg::Feature | msg::Status);
  mMessage.payload = fMessage;
  observer->out(mMessage);
}

void Base::toggle3D()
{
  assert(mainSwitch->getNumChildren());
  if (isVisible3D())
    hide3D();
  else
    show3D();
}

void Base::showOverlay()
{
  if (isVisibleOverlay())
    return; //already on.
  overlaySwitch->setAllChildrenOn();
  state.set(ftr::StateOffset::HiddenOverlay, false);
  
  ftr::Message fMessage(id, state, StateOffset::HiddenOverlay, false);
  msg::Message mMessage(msg::Response | msg::Feature | msg::Status);
  mMessage.payload = fMessage;
  observer->out(mMessage);
}

void Base::hideOverlay()
{
  if (isHiddenOverlay())
    return; //already off.
  overlaySwitch->setAllChildrenOff();
  state.set(ftr::StateOffset::HiddenOverlay, true);
  
  ftr::Message fMessage(id, state, StateOffset::HiddenOverlay, true);
  msg::Message mMessage(msg::Response | msg::Feature | msg::Status);
  mMessage.payload = fMessage;
  observer->out(mMessage);
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
  
  ftr::Message fMessage(id, state, StateOffset::Failure, false);
  msg::Message mMessage(msg::Response | msg::Feature | msg::Status);
  mMessage.payload = fMessage;
  observer->out(mMessage);
}

void Base::setFailure()
{
  if (isFailure())
    return; //already failure
  state.set(ftr::StateOffset::Failure, true);
  
  ftr::Message fMessage(id, state, StateOffset::Failure, true);
  msg::Message mMessage(msg::Response | msg::Feature | msg::Status);
  mMessage.payload = fMessage;
  observer->out(mMessage);
}

void Base::setActive()
{
  if (isActive())
    return; //already active.
  state.set(ftr::StateOffset::Inactive, false);
  
  ftr::Message fMessage(id, state, StateOffset::Inactive, false);
  msg::Message mMessage(msg::Response | msg::Feature | msg::Status);
  mMessage.payload = fMessage;
  observer->out(mMessage);
}

void Base::setInActive()
{
  if (isInactive())
    return; //already inactive.
  state.set(ftr::StateOffset::Inactive, true);
  
  ftr::Message fMessage(id, state, StateOffset::Inactive, true);
  msg::Message mMessage(msg::Response | msg::Feature | msg::Status);
  mMessage.payload = fMessage;
  observer->out(mMessage);
}

void Base::setLeaf()
{
  if (isLeaf())
    return; //already a leaf.
  state.set(ftr::StateOffset::NonLeaf, false);
  
  ftr::Message fMessage(id, state, StateOffset::NonLeaf, false);
  msg::Message mMessage(msg::Response | msg::Feature | msg::Status);
  mMessage.payload = fMessage;
  observer->out(mMessage);
}

void Base::setNonLeaf()
{
  if (isNonLeaf())
    return; //already nonLeaf.
  state.set(ftr::StateOffset::NonLeaf, true);
  
  ftr::Message fMessage(id, state, StateOffset::NonLeaf, true);
  msg::Message mMessage(msg::Response | msg::Feature | msg::Status);
  mMessage.payload = fMessage;
  observer->out(mMessage);
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
  
  prj::srl::Color colorOut
  (
    color.r(),
    color.g(),
    color.b(),
    color.a()
  );
  out.color() = colorOut;
  
  out.state() = state.to_string();
  
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
  
  if (sBaseIn.color().present())
  {
    color.r() = sBaseIn.color().get().r();
    color.g() = sBaseIn.color().get().g();
    color.b() = sBaseIn.color().get().b();
    color.a() = sBaseIn.color().get().a();
  }
  
  if (sBaseIn.state().present())
    state = State(sBaseIn.state().get());
  
  if (isVisible3D())
    mainSwitch->setAllChildrenOn();
  else
    mainSwitch->setAllChildrenOff();
  
  if (isVisibleOverlay())
    overlaySwitch->setAllChildrenOn();
  else
    overlaySwitch->setAllChildrenOff();
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

bool Base::hasParameter(const QString &nameIn) const
{
  for (const auto &p : parameterVector)
    if (p->getName() == nameIn)
      return true;
    
  return false;
}

Parameter* Base::getParameter(const QString &nameIn) const
{
  for (const auto &p : parameterVector)
    if (p->getName() == nameIn)
      return p;
  assert(0); //no parameter by that name. use hasParameter.
  return nullptr;
}

bool Base::hasParameter(const boost::uuids::uuid &idIn) const
{
  for (const auto &p : parameterVector)
    if (p->getId() == idIn)
      return true;
    
  return false;
}

Parameter* Base::getParameter(const boost::uuids::uuid &idIn) const
{
  for (const auto &p : parameterVector)
    if (p->getId() == idIn)
      return p;
  assert(0); //no parameter by that name. use hasParameter.
  return nullptr;
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
    
    stream << "Feature info: " << endl
        << "    Feature name: " << name << endl
        << "    Feature id: " << QString::fromStdString(gu::idToString(id)) << endl
        << "    Feature type: " << QString::fromStdString(getTypeString()) << endl
        << "    Model is clean: " << boolString(isModelClean()) << endl
        << "    Visual is clean: " << boolString(isVisualClean()) << endl
        << "    Update was successful: " << boolString(isSuccess()) << endl
        << "    Feature is active: " << boolString(isActive()) << endl
        << "    Feture is leaf: " << boolString(isLeaf()) << endl;
    
    if (!parameterVector.empty())
    {
      stream << endl << "Parameters:" << endl;
      for (const auto &p : parameterVector)
      {
          stream
              << "    Parameter name: " << p->getName()
              << "    Value: " << QString::number(p->getValue(), 'f', 12)
              << "    Is linked: " << boolString(!(p->isConstant())) << endl;
      }
    }
    
    if (hasSeerShape())
      getShapeInfo(stream, seerShape->getRootShapeId());
    
    return stream;
}

QTextStream&  Base::getShapeInfo(QTextStream &streamIn, const boost::uuids::uuid &idIn) const
{
    assert(hasSeerShape());
    SeerShapeInfo shapeInfo(*seerShape);
    shapeInfo.getShapeInfo(streamIn, idIn);
    
    return streamIn;
}

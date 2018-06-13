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

#include <limits.h>

#include <QDir>
#include <QTextStream>

#include <boost/variant/variant.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopExp.hxx>
#include <Standard_StdAllocator.hxx>
#include <Precision.hxx>
#include <BinTools.hxx>

#include <osg/Switch>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/Options>
#include <osg/KdTree>
#include <osg/LightModel>
#include <osgDB/WriteFile>

#include <tools/idtools.h>
#include <tools/infotools.h>
#include <application/application.h> //need project directory for viz
#include <project/project.h> //need project directory for viz
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <modelviz/nodemaskdefs.h>
#include <modelviz/shapegeometry.h>
#include <globalutilities.h>
#include <message/message.h>
#include <message/observer.h>
#include <lod/message.h>
#include <annex/seershape.h>
#include <feature/parameter.h>
#include <feature/shapehistory.h>
#include <feature/seershapeinfo.h>
#include <project/serial/xsdcxxoutput/featurebase.h>
#include <feature/base.h>


using namespace ftr;

std::size_t Base::nextConstructionIndex = 0;


//used to display value info.
class InfoVisitor : public boost::static_visitor<QString>
{
public:
  QString operator()(double d) const {return QString::number(d, 'f', 12);}
  QString operator()(int i) const {return QString::number(i);}
  QString operator()(bool b) const {return (b) ? (QObject::tr("True")) : (QObject::tr("False"));}
  QString operator()(const std::string &s) const {return QString::fromStdString(s);}
  QString operator()(const boost::filesystem::path &p) const {return QString::fromStdString(p.string());}
  QString operator()(const osg::Vec3d &vIn) const {return gu::osgVectorOut(vIn);}
  QString operator()(const osg::Quat &qIn) const {return gu::osgQuatOut(qIn);}
  QString operator()(const osg::Matrixd &mIn) const {return gu::osgMatrixOut(mIn);}
};

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
  mainSwitch->setUserValue<std::string>(gu::idAttributeTitle, gu::idToString(id));
  
  mainTransform = new osg::MatrixTransform();
  mainTransform->setName("mainTransform");
  mainTransform->setMatrix(osg::Matrixd::identity());
  mainSwitch->addChild(mainTransform);
  
  lod = new osg::PagedLOD();
  lod->setName("lod");
  lod->setNodeMask(mdv::lod);
  lod->setRangeMode(osg::LOD::PIXEL_SIZE_ON_SCREEN);
  //not using database path in options. setting path directly on lod node in visitor within viewer.
  osgDB::Options *options = new osgDB::Options();
  options->setBuildKdTreesHint(osgDB::Options::BUILD_KDTREES);
  lod->setDatabaseOptions(options);
  mainTransform->addChild(lod.get());
  
  overlaySwitch = new osg::Switch();
  overlaySwitch->setNodeMask(mdv::overlaySwitch);
  overlaySwitch->setName("overlay");
  overlaySwitch->setUserValue<std::string>(gu::idAttributeTitle, gu::idToString(id));
  overlaySwitch->setCullingActive(false);
  
  state.set(ftr::StateOffset::ModelDirty, true);
  state.set(ftr::StateOffset::VisualDirty, true);
  state.set(ftr::StateOffset::Failure, false);
  state.set(ftr::StateOffset::Skipped, false);
  
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
    sendStateMessage(StateOffset::ModelDirty);
  }
  setVisualDirty();
}

void Base::setModelClean()
{
  if (isModelClean())
    return;
  state.set(ftr::StateOffset::ModelDirty, false);
  sendStateMessage(StateOffset::ModelDirty);
}

void Base::setVisualClean()
{
  if (isVisualClean())
    return;
  state.set(ftr::StateOffset::VisualDirty, false);
  sendStateMessage(StateOffset::VisualDirty);
}

void Base::setVisualDirty()
{
  if(isVisualDirty())
    return;
  state.set(ftr::StateOffset::VisualDirty, true);
  sendStateMessage(StateOffset::VisualDirty);
}

void Base::setSkipped()
{
  if (isSkipped())
    return;
  state.set(StateOffset::Skipped, true);
  sendStateMessage(StateOffset::Skipped);
}

void Base::setNotSkipped()
{
  if (isNotSkipped())
    return;
  state.set(StateOffset::Skipped, false);
  sendStateMessage(StateOffset::Skipped);
}

void Base::sendStateMessage(std::size_t stateOffset)
{
  ftr::Message fMessage(id, state, stateOffset);
  msg::Message mMessage(msg::Response | msg::Feature | msg::Status);
  mMessage.payload = fMessage;
  observer->out(mMessage);
}

bool Base::isVisible3D() const
{
  return mainSwitch->getNewChildDefaultValue();
}

bool Base::isHidden3D() const
{
  return !mainSwitch->getNewChildDefaultValue();
}

bool Base::isVisibleOverlay() const
{
  return overlaySwitch->getNewChildDefaultValue();
}

bool Base::isHiddenOverlay() const
{
  return !overlaySwitch->getNewChildDefaultValue();
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
  
  if ((!hasAnnex(ann::Type::SeerShape)) || (getAnnex<ann::SeerShape>(ann::Type::SeerShape).isNull()))
    return;
  const ann::SeerShape &ss = getAnnex<ann::SeerShape>(ann::Type::SeerShape);

  double linear = prf::manager().rootPtr->visual().mesh().linearDeflection();
  double angular = osg::DegreesToRadians(prf::manager().rootPtr->visual().mesh().angularDeflection());
  float screenHeight = osg::DisplaySettings::instance()->getScreenHeight(); 
  
  occt::BoundingBox bbox(ss.getRootOCCTShape());
  double diagonal = bbox.getDiagonal();
  if (diagonal < Precision::Confusion())
  {
    std::cout << "WARNING: bounding box diagonal is less than confusion in Base::updateVisual()" << std::endl;
    diagonal = 1.0;
  }
  
  ann::ShapeIdHelper helper = ss.buildHelper();
  mdv::ShapeGeometryBuilder sBuilder(ss.getRootOCCTShape(), helper);
  
  sBuilder.go
  (
    linear * prf::manager().rootPtr->visual().mesh().lod().get().LODEntry01().linearFactor(),
    angular * prf::manager().rootPtr->visual().mesh().lod().get().LODEntry01().angularFactor()
  );
  assert(sBuilder.success);
  lod->setCenter(sBuilder.out->getBound().center());
  lod->setRadius(sBuilder.out->getBound().radius());
  
  boost::filesystem::path filePathBase;
  filePathBase = static_cast<app::Application*>(qApp)->getProject()->getSaveDirectory();
  filePathBase /= ".scratch";
  boost::filesystem::path filePath00 = filePathBase / (gu::idToString(id) + "_00.osgb");
  boost::filesystem::path filePath01 = filePathBase / (gu::idToString(id) + "_01.osgb");
  boost::filesystem::path filePath02 = filePathBase / (gu::idToString(id) + "_02.osgb");
  //make sure old files are gone.
  boost::filesystem::remove(filePath00);
  boost::filesystem::remove(filePath01);
  boost::filesystem::remove(filePath02);
  
  double partition00 = screenHeight * prf::manager().rootPtr->visual().mesh().lod().get().partition00();
  double partition01 = screenHeight * prf::manager().rootPtr->visual().mesh().lod().get().partition01();
  lod->addChild(sBuilder.out, partition00, partition01, filePath00.string());
  
  //each subsequent meshing operation will take linear deflection / 10.
  //but we don't that here. that will be done in another process.
  //for now we will just copy the course mesh file to the other filenames.
  //hopefully other process can overwrite the files and pagedlod will figure it out.
  //follow up: that didn't work. going to have to generate and update pagedlod node.

  //the binary format seems to be a bit of a black box.
  //I just chose to use the brp extension. Couldn't find the accepted binary file extension.
  boost::filesystem::path filePathOCCT = filePathBase / (gu::idToString(id) + ".brp");
  BinTools::Write(getAnnex<ann::SeerShape>(ann::Type::SeerShape).getRootOCCTShape(), filePathOCCT.string().c_str());
  
  boost::filesystem::path filePathIds = filePathBase / (gu::idToString(id) + ".ids");
  helper.write(filePathIds);

  double partition02 = screenHeight * prf::manager().rootPtr->visual().mesh().lod().get().partition02();
  lod::Message m1
  (
    id,
    filePathOCCT,
    filePath01,
    filePathIds,
    linear * prf::manager().rootPtr->visual().mesh().lod().get().LODEntry02().linearFactor(),
    angular * prf::manager().rootPtr->visual().mesh().lod().get().LODEntry02().angularFactor(),
    partition01,
    partition02
  );
  observer->outBlocked(msg::Message(msg::Mask(msg::Request | msg::Construct | msg::LOD), m1));
  lod->addChild(sBuilder.out, partition01, partition02, filePath00.string());
  
  double partition03 = prf::manager().rootPtr->visual().mesh().lod().get().partition03();
  lod::Message m2
  (
    id,
    filePathOCCT,
    filePath02,
    filePathIds,
    linear * prf::manager().rootPtr->visual().mesh().lod().get().LODEntry03().linearFactor(),
    angular * prf::manager().rootPtr->visual().mesh().lod().get().LODEntry03().angularFactor(),
    partition02,
    partition03
  );
  observer->outBlocked(msg::Message(msg::Mask(msg::Request | msg::Construct | msg::LOD), m2));
  lod->addChild(sBuilder.out, partition02, partition03, filePath00.string());
  
  osg::ref_ptr<osg::KdTreeBuilder> kdTreeBuilder = new osg::KdTreeBuilder();
  lod->accept(*kdTreeBuilder);
  
  applyColor();
  
  //if the root compound contains a face or shell, turn on 2 sided lighting.
  auto add2Sided = [&]()
  {
    osg::LightModel *lm = new osg::LightModel();
    lm->setTwoSided(true);
    lod->getOrCreateStateSet()->setAttributeAndModes(lm);
  };
  //this isn't right! I need to use top explorer and avoid solids
  for (TopoDS_Iterator it(getAnnex<ann::SeerShape>(ann::Type::SeerShape).getRootOCCTShape()); it.More(); it.Next())
  {
    if
    (
      (it.Value().ShapeType() == TopAbs_SHELL) &&
      (!it.Value().Closed())
    )
    {
      add2Sided();
      break;
    }
    if (it.Value().ShapeType() == TopAbs_FACE)
    {
      add2Sided();
      break;
    }
  }
  
  setVisualClean();
}

void Base::setColor(const osg::Vec4 &colorIn)
{
  color = colorIn;
  applyColor();
}

void Base::applyColor()
{
  if (!hasAnnex(ann::Type::SeerShape))
    return;
  
  for (unsigned int i = 0; i < lod->getNumChildren(); ++i)
  {
    mdv::ShapeGeometry *shapeViz = dynamic_cast<mdv::ShapeGeometry*>
      (lod->getChild(i)->asSwitch()->getChild(0));
    if (shapeViz)
      shapeViz->setColor(color);
  }
}

void Base::fillInHistory(ShapeHistory &historyIn)
{
  if (hasAnnex(ann::Type::SeerShape))
    getAnnex<ann::SeerShape>(ann::Type::SeerShape).fillInHistory(historyIn, id);
}

void Base::replaceId(const boost::uuids::uuid &staleId, const boost::uuids::uuid &freshId, const ShapeHistory &shapeHistory)
{
  if (hasAnnex(ann::Type::SeerShape))
    getAnnex<ann::SeerShape>(ann::Type::SeerShape).replaceId(staleId, freshId, shapeHistory);
}

void Base::setSuccess()
{
  if (isSuccess())
    return; //already success
  state.set(ftr::StateOffset::Failure, false);
  
  ftr::Message fMessage(id, state, ftr::StateOffset::Failure);
  msg::Message mMessage(msg::Response | msg::Feature | msg::Status);
  mMessage.payload = fMessage;
  observer->out(mMessage);
}

void Base::setFailure()
{
  if (isFailure())
    return; //already failure
  state.set(ftr::StateOffset::Failure, true);
  
  ftr::Message fMessage(id, state, ftr::StateOffset::Failure);
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
  
  if (hasAnnex(ann::Type::SeerShape))
    out.seerShape() = getAnnex<ann::SeerShape>(ann::Type::SeerShape).serialOut();
  
  prj::srl::Color colorOut
  (
    color.r(),
    color.g(),
    color.b(),
    color.a()
  );
  out.color() = colorOut;
  
  //currently we don't serialize anything for visual.
  //so always set visualDirty so viz gets generated when
  //needed after project open.
  State temp = state;
  temp.set(StateOffset::VisualDirty);
  out.state() = temp.to_string();
  
  return out;
}

void Base::serialIn(const prj::srl::FeatureBase& sBaseIn)
{
  name = QString::fromStdString(sBaseIn.name());
  id = gu::stringToId(sBaseIn.id());
  //didn't investigate why, but was getting exception when  using sBaseIn.id() in next 2 calls.
  mainSwitch->setUserValue<std::string>(gu::idAttributeTitle, gu::idToString(id));
  overlaySwitch->setUserValue<std::string>(gu::idAttributeTitle, gu::idToString(id));
  
  if (sBaseIn.seerShape().present())
    getAnnex<ann::SeerShape>(ann::Type::SeerShape).serialIn(sBaseIn.seerShape().get());
  
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
  for (const auto &p : parameters)
    if (p->getName() == nameIn)
      return true;
    
  return false;
}

prm::Parameter* Base::getParameter(const QString &nameIn) const
{
  for (const auto &p : parameters)
    if (p->getName() == nameIn)
      return p;
  assert(0); //no parameter by that name. use hasParameter.
  return nullptr;
}

bool Base::hasParameter(const boost::uuids::uuid &idIn) const
{
  for (const auto &p : parameters)
    if (p->getId() == idIn)
      return true;
    
  return false;
}

prm::Parameter* Base::getParameter(const boost::uuids::uuid &idIn) const
{
  for (const auto &p : parameters)
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
        << endl << "Last Update: " << QString::fromStdString(lastUpdateLog);
    
    if (!parameters.empty())
    {
      stream << endl << "Parameters:" << endl;
      for (const auto &p : parameters)
      {
          stream
              << "    Parameter name: " << p->getName()
              << "    Value: " << boost::apply_visitor(InfoVisitor(), p->getVariant())
              << "    Is linked: " << boolString(!(p->isConstant())) << endl;
      }
    }
    
    if (hasAnnex(ann::Type::SeerShape))
      getShapeInfo(stream, getAnnex<ann::SeerShape>(ann::Type::SeerShape).getRootShapeId());
    
    return stream;
}

QTextStream&  Base::getShapeInfo(QTextStream &streamIn, const boost::uuids::uuid &idIn) const
{
    assert(hasAnnex(ann::Type::SeerShape));
    SeerShapeInfo shapeInfo(getAnnex<ann::SeerShape>(ann::Type::SeerShape));
    shapeInfo.getShapeInfo(streamIn, idIn);
    
    return streamIn;
}

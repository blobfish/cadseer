/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2018  Thomas S. Anderson blobfish.at.gmx.com
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

#include <gp_Cone.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <GCE2d_MakeSegment.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepLib.hxx>
#include <BRepFill.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeHalfSpace.hxx>
#include <ShapeUpgrade_UnifySameDomain.hxx>
#include <BRepBuilderAPI_Transform.hxx>

#include <osg/Switch>

#include <tools/occtools.h>
#include <annex/seershape.h>
#include <annex/csysdragger.h>
#include <library/plabel.h>
#include <library/csysdragger.h>
#include <project/serial/xsdcxxoutput/featurethread.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <feature/booleanoperation.h>
#include <feature/shapecheck.h>
#include <feature/parameter.h>
#include <feature/thread.h>

using namespace ftr;

using boost::uuids::uuid;

QIcon Thread::icon;

//defaulting to 10mm screw. need to fill in with preferences.
Thread::Thread():
Base(),
diameter(new prm::Parameter(QObject::tr("Diameter"), prf::manager().rootPtr->features().thread().get().diameter())),
pitch(new prm::Parameter(QObject::tr("Pitch"), prf::manager().rootPtr->features().thread().get().pitch())),
length(new prm::Parameter(prm::Names::Length, prf::manager().rootPtr->features().thread().get().length())),
angle(new prm::Parameter(prm::Names::Angle, prf::manager().rootPtr->features().thread().get().angle())),
internal(new prm::Parameter(QObject::tr("Internal Thread"), prf::manager().rootPtr->features().thread().get().internal())),
fake(new prm::Parameter(QObject::tr("Fake"), prf::manager().rootPtr->features().thread().get().fake())),
leftHanded(new prm::Parameter(QObject::tr("Left Handed"), prf::manager().rootPtr->features().thread().get().leftHanded())),
csys(new prm::Parameter(prm::Names::CSys, osg::Matrixd::identity())),
diameterLabel(new lbr::PLabel(diameter.get())),
pitchLabel(new lbr::PLabel(pitch.get())),
lengthLabel(new lbr::PLabel(length.get())),
angleLabel(new lbr::PLabel(angle.get())),
internalLabel(new lbr::PLabel(internal.get())),
fakeLabel(new lbr::PLabel(fake.get())),
leftHandedLabel(new lbr::PLabel(leftHanded.get())),
csysDragger(new ann::CSysDragger(this, csys.get())),
sShape(new ann::SeerShape()),
solidId(gu::createRandomId())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionThread.svg");
  
  name = QObject::tr("Thread");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  auto setupParameter = [&](prm::Parameter *p)
  {
    p->setConstraint(prm::Constraint::buildNonZeroPositive());
    p->connectValue(boost::bind(&Thread::setModelDirty, this));
    parameters.push_back(p);
  };
  setupParameter(diameter.get());
  setupParameter(pitch.get());
  setupParameter(length.get());
  
  //angle has a special constraint
  prm::Constraint angleConstraint;
  prm::Boundary lower(0.0, prm::Boundary::End::Open);
  prm::Boundary upper(180.0, prm::Boundary::End::Open);
  prm::Interval interval(lower, upper);
  angleConstraint.intervals.push_back(interval);
  angle->setConstraint(angleConstraint);
  angle->connectValue(boost::bind(&Thread::setModelDirty, this));
  parameters.push_back(angle.get());
  
  internal->connectValue(boost::bind(&Thread::setModelDirty, this));
  parameters.push_back(internal.get());
  
  fake->connectValue(boost::bind(&Thread::setModelDirty, this));
  parameters.push_back(fake.get());
  
  leftHanded->connectValue(boost::bind(&Thread::setModelDirty, this));
  parameters.push_back(leftHanded.get());
  
  csys->connectValue(boost::bind(&Thread::setModelDirty, this));
  parameters.push_back(csys.get());
  
  auto setupLabel = [&](lbr::PLabel *l)
  {
    l->showName = true;
    l->valueHasChanged();
    overlaySwitch->addChild(l);
  };
  setupLabel(diameterLabel.get());
  setupLabel(pitchLabel.get());
  setupLabel(lengthLabel.get());
  setupLabel(angleLabel.get());
  setupLabel(internalLabel.get());
  setupLabel(fakeLabel.get());
  setupLabel(leftHandedLabel.get());
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
  annexes.insert(std::make_pair(ann::Type::CSysDragger, csysDragger.get()));
  overlaySwitch->addChild(csysDragger->dragger);
}

Thread::~Thread(){}

void Thread::setDiameter(double dIn)
{
  diameter->setValue(dIn);
}

void Thread::setPitch(double pIn)
{
  pitch->setValue(pIn);
}

void Thread::setLength(double lIn)
{
  length->setValue(lIn);
}

void Thread::setAngle(double aIn)
{
  angle->setValue(aIn);
}

void Thread::setInternal(bool iIn)
{
  internal->setValue(iIn);
}

void Thread::setFake(bool fIn)
{
  fake->setValue(fIn);
}

void Thread::setLeftHanded(bool hIn)
{
  leftHanded->setValue(hIn);
}

void Thread::setCSys(const osg::Matrixd &csysIn)
{
  osg::Matrixd oldSystem = static_cast<osg::Matrixd>(*csys);
  if (!csys->setValue(csysIn))
    return; // already at this csys
    
  //apply the same transformation to dragger, so dragger moves with it.
  osg::Matrixd diffMatrix = osg::Matrixd::inverse(oldSystem) * csysIn;
  csysDragger->draggerUpdate(csysDragger->dragger->getMatrix() * diffMatrix);
}

double Thread::getDiameter() const
{
  return static_cast<double>(*diameter);
}

double Thread::getPitch() const
{
  return static_cast<double>(*pitch);
}

double Thread::getLength() const
{
  return static_cast<double>(*length);
}

double Thread::getAngle() const
{
  return static_cast<double>(*angle);
}

bool Thread::getInternal() const
{
  return static_cast<bool>(*internal);
}

bool Thread::getFake() const
{
  return static_cast<bool>(*fake);
}

bool Thread::getLeftHanded() const
{
  return static_cast<bool>(*leftHanded);
}

osg::Matrixd Thread::getCSys() const
{
  return static_cast<osg::Matrixd>(*csys);
}

//builds one revolution
TopoDS_Edge buildOneHelix(double dia, double p)
{
  auto surface = new Geom_CylindricalSurface(gp_Ax3(), dia/2.0);
  gp_Pnt2d p1(0.0, 0.0);
  gp_Pnt2d p2(2.0 * osg::PI, p);
  GCE2d_MakeSegment segmentMaker(p1, p2);
  if (!segmentMaker.IsDone())
    throw std::runtime_error("couldn't build helix");
  auto trimmedCurve = segmentMaker.Value();
  TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(trimmedCurve, surface);
  BRepLib::BuildCurves3d(edge);
  return edge;
}

//ref:
//https://en.wikipedia.org/wiki/ISO_metric_screw_thread
//tried using BRepBuilderAPI_FastSewing but it didn't work. FYI
void Thread::updateModel(const UpdatePayload&)
{
  setFailure();
  lastUpdateLog.clear();
  sShape->reset();
  try
  {
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    double d = static_cast<double>(*diameter);
    double p = static_cast<double>(*pitch);
    double l = static_cast<double>(*length);
    double a = osg::DegreesToRadians(static_cast<double>(*angle));
    
    double h = p / (2.0 * std::tan(a / 2.0));
    
    TopoDS_Shape proto;
    
    if (!static_cast<bool>(*fake))
    {
      //internal or external
      TopoDS_Edge outside01 = buildOneHelix(d + .125 * h * 2.0, p);
      
      TopoDS_Edge inside01 = buildOneHelix(d - .875 * h * 2.0, p);
      occt::moveShape(inside01, gp_Vec(0.0, 0.0, 1.0), p / 2.0);
      
      TopoDS_Edge outside02 = outside01;
      occt::moveShape(outside02, gp_Vec(0.0, 0.0, 1.0), p);
      
      TopoDS_Edge inside02 = inside01;
      occt::moveShape(inside02, gp_Vec(0.0, 0.0, 1.0), p);
      
      TopoDS_Face face01 = BRepFill::Face(outside01, inside01);
      TopoDS_Face face02 = BRepFill::Face(inside01, outside02);
      TopoDS_Face face03 = BRepFill::Face(outside02, inside02);
      
      BRepBuilderAPI_Sewing sewOp;
      TopoDS_Shape edgeToBlend;
      double blendRadius = 0.0;
      if (static_cast<bool>(*internal))
      {
        //internal threads
        sewOp.Add(face02);
        sewOp.Add(face03);
        sewOp.Perform();
        edgeToBlend = sewOp.ModifiedSubShape(outside02);
        blendRadius = (p / 16.0 * std::tan(a / 2.0)) / std::sin(a / 2.0);
      }
      else
      {
        //external threads
        sewOp.Add(face01);
        sewOp.Add(face02);
        sewOp.Perform();
        edgeToBlend = sewOp.ModifiedSubShape(inside01);
        blendRadius = (p / 8.0 * std::tan(a / 2.0)) / std::sin(a / 2.0);
      }
      
      assert(blendRadius > 0.0);
      assert(!edgeToBlend.IsNull());
      assert(!sewOp.SewedShape().IsNull());
      
      BRepFilletAPI_MakeFillet filletOp(sewOp.SewedShape());
      filletOp.Add(blendRadius, TopoDS::Edge(sewOp.ModifiedSubShape(edgeToBlend)));
      filletOp.Build();
      if (!filletOp.IsDone())
        throw std::runtime_error("fillet operation failed");
      
      proto = occt::getFirstNonCompound(filletOp.Shape());
    }
    else
    {
      //build fake threads. just cone faces no helix, no blends, or flats.
      gp_Cone coneDef(gp_Ax3(), (osg::PI / 2.0 - a / 2.0), (d - .875 * h * 2.0) / 2.0);
      double sectionLength = p / 2.0 / std::sin(a / 2.0);
      TopoDS_Face face01 = BRepBuilderAPI_MakeFace(coneDef, 0.0, 2 * osg::PI, 0.0, sectionLength);
      
      gp_Trsf mirror; mirror.SetMirror(gp_Ax1(gp_Pnt(0.0, 0.0, p / 2.0), gp_Dir(1.0, 0.0, 0.0)));
      TopoDS_Face face02 = TopoDS::Face(BRepBuilderAPI_Transform(face01, mirror, true).Shape());
      
      BRepBuilderAPI_Sewing sewOp;
      sewOp.Add(face01);
      sewOp.Add(face02);
      sewOp.Perform();
      
      proto = occt::getFirstNonCompound(sewOp.SewedShape());
    }
 
    if (proto.IsNull())
      throw std::runtime_error("couldn't build proto shape");
    
    BRepBuilderAPI_Sewing sewOp;
    int instanceCount = static_cast<int>(l / p);
    instanceCount += 4;
    for (int i = 0; i < instanceCount; ++i)
    {
      TopoDS_Shape t = proto;
      occt::moveShape(t, gp_Vec(0.0, 0.0, 1.0), p * i);
      sewOp.Add(t);
    }
    sewOp.Perform();
    proto = occt::getFirstNonCompound(sewOp.SewedShape());
    if (proto.ShapeType() != TopAbs_SHELL)
      throw std::runtime_error("didn't get expected shell out of sew operation");
    occt::moveShape(proto, gp_Vec(0.0, 0.0, -1.0), p * 2.25);
    
    gp_Pnt refPoint(d, 0.0, l / 2.0);
    TopoDS_Solid tool = BRepPrimAPI_MakeHalfSpace(TopoDS::Shell(proto), refPoint);
    TopoDS_Shape out;
    if (!static_cast<bool>(*fake))
    {
      //real threads
      if (static_cast<bool>(*internal))
      {
        //internal
        
        //create solid to work on.
        BRepPrimAPI_MakeCylinder baseMaker(d, l); //plenty big on diameter.
        baseMaker.Build();
        if (!baseMaker.IsDone())
          throw std::runtime_error("couldn't build cylinder");
        
        //cut threads away.
        BooleanOperation threadCut(baseMaker.Shape(), tool, BOPAlgo_CUT);
        threadCut.Build();
        if (!threadCut.IsDone())
          throw std::runtime_error("OCC subtraction failed");
        
        //make a cylinder and union for thread trim. p/4 in ref picture.
        BRepPrimAPI_MakeCylinder flatMaker(d / 2.0 - .625 * h, l);
        flatMaker.Build();
        if (!flatMaker.IsDone())
          throw std::runtime_error("couldn't build cylinder");
        BooleanOperation flatUniter(threadCut.Shape(), flatMaker.Shape(), BOPAlgo_FUSE);
        flatUniter.Build();
        if (!flatUniter.IsDone())
          throw std::runtime_error("OCC union failed");
        
        //this probably overkill.
        ShapeUpgrade_UnifySameDomain usd(flatUniter.Shape());
        usd.History().Nullify(); 
        usd.Build();
        
        out = usd.Shape();
      }
      else
      {
        //external
        BRepPrimAPI_MakeCylinder cylinderMaker(d / 2.0, l);
        cylinderMaker.Build();
        if (!cylinderMaker.IsDone())
          throw std::runtime_error("couldn't build cylinder");
        
        BooleanOperation subtracter(cylinderMaker.Shape(), tool, BOPAlgo_CUT);
        subtracter.Build();
        if (!subtracter.IsDone())
          throw std::runtime_error("OCC subtraction failed");
        out = subtracter.Shape();
      }
    }
    else
    {
      //fake threads.
      BRepPrimAPI_MakeCylinder cylinderMaker(d, l); //plenty big
      cylinderMaker.Build();
      if (!cylinderMaker.IsDone())
        throw std::runtime_error("couldn't build cylinder");
      
      BooleanOperation subtracter(cylinderMaker.Shape(), tool, BOPAlgo_CUT);
      subtracter.Build();
      if (!subtracter.IsDone())
        throw std::runtime_error("OCC subtraction failed");
      out = subtracter.Shape();
    }
    
    //doesn't hurt to mirror the fake threads.
    if (static_cast<bool>(*leftHanded))
    {
      gp_Trsf mirror;
      mirror.SetMirror(gp_Ax2(gp_Pnt(0.0, 0.0, l / 2.0), gp_Dir(0.0, 0.0, 1.0)));
      out = BRepBuilderAPI_Transform(out, mirror, true).Shape();
    }
    
    ShapeCheck check(out);
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    gp_Trsf nt; //new transformation
    nt.SetTransformation(gp_Ax3(gu::toOcc(static_cast<osg::Matrixd>(*csys))));
    nt.Invert();
    TopLoc_Location nl(nt); //new location
    out.Location(nt);
    
    sShape->setOCCTShape(out);
    updateIds();
    sShape->ensureNoNils();
    sShape->ensureNoDuplicates();
        
    mainTransform->setMatrix(osg::Matrixd::identity());
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in thread update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in thread update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in thread update. " << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  updateLabels();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Thread::updateIds()
{
  //the solid is the only shape guarenteed to be consistant. We just use a shape index
  //mapping. This won't guarentee consistant ids, but should eliminate subsequant feature
  //id bloat.
  
  occt::ShapeVector shapes = occt::mapShapes(sShape->getRootOCCTShape());
  int diff = shapes.size() - ids.size();
  if (diff < 0)
    diff = 0;
  for (int i = 0; i < diff; ++i)
    ids.push_back(gu::createRandomId());
  int si = 0; //shapeIndex.
  for (const auto &s : shapes)
  {
    //don't overwrite the root compound
    if (sShape->findShapeIdRecord(s).id.is_nil())
    {
      sShape->updateShapeIdRecord(s, ids.at(si));
      if (!sShape->hasEvolveRecordOut(ids.at(si)))
        sShape->insertEvolve(gu::createNilId(), ids.at(si));
    }
    si++;
  }
  
  TopoDS_Shape solid = occt::getFirstNonCompound(sShape->getRootOCCTShape());
  assert(solid.ShapeType() == TopAbs_SOLID);
  sShape->updateShapeIdRecord(solid, solidId);
  if (!sShape->hasEvolveRecordOut(solidId))
    sShape->insertEvolve(gu::createNilId(), solidId);
}

void Thread::updateLabels()
{
  osg::Matrixd m = static_cast<osg::Matrixd>(*csys);
  double d = static_cast<double>(*diameter);
  double l = static_cast<double>(*length);
  
  diameterLabel->setMatrix(osg::Matrixd::translate(osg::Vec3d(d / 2.0, 0.0, l * .625) * m));
  pitchLabel->setMatrix(osg::Matrixd::translate(osg::Vec3d(d / 2.0, 0.0, l * .875) * m));
  lengthLabel->setMatrix(osg::Matrixd::translate(osg::Vec3d(0.0, 0.0, l) * m));
  angleLabel->setMatrix(osg::Matrixd::translate(osg::Vec3d(d / 2.0, 0.0, l *.375) * m));
  internalLabel->setMatrix(osg::Matrixd::translate(osg::Vec3d(-d / 2.0, 0.0, l * .875) * m));
  fakeLabel->setMatrix(osg::Matrixd::translate(osg::Vec3d(-d / 2.0, 0.0, l * .625) * m));
  leftHandedLabel->setMatrix(osg::Matrixd::translate(osg::Vec3d(-d / 2.0, 0.0, l *.375) * m));
}

void Thread::serialWrite(const QDir &dIn)
{
  prj::srl::Ids idsOut;
  for (const auto &idOut : ids)
    idsOut.id().push_back(gu::idToString(idOut));
  
  prj::srl::FeatureThread to //thread out.
  (
    Base::serialOut(),
    diameter->serialOut(),
    pitch->serialOut(),
    length->serialOut(),
    angle->serialOut(),
    internal->serialOut(),
    fake->serialOut(),
    leftHanded->serialOut(),
    csys->serialOut(),
    diameterLabel->serialOut(),
    pitchLabel->serialOut(),
    lengthLabel->serialOut(),
    angleLabel->serialOut(),
    internalLabel->serialOut(),
    fakeLabel->serialOut(),
    leftHandedLabel->serialOut(),
    csysDragger->serialOut(),
    gu::idToString(solidId),
    idsOut
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::thread(stream, to, infoMap);
}

void Thread::serialRead(const prj::srl::FeatureThread &ti)
{
  Base::serialIn(ti.featureBase());
  diameter->serialIn(ti.diameter());
  pitch->serialIn(ti.pitch());
  length->serialIn(ti.length());
  angle->serialIn(ti.angle());
  internal->serialIn(ti.internal());
  fake->serialIn(ti.fake());
  leftHanded->serialIn(ti.leftHanded());
  csys->serialIn(ti.csys());
  diameterLabel->serialIn(ti.diameterLabel());
  pitchLabel->serialIn(ti.pitchLabel());
  lengthLabel->serialIn(ti.lengthLabel());
  angleLabel->serialIn(ti.angleLabel());
  internalLabel->serialIn(ti.internalLabel());
  fakeLabel->serialIn(ti.fakeLabel());
  leftHandedLabel->serialIn(ti.leftHandedLabel());
  csysDragger->serialIn(ti.csysDragger());
  solidId = gu::stringToId(ti.solidId());
  
  for (const auto &idIn : ti.ids().id())
    ids.push_back(gu::stringToId(idIn));
}

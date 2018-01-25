/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepExtrema_Poly.hxx>
#include <BRepMesh_IncrementalMesh.hxx>

#include <globalutilities.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <library/plabel.h>
#include <feature/parameter.h>
#include <annex/seershape.h>
#include <feature/shapecheck.h>
#include <tools/occtools.h>
#include <project/serial/xsdcxxoutput/featurenest.h>
#include <feature/nest.h>

using namespace ftr;

using boost::uuids::uuid;

QIcon Nest::icon;

Nest::Nest() : 
Base(),
pitch(new prm::Parameter(QObject::tr("Pitch"), 1.0)),
gap(new prm::Parameter(QObject::tr("Gap"), prf::manager().rootPtr->features().nest().get().gap())),
feedDirection(new prm::Parameter(QObject::tr("Feed Direction"), osg::Vec3d(-1.0, 0.0, 0.0))),
sShape(new ann::SeerShape())

{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionNest.svg");
  
  name = QObject::tr("Nest");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  pitch->setConstraint(prm::Constraint::buildNonZeroPositive());
  parameterVector.push_back(pitch.get());
  //pitch does not get wired up to dirty.
  
  gap->setConstraint(prm::Constraint::buildNonZeroPositive());
  gap->connectValue(boost::bind(&Nest::setModelDirty, this));
  parameterVector.push_back(gap.get());
  
  feedDirection->connectValue(boost::bind(&Nest::setModelDirty, this));
  parameterVector.push_back(feedDirection.get());
  
  gapLabel = new lbr::PLabel(gap.get());
  gapLabel->showName = true;
  gapLabel->valueHasChanged();
  overlaySwitch->addChild(gapLabel.get());
  
  feedDirectionLabel = new lbr::PLabel(feedDirection.get());
  feedDirectionLabel->showName = true;
  feedDirectionLabel->valueHasChanged();
  overlaySwitch->addChild(feedDirectionLabel.get());
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
}

Nest::~Nest(){}

double Nest::getPitch() const
{
  return static_cast<double>(*pitch);
}

double Nest::getGap() const
{
  return static_cast<double>(*gap);
}

osg::Vec3d Nest::getFeedDirection() const
{
  return static_cast<osg::Vec3d>(*feedDirection);
}

double Nest::getDistance(const TopoDS_Shape &sIn1, const TopoDS_Shape &sIn2)
{
  gp_Pnt p1, p2;
  double distance;
  if (BRepExtrema_Poly::Distance(sIn1, sIn2, p1, p2, distance))
  {
    //position the gapLabel.
    gp_Vec pos1(p1.XYZ());
    gp_Vec pos2(p2.XYZ());
    osg::Vec3d gapPosition = gu::toOsg(pos1 + ((pos2 - pos1) * 0.5));
    gapLabel->setMatrix(osg::Matrixd::translate(gapPosition));
    
    return distance;
  }
  
  //this shouldn't ever be run as we ensure the poly/mesh before calling.
  //note this execution path doesn't set the gap label position.
  //adding tolerance didn't make the 1 test I was using any faster.
  //these parts have nothing but linear edges, so maybe once we have
  //some non-linear edges this tolerance will be beneficial.
  double tol = 0.1;
  BRepExtrema_DistShapeShape dc(sIn1, sIn2, tol, Extrema_ExtFlag_MIN);
  if (!dc.IsDone())
    return -1.0;
  if (dc.NbSolution() < 1)
    return -1.0;
  return dc.Value();
}

TopoDS_Shape Nest::calcPitch(TopoDS_Shape &bIn, double guess)
{
  //guess is expected from the bounding box and assumes no overlap.
  //dir is a unit vector.
  gp_Vec dir = gu::toOcc(static_cast<osg::Vec3d>(*feedDirection));
  double localGap = static_cast<double>(*gap);
  double tol = localGap * 0.1;
  TopoDS_Shape other = occt::instanceShape(bIn, dir, guess + localGap + tol);
  
  double dist = getDistance(bIn, other);
  if (dist == -1.0)
    throw std::runtime_error("couldn't get start position in getPitch");
  
  int maxIter = 100;
  int iter = 0;
  while (dist > localGap)
  {
    occt::moveShape(other, -dir, localGap);
    dist = getDistance(bIn, other);
    
    iter++;
    if (iter >= maxIter)
    {
      //exception ?
      std::ostringstream s; s << "warning: max iterations reached in Nest::calcPitch" << std::endl;
      lastUpdateLog += s.str();
      break;
    }
  }
  
  iter = 0;
  while (dist < localGap)
  {
    occt::moveShape(other, dir, tol); //move back so we are greater than gap.
    dist = getDistance(bIn, other);
    
    iter++;
    if (iter >= maxIter)
    {
      //exception ?
      std::ostringstream s; s << "warning: max iterations reached in Nest::calcPitch" << std::endl;
      lastUpdateLog += s.str();
      break;
    }
  }
  
  //set pitch
  gp_Vec pos1 = gp_Vec(bIn.Location().Transformation().TranslationPart());
  gp_Vec pos2 = gp_Vec(other.Location().Transformation().TranslationPart());
  double p = (pos1 - pos2).Magnitude();
  pitch->setValue(p);
  
  return other;
}

void Nest::updateModel(const UpdatePayload &payloadIn)
{
  setFailure();
  lastUpdateLog.clear();
  try
  {
    if (payloadIn.updateMap.count(Nest::blank) != 1)
      throw std::runtime_error("couldn't find 'blank' input");
    const ftr::Base *bbf = payloadIn.updateMap.equal_range(Nest::blank).first->second;
    if(!bbf->hasAnnex(ann::Type::SeerShape))
      throw std::runtime_error("no seer shape for blank");
    const ann::SeerShape &bss = bbf->getAnnex<ann::SeerShape>(ann::Type::SeerShape); //part seer shape.
    TopoDS_Shape bs = occt::getFirstNonCompound(bss.getRootOCCTShape()); //blank shape. not const, might mesh.
    
    /* this is a little unusual. For performance reasons, we are using
     * poly extrema for calculating pitch. Therefore the shapes need
     * triangulation done before this or they will fall back onto normal
     * extrema(slow). When update is called on the project we go through
     * and calculate all the model and then the viz. Long story short, we
     * don't have any triangulation in the blank shape. so manually call
     * update viz on it so we can use the poly extrema.
     */
    if (bbf->isVisualDirty())
    {
      double linear = prf::manager().rootPtr->visual().mesh().linearDeflection();
      double angular = prf::manager().rootPtr->visual().mesh().angularDeflection();
      BRepMesh_IncrementalMesh(bs, linear, Standard_False, angular, Standard_True);
    }
    //right now we are not consider the feed direction and just go with box length.
    //of course when we make the feed direction a parameter we will have to adjust.
    occt::BoundingBox bbox(bs); //use for both pitch calc and label location.
    TopoDS_Shape other = calcPitch(bs, bbox.getLength());
    
    occt::ShapeVector shapes;
    shapes.push_back(bs); //original part shape.
    shapes.push_back(other); //original blank shape.
    TopoDS_Shape out = static_cast<TopoDS_Compound>(occt::ShapeVectorCast(shapes));
    ShapeCheck check(out);
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    //No shape consistency yet.
    sShape->setOCCTShape(out);
    sShape->ensureNoNils();
    
    //update feed direction label. put at center of blank bounding box.
    feedDirectionLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(bbox.getCenter())));
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in nest update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in nest update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in nest update. " << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Nest::serialWrite(const QDir &dIn)
{
  prj::srl::FeatureNest so
  (
    Base::serialOut(),
    gap->serialOut(),
    feedDirection->serialOut(),
    gapLabel->serialOut(),
    feedDirectionLabel->serialOut(),
    static_cast<double>(*pitch)
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::nest(stream, so, infoMap);
}

void Nest::serialRead(const prj::srl::FeatureNest &sNestIn)
{
  Base::serialIn(sNestIn.featureBase());
  gap->serialIn(sNestIn.gap());
  feedDirection->serialIn(sNestIn.feedDirection());
  gapLabel->serialIn(sNestIn.gapLabel());
  feedDirectionLabel->serialIn(sNestIn.feedDirectionLabel());
  pitch->setValueQuiet(sNestIn.pitch());
}

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

#include <boost/filesystem.hpp>

#include <gp_Pnt.hxx>
#include <gp_Pln.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>

#include <osg/AutoTransform>
#include <osgQt/QFontImplementation>
#include <osgText/Text>

#include <libzippp.h>
#include <libreoffice/odshack.h>

#include <application/application.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <globalutilities.h>
#include <tools/occtools.h>
#include <annex/seershape.h>
#include <feature/shapecheck.h>
#include <feature/nest.h>
#include <project/serial/xsdcxxoutput/featurestrip.h>
#include <feature/strip.h>

using namespace ftr;

using boost::uuids::uuid;

QIcon Strip::icon;

TopoDS_Edge makeEdge(const osg::Vec3d &v1, const osg::Vec3d &v2)
{
  gp_Pnt p1(v1.x(), v1.y(), v1.z());
  gp_Pnt p2(v2.x(), v2.y(), v2.z());
  return BRepBuilderAPI_MakeEdge(p1, p2);
}

Strip::Strip() :
Base(),
feedDirection(new prm::Parameter(QObject::tr("Feed Direction"), osg::Vec3d(-1.0, 0.0, 0.0))),
pitch(new prm::Parameter(QObject::tr("Pitch"), 1.0)),
width(new prm::Parameter(QObject::tr("Width"), 1.0)),
widthOffset(new prm::Parameter(QObject::tr("Width Offset"), 1.0)),
gap(new prm::Parameter(QObject::tr("Gap"), prf::manager().rootPtr->features().strip().get().gap())),
autoCalc(new prm::Parameter(QObject::tr("Auto Calc"), true)),
sShape(new ann::SeerShape())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionStrip.svg");
  
  name = QObject::tr("Strip");
  mainSwitch->setUserValue(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
  
  feedDirection->connectValue(boost::bind(&Strip::setModelDirty, this));
  parameterVector.push_back(feedDirection.get());
  
  pitch->setConstraint(prm::Constraint::buildNonZeroPositive());
  pitch->connectValue(boost::bind(&Strip::setModelDirty, this));
  parameterVector.push_back(pitch.get());
  
  width->setConstraint(prm::Constraint::buildNonZeroPositive());
  width->connectValue(boost::bind(&Strip::setModelDirty, this));
  parameterVector.push_back(width.get());
  
  widthOffset->setConstraint(prm::Constraint::buildAll());
  widthOffset->connectValue(boost::bind(&Strip::setModelDirty, this));
  parameterVector.push_back(widthOffset.get());
  
  gap->setConstraint(prm::Constraint::buildNonZeroPositive());
  gap->connectValue(boost::bind(&Strip::setModelDirty, this));
  parameterVector.push_back(gap.get());
  
  autoCalc->connectValue(boost::bind(&Strip::setModelDirty, this));
  parameterVector.push_back(autoCalc.get());
  
  feedDirectionLabel = new lbr::PLabel(feedDirection.get());
  feedDirectionLabel->showName = true;
  feedDirectionLabel->valueHasChanged();
  overlaySwitch->addChild(feedDirectionLabel.get());
  
  pitchLabel = new lbr::PLabel(pitch.get());
  pitchLabel->showName = true;
  pitchLabel->valueHasChanged();
  overlaySwitch->addChild(pitchLabel.get());
  
  widthLabel = new lbr::PLabel(width.get());
  widthLabel->showName = true;
  widthLabel->valueHasChanged();
  overlaySwitch->addChild(widthLabel.get());
  
  widthOffsetLabel = new lbr::PLabel(widthOffset.get());
  widthOffsetLabel->showName = true;
  widthOffsetLabel->valueHasChanged();
  overlaySwitch->addChild(widthOffsetLabel.get());
  
  gapLabel = new lbr::PLabel(gap.get());
  gapLabel->showName = true;
  gapLabel->valueHasChanged();
  overlaySwitch->addChild(gapLabel.get());
  
  autoCalcLabel = new lbr::PLabel(autoCalc.get());
  autoCalcLabel->showName = true;
  autoCalcLabel->valueHasChanged();
  overlaySwitch->addChild(autoCalcLabel.get());
  
  stations.push_back("Blank");
  stations.push_back("Blank");
  stations.push_back("Blank");
  stations.push_back("Form");
}

Strip::~Strip(){}

//copied from lbr::PLabel::build
osg::Node* buildStationLabel(const std::string &sIn)
{
  const prf::InteractiveParameter& iPref = prf::manager().rootPtr->interactiveParameter();
  
  osg::AutoTransform *autoTransform = new osg::AutoTransform();
  autoTransform->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
  autoTransform->setAutoRotateMode(osg::AutoTransform::ROTATE_TO_SCREEN);
  autoTransform->setAutoScaleToScreen(true);
  
  osg::MatrixTransform *textScale = new osg::MatrixTransform();
  textScale->setMatrix(osg::Matrixd::scale(75.0, 75.0, 75.0));
  autoTransform->addChild(textScale);
  
  osgText::Text *text = new osgText::Text();
  text->setName(sIn);
  text->setText(sIn);
  osg::ref_ptr<osgQt::QFontImplementation> fontImplement(new osgQt::QFontImplementation(qApp->font()));
  osg::ref_ptr<osgText::Font> textFont(new osgText::Font(fontImplement.get()));
  text->setFont(textFont.get());
  text->setColor(osg::Vec4(1.0, 1.0, 1.0, 1.0));
  text->setBackdropType(osgText::Text::OUTLINE);
  text->setBackdropColor(osg::Vec4(0.0, 0.0, 0.0, 1.0));
  text->setCharacterSize(iPref.characterSize());
  text->setAlignment(osgText::Text::CENTER_CENTER);
  textScale->addChild(text);
  
  return autoTransform;
}

void Strip::updateModel(const UpdatePayload &payloadIn)
{
  setFailure();
  lastUpdateLog.clear();
  try
  {
    if (payloadIn.updateMap.count(part) != 1)
      throw std::runtime_error("couldn't find 'part' input");
    const ftr::Base *pbf = payloadIn.updateMap.equal_range(part).first->second;
    if(!pbf->hasAnnex(ann::Type::SeerShape))
      throw std::runtime_error("no seer shape for part");
    const ann::SeerShape &pss = pbf->getAnnex<ann::SeerShape>(ann::Type::SeerShape); //part seer shape.
    const TopoDS_Shape &ps = occt::getFirstNonCompound(pss.getRootOCCTShape()); //part shape.
      
    if (payloadIn.updateMap.count(blank) != 1)
      throw std::runtime_error("couldn't find 'blank' input");
    const ftr::Base *bbf = payloadIn.updateMap.equal_range(blank).first->second;
    if(!bbf->hasAnnex(ann::Type::SeerShape))
      throw std::runtime_error("no seer shape for blank");
    const ann::SeerShape &bss = bbf->getAnnex<ann::SeerShape>(ann::Type::SeerShape); //blank seer shape.
    const TopoDS_Shape &bs = occt::getFirstNonCompound(bss.getRootOCCTShape()); //blank shape.
      
    if (payloadIn.updateMap.count(nest) != 1)
      throw std::runtime_error("couldn't find 'nest' input");
    const ftr::Base *nbf = payloadIn.updateMap.equal_range(nest).first->second;
    
    occt::BoundingBox bbbox(bs); //blank bounding box.
    
    if (static_cast<bool>(*autoCalc))
    {
      osg::Vec4 cIn(1.0, 0.0, 0.0, 1.0);
      feedDirectionLabel->setTextColor(cIn);
      pitchLabel->setTextColor(cIn);
      widthLabel->setTextColor(cIn);
      widthOffsetLabel->setTextColor(cIn);
      gapLabel->setTextColor(cIn);
      
      const Nest *nf = dynamic_cast<const Nest *>(nbf);
      assert(nf);
      if (!nf)
        throw std::runtime_error("Bad cast to Nest");
      feedDirection->setValueQuiet(nf->getFeedDirection());
      pitch->setValueQuiet(nf->getPitch());
      pitchLabel->valueHasChanged();
      gap->setValueQuiet(nf->getGap());
      gapLabel->valueHasChanged();
      
      goAutoCalc(bs, bbbox);
    }
    else
    {
      feedDirectionLabel->setTextColor();
      pitchLabel->setTextColor();
      widthLabel->setTextColor();
      widthOffsetLabel->setTextColor();
      gapLabel->setTextColor();
    }
    
    occt::ShapeVector shapes;
    shapes.push_back(ps); //original part shape.
    shapes.push_back(bs); //original blank shape.
    //find first not blank index
    std::size_t nb = 0;
    for (std::size_t index = 0; index < stations.size(); ++index)
    {
      if (stations.at(index) != "Blank")
      {
        nb = index;
        break;
      }
    }
    
    osg::Vec3d lFeed = static_cast<osg::Vec3d>(*feedDirection);
    osg::Vec3d fNorm = lFeed * osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(0.0, 0.0, -1.0));
      
    for (std::size_t i = 1; i < nb + 1; ++i) //blanks
      shapes.push_back(occt::instanceShape(bs, gu::toOcc(-lFeed), static_cast<double>(*pitch) * i));
    for (std::size_t i = 1; i < stations.size() - nb; ++i) //parts
      shapes.push_back(occt::instanceShape(ps, gu::toOcc(lFeed), static_cast<double>(*pitch) * i));
    
    //add edges representing the incoming strip.
    double edgeLength = nb * static_cast<double>(*pitch);
    osg::Vec3d centerLinePoint = fNorm * static_cast<double>(*widthOffset);
    centerLinePoint += lFeed * (gu::toOsg(bbbox.getCenter()) * lFeed);
    
    osg::Vec3d backLinePoint = centerLinePoint + (fNorm * static_cast<double>(*width) / 2.0);
    osg::Vec3d backLineEnd1 = backLinePoint;
    osg::Vec3d backLineEnd2 = backLinePoint + (-lFeed * edgeLength);
    shapes.push_back(makeEdge(backLineEnd1, backLineEnd2));
    
    osg::Vec3d frontLinePoint = centerLinePoint + (-fNorm * static_cast<double>(*width) / 2.0);
    osg::Vec3d frontLineEnd1 = frontLinePoint;
    osg::Vec3d frontLineEnd2 = frontLinePoint + (-lFeed * edgeLength);
    shapes.push_back(makeEdge(frontLineEnd1, frontLineEnd2));
    
    TopoDS_Shape out = static_cast<TopoDS_Compound>(occt::ShapeVectorCast(shapes));
    ShapeCheck check(out);
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    //for now, we are only going to have consistent ids for face and outer wire.
    sShape->setOCCTShape(out);
    sShape->ensureNoNils();
    
    
    //update label locations
    osg::Vec3d plLoc = //pitch label location.
      gu::toOsg(bbbox.getCenter())
      + (-lFeed * static_cast<double>(*pitch) * 0.5)
      + (fNorm * bbbox.getWidth() * 0.5);
    pitchLabel->setMatrix(osg::Matrixd::translate(plLoc));
    
    osg::Vec3d glLoc = //gap label location.
      gu::toOsg(bbbox.getCenter())
      + (-lFeed * static_cast<double>(*pitch) * 0.5)
      + (-fNorm * bbbox.getWidth() * 0.5);
    gapLabel->setMatrix(osg::Matrixd::translate(glLoc));
    
    osg::Vec3d wolLoc = //width offset location.
      gu::toOsg(bbbox.getCenter())
      + (-lFeed * (static_cast<double>(*pitch) * (static_cast<double>(nb) + 0.5)));
    widthOffsetLabel->setMatrix(osg::Matrixd::translate(wolLoc));
    
    osg::Vec3d wlLoc = //width location.
      gu::toOsg(bbbox.getCenter())
      + (-lFeed * (static_cast<double>(*pitch) * (static_cast<double>(nb) + 0.5)))
      + (fNorm * bbbox.getWidth() * 0.5);
    widthLabel->setMatrix(osg::Matrixd::translate(wlLoc));
    
    osg::Vec3d acLoc = gu::toOsg(bbbox.getCenter()); //autocalc label location
    autoCalcLabel->setMatrix(osg::Matrixd::translate(acLoc + osg::Vec3d(0.0, bbbox.getWidth(), 0.0)));
    
    osg::Vec3d fdLoc = gu::toOsg(bbbox.getCenter()) + osg::Vec3d(0.0, -bbbox.getWidth(), 0.0);
    feedDirectionLabel->setMatrix(osg::Matrixd::translate(fdLoc));
    
    for (const auto &l : stationLabels)
    {
      for (const auto &pg : l->getParents()) //parent group
        pg->removeChild(l.get());
    }
    stationLabels.clear();
    bool cv = overlaySwitch->getValue(0); //overlaySwitch always has at least four children.
    for (std::size_t i = 1; i < nb + 1; ++i)
    {
      osg::ref_ptr<osg::MatrixTransform> sl = new osg::MatrixTransform(); // station label
      sl->setMatrix(osg::Matrixd::translate(gu::toOsg(bbbox.getCenter()) + (-lFeed * static_cast<double>(*pitch) * i)));
      sl->addChild(buildStationLabel("Blank"));
      stationLabels.push_back(sl);
      overlaySwitch->addChild(sl.get(), cv);
    }
    for (std::size_t i = nb; i < stations.size(); ++i)
    {
      osg::ref_ptr<osg::MatrixTransform> sl = new osg::MatrixTransform(); // station label
      sl->setMatrix(osg::Matrixd::translate(gu::toOsg(bbbox.getCenter()) + (lFeed * static_cast<double>(*pitch) * (i - nb))));
      sl->addChild(buildStationLabel(stations.at(i).toStdString()));
      stationLabels.push_back(sl);
      overlaySwitch->addChild(sl.get(), cv);
    }
    
    //boundingbox of whole strip. used for travel estimation.
    occt::BoundingBox sbb(out);
    stripHeight = sbb.getHeight();
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in strip update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in strip update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in strip update. " << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Strip::goAutoCalc(const TopoDS_Shape &sIn, occt::BoundingBox &bbbox)
{
  double offset = bbbox.getDiagonal() / 2.0;
  osg::Vec3d feed = static_cast<osg::Vec3d>(*feedDirection);
  osg::Vec3d norm = feed * osg::Matrixd::rotate(osg::PI_2, osg::Vec3d(0.0, 0.0, -1.0));
  
  gp_Ax3 orientation(gp_Pnt(0.0, 0.0, 0.0), gu::toOcc(norm), gu::toOcc(feed));
  gp_Pln plane(orientation);
  TopoDS_Face face1 = BRepBuilderAPI_MakeFace(plane, -offset, offset, -offset, offset);
//   double linear = prf::manager().rootPtr->visual().mesh().linearDeflection();
//   double angular = prf::manager().rootPtr->visual().mesh().angularDeflection();
//   BRepMesh_IncrementalMesh(face1, linear, Standard_False, angular, Standard_True);
//   BRepMesh_IncrementalMesh(sIn, linear, Standard_False, angular, Standard_True);
  
  gp_Trsf t1;
  t1.SetTranslation(gu::toOcc(gu::toOsg(bbbox.getCenter()) + (norm * offset)));
  face1.Location(TopLoc_Location(t1));
  
  TopoDS_Face face2 = face1;
  gp_Trsf t2;
  t2.SetTranslation(gu::toOcc(gu::toOsg(bbbox.getCenter()) + (norm * -offset)));
  face2.Location(TopLoc_Location(t2));
  
  auto getDistance = [](const TopoDS_Shape &s1, const TopoDS_Shape &s2) -> double
  {
    //shape
    double tol = 0.1;
    BRepExtrema_DistShapeShape dc(s1, s2, tol, Extrema_ExtFlag_MIN);
    if (!dc.IsDone())
      throw std::runtime_error("BRepExtrema_DistShapeShape failed");;
    if (dc.NbSolution() < 1)
      throw std::runtime_error("BRepExtrema_DistShapeShape failed");;
    return dc.Value();
    
    
    /* I couldn't get poly to work. Not sure why.
     * I am guessing it had to do with orthogonal planes
     */
//     gp_Pnt p1, p2;
//     double distance;
//     if (BRepExtrema_Poly::Distance(s1, s2, p1, p2, distance))
//       return distance;
//     
//     throw std::runtime_error("BRepExtrema_Poly failed");
  };
  
  double d1 = getDistance(sIn, face1);
  double d2 = getDistance(sIn, face2);
  double widthCalc = 2 * offset - d1 - d2 + 2 * static_cast<double>(*gap);
  width->setValueQuiet(widthCalc);
  widthLabel->valueHasChanged();
  
  osg::Vec3d projection = (norm * (offset - d1)) + (-norm * (offset - d2));
  osg::Vec3d center = gu::toOsg(bbbox.getCenter()) + (projection * 0.5);
  osg::Vec3d aux = feed * (feed * center);
  osg::Vec3d auxProjection = center - aux;
  double directionFactor = ((auxProjection * norm) < 0.0) ? -1.0 : 1.0;
  widthOffset->setValueQuiet(auxProjection.length() * directionFactor);
  widthOffsetLabel->valueHasChanged();
}

void Strip::serialWrite(const QDir &dIn)
{
  assert(stations.size() == stationLabels.size());
  prj::srl::FeatureStrip::StationsType so;
  for (std::size_t index = 0; index < stations.size(); ++index)
  {
    const osg::Matrixd &m = stationLabels.at(index)->getMatrix();
    prj::srl::Matrixd mOut
    (
      m(0,0), m(0,1), m(0,2), m(0,3),
      m(1,0), m(1,1), m(1,2), m(1,3),
      m(2,0), m(2,1), m(2,2), m(2,3),
      m(3,0), m(3,1), m(3,2), m(3,3)
    );
    
    prj::srl::Station stationOut
    (
      stations.at(index).toStdString(),
      mOut
    );
    so.array().push_back(stationOut);
  }
  
  prj::srl::FeatureStrip stripOut
  (
    Base::serialOut(),
    feedDirection->serialOut(),
    pitch->serialOut(),
    width->serialOut(),
    widthOffset->serialOut(),
    gap->serialOut(),
    autoCalc->serialOut(),
    stripHeight,
    feedDirectionLabel->serialOut(),
    pitchLabel->serialOut(),
    widthLabel->serialOut(),
    widthOffsetLabel->serialOut(),
    gapLabel->serialOut(),
    autoCalcLabel->serialOut(),
    so
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::strip(stream, stripOut, infoMap);
}

void Strip::serialRead(const prj::srl::FeatureStrip &sIn)
{
  Base::serialIn(sIn.featureBase());
  feedDirection->serialIn(sIn.feedDirection());
  pitch->serialIn(sIn.pitch());
  width->serialIn(sIn.width());
  widthOffset->serialIn(sIn.widthOffset());
  gap->serialIn(sIn.gap());
  autoCalc->serialIn(sIn.autoCalc());
  stripHeight = sIn.stripHeight();
  feedDirectionLabel->serialIn(sIn.feedDirectionLabel());
  pitchLabel->serialIn(sIn.pitchLabel());
  widthLabel->serialIn(sIn.widthLabel());
  widthOffsetLabel->serialIn(sIn.widthOffsetLabel());
  gapLabel->serialIn(sIn.gapLabel());
  autoCalcLabel->serialIn(sIn.autoCalcLabel());
  
  stations.clear();
  stationLabels.clear();
  for (const auto &stationIn : sIn.stations().array())
  {
    stations.push_back(QString::fromStdString(stationIn.text()));
    
    osg::ref_ptr<osg::MatrixTransform> sl = new osg::MatrixTransform();
    const auto &mIn = stationIn.matrix();
    osg::Matrixd position
    (
      mIn.i0j0(), mIn.i0j1(), mIn.i0j2(), mIn.i0j3(),
      mIn.i1j0(), mIn.i1j1(), mIn.i1j2(), mIn.i1j3(),
      mIn.i2j0(), mIn.i2j1(), mIn.i2j2(), mIn.i2j3(),
      mIn.i3j0(), mIn.i3j1(), mIn.i3j2(), mIn.i3j3()
    );
    sl->setMatrix(position);
    sl->addChild(buildStationLabel(stationIn.text()));
    stationLabels.push_back(sl);
    overlaySwitch->addChild(sl.get(), overlaySwitch->getValue(0));
  }
}

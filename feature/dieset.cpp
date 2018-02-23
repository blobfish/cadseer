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

#include <osg/Switch>

#include <globalutilities.h>
#include <library/plabel.h>
#include <tools/occtools.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <feature/parameter.h>
#include <annex/seershape.h>
#include <feature/shapecheck.h>
#include <feature/boxbuilder.h>
#include <project/serial/xsdcxxoutput/featuredieset.h>
#include <feature/inputtype.h>
#include <feature/updatepayload.h>
#include <feature/dieset.h>

using namespace ftr;

using boost::uuids::uuid;

QIcon DieSet::icon;

DieSet::DieSet() : Base(), sShape(new ann::SeerShape())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionDieSet.svg");
  
  name = QObject::tr("DieSet");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  length = std::shared_ptr<prm::Parameter>(new prm::Parameter(prm::Names::Length, 1.0));
  length->setConstraint(prm::Constraint::buildNonZeroPositive());
  length->connectValue(boost::bind(&DieSet::setModelDirty, this));
  parameters.push_back(length.get());
  
  width = std::shared_ptr<prm::Parameter>(new prm::Parameter(prm::Names::Width, 1.0));
  width->setConstraint(prm::Constraint::buildNonZeroPositive());
  width->connectValue(boost::bind(&DieSet::setModelDirty, this));
  parameters.push_back(width.get());
  
  lengthPadding = std::shared_ptr<prm::Parameter>
  (
    new prm::Parameter
    (
      QObject::tr("Length Padding"),
      prf::manager().rootPtr->features().dieset().get().lengthPadding()
    )
  );
  lengthPadding->setConstraint(prm::Constraint::buildZeroPositive());
  lengthPadding->connectValue(boost::bind(&DieSet::setModelDirty, this));
  parameters.push_back(lengthPadding.get());
  
  widthPadding = std::shared_ptr<prm::Parameter>
  (
    new prm::Parameter
    (
      QObject::tr("Width Padding"),
      prf::manager().rootPtr->features().dieset().get().widthPadding()
    )
  );
  widthPadding->setConstraint(prm::Constraint::buildZeroPositive());
  widthPadding->connectValue(boost::bind(&DieSet::setModelDirty, this));
  parameters.push_back(widthPadding.get());
  
  origin = std::shared_ptr<prm::Parameter>
  (
    new prm::Parameter
    (
      QObject::tr("Origin"),
      osg::Vec3d(-1.0, -1.0, -1.0)
    )
  );
  origin->connectValue(boost::bind(&DieSet::setModelDirty, this));
  parameters.push_back(origin.get());
  
  autoCalc = std::shared_ptr<prm::Parameter>
  (
    new prm::Parameter
    (
      QObject::tr("Auto Calc"),
      true
    )
  );
  autoCalc->connectValue(boost::bind(&DieSet::setModelDirty, this));
  parameters.push_back(autoCalc.get());
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
  
  lengthLabel = new lbr::PLabel(length.get());
  lengthLabel->showName = true;
  lengthLabel->valueHasChanged();
  overlaySwitch->addChild(lengthLabel.get());
  
  widthLabel = new lbr::PLabel(width.get());
  widthLabel->showName = true;
  widthLabel->valueHasChanged();
  overlaySwitch->addChild(widthLabel.get());
  
  lengthPaddingLabel = new lbr::PLabel(lengthPadding.get());
  lengthPaddingLabel->showName = true;
  lengthPaddingLabel->valueHasChanged();
  overlaySwitch->addChild(lengthPaddingLabel.get());
  
  widthPaddingLabel = new lbr::PLabel(widthPadding.get());
  widthPaddingLabel->showName = true;
  widthPaddingLabel->valueHasChanged();
  overlaySwitch->addChild(widthPaddingLabel.get());
  
  originLabel = new lbr::PLabel(origin.get());
  originLabel->showName = true;
  originLabel->valueHasChanged();
  overlaySwitch->addChild(originLabel.get());
  
  autoCalcLabel = new lbr::PLabel(autoCalc.get());
  autoCalcLabel->showName = true;
  autoCalcLabel->valueHasChanged();
  overlaySwitch->addChild(autoCalcLabel.get());
}

DieSet::~DieSet()
{
}

double DieSet::getLength() const
{
  return static_cast<double>(*length);
}

double DieSet::getWidth() const
{
  return static_cast<double>(*width);
}

void DieSet::updateModel(const UpdatePayload &payloadIn)
{
  setFailure();
  lastUpdateLog.clear();
  sShape->reset();
  try
  {
    double h = 50.0; //height
    double zPadding = 50.0; //distance from bottom of bounding box to top of set.
    if (static_cast<bool>(*autoCalc))
    {
      if (payloadIn.updateMap.count(strip) != 1)
        throw std::runtime_error("couldn't find 'strip' input");
      const ftr::Base *sbf = payloadIn.updateMap.equal_range(strip).first->second;
      if(!sbf->hasAnnex(ann::Type::SeerShape))
        throw std::runtime_error("no seer shape for strip");
      const ann::SeerShape &sss = sbf->getAnnex<ann::SeerShape>(ann::Type::SeerShape); //part seer shape.
      const TopoDS_Shape &ss = sss.getRootOCCTShape(); //part shape.
      
      occt::BoundingBox sbbox(ss); //blank bounding box.
      
      gp_Pnt bbc = sbbox.getCorners().front();
      gp_Vec xVec(-static_cast<double>(*lengthPadding), 0.0, 0.0);
      gp_Vec yVec(0.0, -static_cast<double>(*widthPadding), 0.0);
      gp_Vec zVec(0.0, 0.0, -zPadding - h);
      
      gp_Pnt corner = bbc.Translated(xVec).Translated(yVec).Translated(zVec);
      origin->setValueQuiet(osg::Vec3d(corner.X(), corner.Y(), corner.Z()));
      originLabel->valueHasChanged();
      double l = sbbox.getLength() + 2 * static_cast<double>(*lengthPadding);
      double w = sbbox.getWidth() + 2 * static_cast<double>(*widthPadding);
      length->setValueQuiet(l);
      lengthLabel->valueHasChanged();
      width->setValueQuiet(w);
      widthLabel->valueHasChanged();
    }
    
    //assumptions on orientation.
    osg::Vec3d to = static_cast<osg::Vec3d>(*origin); //temp origin.
    gp_Ax2 sys(gp_Pnt(to.x(), to.y(), to.z()), gp_Dir(0.0, 0.0, 1.0), gp_Dir(1.0, 0.0, 0.0));
    BoxBuilder b(static_cast<double>(*length), static_cast<double>(*width), h, sys);
    TopoDS_Shape out = b.getSolid();
    ShapeCheck check(out);
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    //No shape consistency yet.
    sShape->setOCCTShape(out);
    sShape->ensureNoNils();
    
    //update label locations
    osg::Vec3d lLoc = //length label location.
      static_cast<osg::Vec3d>(*origin)
      + osg::Vec3d(1.0, 0.0, 0.0) * static_cast<double>(*length) / 2.0
      + osg::Vec3d(0.0, 0.0, 1.0) * h;
    lengthLabel->setMatrix(osg::Matrixd::translate(lLoc));
    
    osg::Vec3d wLoc = //length label location.
      static_cast<osg::Vec3d>(*origin)
      + osg::Vec3d(1.0, 0.0, 0.0) * static_cast<double>(*length)
      + osg::Vec3d(0.0, 1.0, 0.0) * static_cast<double>(*width) / 2.0
      + osg::Vec3d(0.0, 0.0, 1.0) * h;
    widthLabel->setMatrix(osg::Matrixd::translate(wLoc));
    
    osg::Vec3d lpLoc = //length padding label location.
      static_cast<osg::Vec3d>(*origin)
      + osg::Vec3d(0.0, 1.0, 0.0) * static_cast<double>(*width) / 2.0
      + osg::Vec3d(0.0, 0.0, 1.0) * h;
    lengthPaddingLabel->setMatrix(osg::Matrixd::translate(lpLoc));
    
    osg::Vec3d wpLoc = //width padding label location.
      static_cast<osg::Vec3d>(*origin)
      + osg::Vec3d(1.0, 0.0, 0.0) * static_cast<double>(*length) / 2.0
      + osg::Vec3d(0.0, 1.0, 0.0) * static_cast<double>(*width)
      + osg::Vec3d(0.0, 0.0, 1.0) * h;
    widthPaddingLabel->setMatrix(osg::Matrixd::translate(wpLoc));
    
    originLabel->setMatrix(osg::Matrixd::translate(static_cast<osg::Vec3d>(*origin)));
    
    osg::Vec3d acLoc = //auto calc label location
      static_cast<osg::Vec3d>(*origin)
      + osg::Vec3d(static_cast<double>(*length) / 2.0, 0.0, 0.0)
      + osg::Vec3d(0.0, static_cast<double>(*width) / 2.0, 0.0);
    autoCalcLabel->setMatrix(osg::Matrixd::translate(acLoc));
    
    updateLabelColors();
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in DieSet update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in DieSet update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in DieSet update. " << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void DieSet::updateLabelColors()
{
  if (static_cast<bool>(*autoCalc))
  {
    //red while auto calculation.
    lengthLabel->setTextColor(osg::Vec4(1.0, 0.0, 0.0, 1.0));
    widthLabel->setTextColor(osg::Vec4(1.0, 0.0, 0.0, 1.0));
    originLabel->setTextColor(osg::Vec4(1.0, 0.0, 0.0, 1.0));
    
    lengthPaddingLabel->setTextColor();
    widthPaddingLabel->setTextColor();
    
  }
  else
  {
    lengthLabel->setTextColor();
    widthLabel->setTextColor();
    originLabel->setTextColor();
    
    lengthPaddingLabel->setTextColor(osg::Vec4(1.0, 0.0, 0.0, 1.0));
    widthPaddingLabel->setTextColor(osg::Vec4(1.0, 0.0, 0.0, 1.0));
  }
}

void DieSet::serialWrite(const QDir &dIn)
{
  prj::srl::FeatureDieSet dso
  (
    Base::serialOut(),
    length->serialOut(),
    lengthPadding->serialOut(),
    width->serialOut(),
    widthPadding->serialOut(),
    origin->serialOut(),
    autoCalc->serialOut(),
    lengthLabel->serialOut(),
    widthLabel->serialOut(),
    lengthPaddingLabel->serialOut(),
    widthPaddingLabel->serialOut(),
    originLabel->serialOut(),
    autoCalcLabel->serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::dieset(stream, dso, infoMap);
}

void DieSet::serialRead(const prj::srl::FeatureDieSet &dsIn)
{
  Base::serialIn(dsIn.featureBase());
  length->serialIn(dsIn.length());
  lengthPadding->serialIn(dsIn.lengthPadding());
  width->serialIn(dsIn.width());
  widthPadding->serialIn(dsIn.widthPadding());
  origin->serialIn(dsIn.origin());
  autoCalc->serialIn(dsIn.autoCalc());
  
  lengthLabel->serialIn(dsIn.lengthPLabel());
  lengthPaddingLabel->serialIn(dsIn.lengthPaddingPLabel());
  widthLabel->serialIn(dsIn.widthPLabel());
  widthPaddingLabel->serialIn(dsIn.widthPaddingPLabel());
  originLabel->serialIn(dsIn.originPLabel());
  autoCalcLabel->serialIn(dsIn.autoCalcPLabel());
}

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

#include <TopoDS.hxx>
#include <BRepTools.hxx>

#include <globalutilities.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <tools/occtools.h>
#include <squash/squash.h>
#include <feature/seershape.h>
#include <feature/shapecheck.h>
#include <feature/squash.h>
#include <project/serial/xsdcxxoutput/featuresquash.h>

using namespace ftr;

using boost::uuids::uuid;

QIcon Squash::icon;

Squash::Squash() : Base()
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionSquash.svg");
  
  name = QObject::tr("Squash");
  mainSwitch->setUserValue(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  faceId = gu::createRandomId();
  wireId = gu::createRandomId();
  
  granularity = std::shared_ptr<prm::Parameter>
  (
    new prm::Parameter
    (
      QObject::tr("Granularity"),
      static_cast<double>(prf::manager().rootPtr->features().squash().get().granularity())
    )
  );
  prm::Boundary lower(0.0, prm::Boundary::End::Closed); // 0.0 means no update
  prm::Boundary upper(5.0, prm::Boundary::End::Closed);
  prm::Interval interval(lower, upper);
  prm::Constraint c;
  c.intervals.push_back(interval);
  granularity->setConstraint(c);
  
  granularity->connectValue(boost::bind(&Squash::setModelDirty, this));
  parameterVector.push_back(granularity.get());
  
  label = new lbr::PLabel(granularity.get());
  label->valueHasChanged();
  overlaySwitch->addChild(label.get());
}

int Squash::getGranularity()
{
  return static_cast<int>(granularity->getValue());
}

void Squash::setGranularity(int vIn)
{
  granularity->setValue(static_cast<double>(vIn));
}

void Squash::updateModel(const UpdatePayload &payloadIn)
{
  if (granularity->getValue() == 0.0)
  {
    setSuccess();
    setModelClean();
    return;
  }
  
  overlaySwitch->removeChildren(0, overlaySwitch->getNumChildren());
  overlaySwitch->addChild(label.get());
  
  setFailure();
  try
  {
    if (payloadIn.updateMap.count(InputType::target) != 1)
      throw std::runtime_error("wrong number of parents");
    
    const ftr::Base *bf = payloadIn.updateMap.equal_range(InputType::target).first->second;
    if(!bf->hasSeerShape())
      throw std::runtime_error("no seer shape");
      
    const SeerShape &targetSeerShape = bf->getSeerShape();
    
    //get the shell
    TopoDS_Shape ss = occt::getFirstNonCompound(targetSeerShape.getRootOCCTShape());
    if (ss.IsNull())
      throw std::runtime_error("Shape is null");
    if (ss.ShapeType() != TopAbs_SHELL)
      throw std::runtime_error("Shape is not a shell");
    TopoDS_Shell s = TopoDS::Shell(ss);
    
    //get the holding faces.
    occt::FaceVector fs;
    bool od = false; //orientation determined.
    bool sr = false; //should reverse.
    for (const auto &p : picks)
    {
      std::vector<uuid> cps = targetSeerShape.resolvePick(p.shapeHistory);
      for (const auto &lid : cps)
      {
        const TopoDS_Shape &shape = targetSeerShape.getOCCTShape(lid);
        if (shape.ShapeType() != TopAbs_FACE)
        {
          std::cout << "shape is not a face in Squash::updateModel" << std::endl;
          continue;
        }
        const TopoDS_Face &f = TopoDS::Face(shape);
        static const gp_Vec zAxis(0.0, 0.0, 1.0);
        gp_Vec n = occt::getNormal(f, p.u, p.v);
        if (!n.IsParallel(zAxis, Precision::Confusion())) //Precision::Angular was too sensitive.
          throw std::runtime_error("lock face that is not parallel to z axis");
        if (!od)
        {
          od = true;
          if (n.IsOpposite(zAxis, Precision::Angular()))
            sr = true;
        }
        if (sr)
          fs.push_back(TopoDS::Face(f.Reversed())); //might not need to reverse these?
        else
          fs.push_back(f);
      }
    }
    if (fs.empty())
      throw std::runtime_error("No holding faces");
    if (sr)
      s.Reverse();
    
    sqs::Parameters ps(s, fs);
    ps.granularity = static_cast<int>(granularity->getValue());
    sqs::squash(ps);
    TopoDS_Face out = ps.ff;
    if(ps.mesh3d)
      overlaySwitch->insertChild(0, ps.mesh3d.get());
    if(ps.mesh2d)
      overlaySwitch->insertChild(0, ps.mesh2d.get());
    
    if (!out.IsNull())
    {
      ShapeCheck check(out);
      if (!check.isValid())
        throw std::runtime_error("shapeCheck failed");
      
      //for now, we are only going to have consistent ids for face and outer wire.
      seerShape->setOCCTShape(out);
      seerShape->updateShapeIdRecord(out, faceId);
      const TopoDS_Shape &ow = BRepTools::OuterWire(out);
      seerShape->updateShapeIdRecord(ow, wireId);
      seerShape->ensureNoNils();
      
      setSuccess();
    }
    else
    {
      //this is incase face is bad, this should show something.
      //we don't really care about id evolution with these edges.
      TopoDS_Compound c = occt::ShapeVectorCast(ps.es);
      seerShape->setOCCTShape(out);
      seerShape->ensureNoNils();
    }
  }
  catch (const Standard_Failure &e)
  {
    std::cout << std::endl << "Error in squash update. " << e.GetMessageString() << std::endl;
  }
  catch (std::exception &e)
  {
    std::cout << std::endl << "Error in squash update. " << e.what() << std::endl;
  }
  catch (...)
  {
    std::cout << std::endl << "Unknown error in squash update. " << std::endl;
  }
  setModelClean();
}

void Squash::serialWrite(const QDir &dIn)
{
  prj::srl::FeatureSquash so
  (
    Base::serialOut(),
    ::ftr::serialOut(picks),
    gu::idToString(faceId),
    gu::idToString(wireId),
    granularity->serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::squash(stream, so, infoMap);
}

void Squash::serialRead(const prj::srl::FeatureSquash &sSquashIn)
{
  Base::serialIn(sSquashIn.featureBase());
  picks = ::ftr::serialIn(sSquashIn.picks());
  faceId = gu::stringToId(sSquashIn.faceId());
  wireId = gu::stringToId(sSquashIn.wireId());
  granularity->serialIn(sSquashIn.granularity());
}

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

#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepExtrema_Poly.hxx>
#include <BRepMesh_IncrementalMesh.hxx>

#include <libzippp.h>
#include <libreoffice/odshack.h>

#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <globalutilities.h>
#include <tools/occtools.h>
#include <feature/seershape.h>
#include <feature/shapecheck.h>
#include <feature/strip.h>

using namespace ftr;

using boost::uuids::uuid;

QIcon Strip::icon;

Strip::Strip() : Base()
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionStrip.svg");
  
  name = QObject::tr("Strip");
  mainSwitch->setUserValue(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  pitch = std::shared_ptr<Parameter>(new Parameter(QObject::tr("Pitch"), 0.0));
  pitch->setConstraint(ParameterConstraint::buildZeroPositive());
  
  pitch->connectValue(boost::bind(&Strip::setModelDirty, this));
  parameterVector.push_back(pitch.get());
  
  pitchLabel = new lbr::PLabel(pitch.get());
  pitchLabel->valueHasChanged();
  overlaySwitch->addChild(pitchLabel.get());
  
  
  stripData.quoteNumber = 69;
  stripData.customerName = "aCustomer";
  stripData.customerId = gu::createRandomId();
  stripData.partName = "Bracket";
  stripData.partNumber = "C66950";
  stripData.partSetup = "One Out";
  stripData.partRevision = "r69";
  stripData.materialType = "J2340 490X";
  stripData.materialThickness = 1.2;
  stripData.processType = "Prog";
  stripData.annualVolume = 200000;
  
  stripData.stations.push_back("Blank");
  stripData.stations.push_back("Blank");
  stripData.stations.push_back("Blank");
  stripData.stations.push_back("Form");
  stripData.stations.push_back("Form");
  stripData.stations.push_back("Form");
  stripData.stations.push_back("Form");
  stripData.stations.push_back("Form");
}

TopoDS_Shape instanceShape(const TopoDS_Shape sIn, const gp_Vec &dir, double distance)
{
  gp_Trsf move;
  move.SetTranslation(dir * distance);
  TopLoc_Location movement(move);
  
  return sIn.Moved(movement);
}

double getDistance(const TopoDS_Shape &sIn1, const TopoDS_Shape &sIn2)
{
  gp_Pnt p1, p2;
  double distance;
  if (BRepExtrema_Poly::Distance(sIn1, sIn2, p1, p2, distance))
    return distance;
  
  //this shouldn't ever be run as we ensure the poly/mesh before calling.
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

void moveShape(TopoDS_Shape &sIn, const gp_Vec &dir, double distance)
{
  gp_Trsf move;
  move.SetTranslation(dir * distance);
  TopLoc_Location movement(move);
  sIn.Move(movement);
}

double getPitch(TopoDS_Shape &bIn, const gp_Vec &dir, double gap, double guess, double round = 5.0)
{
  //guess is expected from the bounding box and assumes no overlap.
  //dir is a unit vector.
  double tol = gap * 0.1;
  TopoDS_Shape other = instanceShape(bIn, dir, guess + gap + tol);
  
  double dist = getDistance(bIn, other);
  if (dist == -1.0)
    throw std::runtime_error("couldn't get start position in getPitch");
  
  int maxIter = 100;
  int iter = 0;
  while (dist > gap)
  {
    moveShape(other, -dir, gap);
    dist = getDistance(bIn, other);
    
    iter++;
    if (iter >= maxIter)
    {
      //exception ?
      std::cout << "warning: max iterations reached in Strip::getPitch" << std::endl;
      break;
    }
  }
  
  iter = 0;
  while (dist < gap)
  {
    moveShape(other, dir, tol); //move back so we are greater than gap.
    dist = getDistance(bIn, other);
    
    iter++;
    if (iter >= maxIter)
    {
      //exception ?
      std::cout << "warning: max iterations reached in Strip::getPitch" << std::endl;
      break;
    }
  }
  
  gp_Vec pos1 = gp_Vec(bIn.Location().Transformation().TranslationPart());
  gp_Vec pos2 = gp_Vec(other.Location().Transformation().TranslationPart());
  double pitch = (pos1 - pos2).Magnitude();

  //round to nearest.
  int whole = static_cast<int>(pitch / round);
  if ((pitch / round - static_cast<double>(whole)) > 0.5)
    whole++;
  
  return static_cast<double>(whole * round);
}

void Strip::updateModel(const UpdatePayload &payloadIn)
{
  setFailure();
  try
  {
    if (payloadIn.updateMap.count(Strip::part) != 1)
      throw std::runtime_error("couldn't find 'part' input");
    const ftr::Base *pbf = payloadIn.updateMap.equal_range(Strip::part).first->second;
    if(!pbf->hasSeerShape())
      throw std::runtime_error("no seer shape for part");
    const SeerShape &pss = pbf->getSeerShape(); //part seer shape.
    const TopoDS_Shape &ps = pss.getRootOCCTShape(); //part shape.
      
    if (payloadIn.updateMap.count(Strip::blank) != 1)
      throw std::runtime_error("couldn't find 'blank' input");
    const ftr::Base *bbf = payloadIn.updateMap.equal_range(Strip::blank).first->second;
    if(!bbf->hasSeerShape())
      throw std::runtime_error("no seer shape for blank");
    const SeerShape &bss = bbf->getSeerShape(); //part seer shape.
    TopoDS_Shape bs = bss.getRootOCCTShape(); //blank shape. not const, might mesh.
    occt::BoundingBox bbox(bs); //use for both pitch calc and label location.
    
    //update lable location
    pitchLabel->setMatrix(osg::Matrixd::translate(gu::toOsg(bbox.getCenter())));
    
    gp_Vec dir(-1.0, 0.0, 0.0);
    double pitchValue = pitch->getValue();
    if (pitchValue == 0.0)
    {
      /* this is a little unusual. For performance reasons, we are using
       * poly extrema for calculating pitch. There for the shapes need
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
      pitchValue = getPitch(bs, dir, 6.0, bbox.getLength(), 2.0); //length vs dir?
      //todo set pitch parameter quietly to calculated value.
    }
    
    occt::ShapeVector shapes;
    shapes.push_back(ps); //original part shape.
    shapes.push_back(bs); //original blank shape.
    //find first not blank index
    std::size_t nb = 0;
    for (std::size_t index = 0; index < stripData.stations.size(); ++index)
    {
      if (stripData.stations.at(index) != "Blank")
      {
        nb = index;
        break;
      }
    }
      
    //for now lets just add 3 blanks and 5 formed parts.
    for (std::size_t i = 1; i < nb + 1; ++i)
      shapes.push_back(instanceShape(bs, -dir, pitchValue * i));
    for (std::size_t i = 1; i < stripData.stations.size() - nb; ++i)
      shapes.push_back(instanceShape(ps, dir, pitchValue * i));
    
    
    TopoDS_Shape out = static_cast<TopoDS_Compound>(occt::ShapeVectorCast(shapes));
    
    ShapeCheck check(out);
    if (!check.isValid())
      throw std::runtime_error("shapeCheck failed");
    
    //for now, we are only going to have consistent ids for face and outer wire.
    seerShape->setOCCTShape(out);
    seerShape->ensureNoNils();
    
    exportSheet();
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::cout << std::endl << "Error in strip update. " << e.GetMessageString() << std::endl;
  }
  catch (std::exception &e)
  {
    std::cout << std::endl << "Error in strip update. " << e.what() << std::endl;
  }
  catch (...)
  {
    std::cout << std::endl << "Unknown error in strip update. " << std::endl;
  }
  setModelClean();
}

void Strip::exportSheet()
{
  /* we change the entries in a specific sheet and these values are linked into
   * a customer designed sheet. The links are not updated when we open the sheet in calc by default.
   * calc menu: /tools/options/libreoffice calc/formula/Recalculation on file load
   * set that to always.
   */
  boost::filesystem::path o("/home/tanderson/Programming/cadseer/QuoteExportTesting/DaveTestTemplate.ods");
  boost::filesystem::path c("/home/tanderson/temp/DaveTest.ods");
  
  boost::filesystem::copy_file(o, c, boost::filesystem::copy_option::overwrite_if_exists);
  
  libzippp::ZipArchive zip(c.string());
  zip.open(libzippp::ZipArchive::WRITE);
  for(const auto &e : zip.getEntries())
  {
    if (e.getName() == "content.xml")
    {
      std::string contents = e.readAsText();
      boost::filesystem::path temp(boost::filesystem::temp_directory_path());
      temp /= "content.xml";
      
      lbo::Map map = 
      {
        (std::make_pair(std::make_pair(0, 0), std::to_string(stripData.quoteNumber))),
        (std::make_pair(std::make_pair(1, 0), stripData.customerName.toStdString())),
        (std::make_pair(std::make_pair(2, 0), gu::idToString(stripData.customerId))),
        (std::make_pair(std::make_pair(3, 0), stripData.partName.toStdString())),
        (std::make_pair(std::make_pair(4, 0), stripData.partNumber.toStdString())),
        (std::make_pair(std::make_pair(5, 0), stripData.partSetup.toStdString())),
        (std::make_pair(std::make_pair(6, 0), stripData.partRevision.toStdString())),
        (std::make_pair(std::make_pair(7, 0), stripData.materialType.toStdString())),
        (std::make_pair(std::make_pair(8, 0), std::to_string(stripData.materialThickness))),
        (std::make_pair(std::make_pair(9, 0), stripData.processType.toStdString())),
        (std::make_pair(std::make_pair(10, 0), std::to_string(stripData.annualVolume))),
        (std::make_pair(std::make_pair(11, 0), std::to_string(stripData.stations.size()))),
        (std::make_pair(std::make_pair(12, 0), std::to_string(pitch->getValue()))),
        (std::make_pair(std::make_pair(13, 0), "200")), //todo. for stock width
        (std::make_pair(std::make_pair(14, 0), std::to_string(pitch->getValue() * 200))), //todo. update for stock width
        (std::make_pair(std::make_pair(15, 0), "1500")), //todo. for die length
        (std::make_pair(std::make_pair(16, 0), "700")), //todo. for die width
        (std::make_pair(std::make_pair(17, 0), "25")), //todo for upper travel
        (std::make_pair(std::make_pair(18, 0), "25")) //todo for lower travel
      };
      int index = 19;
      for (const auto &s : stripData.stations)
      {
        map.insert(std::make_pair(std::make_pair(index, 0), s.toStdString()));
        index++;
      }
      lbo::replace(contents, map);
      
      fstream stream;
      stream.open(temp.string(), std::ios::out);
      stream << contents;
      stream.close();
      
      zip.addFile(e.getName(), temp.string());
      break; //might invalidate, so get out.
    }
  }
  
  //doing a separate loop to ensure that we don't invalidate with addFile.
  for(const auto &e : zip.getEntries())
  {
//     std::cout << e.getName() << std::endl;
    
    boost::filesystem::path cPath = e.getName();
    
//     std::cout << "parent path: " << cPath.parent_path()
//     << "      extension: " << cPath.extension() << std::endl;
    
    if (cPath.parent_path() == "Pictures" && cPath.extension() == ".png")
    {
      std::cout << e.getName() << std::endl;
      zip.addFile(e.getName(), "/home/tanderson/temp/picture.png");
    }
  }
  
  zip.close();
}

void Strip::serialWrite(const QDir &)
{

}

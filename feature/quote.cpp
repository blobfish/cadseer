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

#include <libzippp.h>

#include <globalutilities.h>
#include <tools/occtools.h>
#include <application/application.h>
#include <project/project.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <tools/idtools.h>
#include <libreoffice/odshack.h>
#include <feature/strip.h>
#include <feature/dieset.h>
#include <annex/seershape.h>
#include <project/serial/xsdcxxoutput/featurequote.h>
#include <feature/quote.h>

using namespace ftr;
using boost::filesystem::path;

QIcon Quote::icon;

Quote::Quote() : Base(),
tFile("Template File", prf::manager().rootPtr->features().quote().get().templateSheet(), prm::PathType::Read),
oFile("Output File",  path(static_cast<app::Application*>(qApp)->getProject()->getSaveDirectory()) /= "Quote.ods", prm::PathType::Write)
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionQuote.svg");
  
  name = QObject::tr("Quote");
  mainSwitch->setUserValue(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  quoteData.quoteNumber = 0;
  quoteData.customerName = "aCustomer";
  quoteData.customerId = gu::createRandomId();
  quoteData.partName = "aName";
  quoteData.partNumber = "aNumber";
  quoteData.partSetup = "aSetup";
  quoteData.partRevision = "aRevision";
  quoteData.materialType = "aMaterialType";
  quoteData.materialThickness = 0.0;
  quoteData.processType = "aProcessType";
  quoteData.annualVolume = 0;
  
  parameterVector.push_back(&tFile);
  parameterVector.push_back(&oFile);
  
  tLabel = new lbr::PLabel(&tFile);
  tLabel->showName = true;
  tLabel->valueHasChanged();
  overlaySwitch->addChild(tLabel.get());
  
  oLabel = new lbr::PLabel(&oFile);
  oLabel->showName = true;
  oLabel->valueHasChanged();
  overlaySwitch->addChild(oLabel.get());
}

Quote::~Quote()
{

}

void Quote::updateModel(const UpdatePayload &payloadIn)
{
  setFailure();
  lastUpdateLog.clear();
  try
  {
    if (payloadIn.updateMap.count(strip) != 1)
      throw std::runtime_error("couldn't find 'strip' input");
    const Strip *sf = dynamic_cast<const Strip*>(payloadIn.updateMap.equal_range(strip).first->second);
    assert(sf);
    if(!sf)
      throw std::runtime_error("can not cast to strip feature");
    
    if (payloadIn.updateMap.count(dieSet) != 1)
      throw std::runtime_error("couldn't find 'dieset' input");
    const DieSet *dsf = dynamic_cast<const DieSet*>(payloadIn.updateMap.equal_range(dieSet).first->second);
    assert(dsf);
    if (!dsf)
      throw std::runtime_error("can not cast to dieset feature");
    
    //place labels
    const ann::SeerShape &dss = dsf->getAnnex<ann::SeerShape>(ann::Type::SeerShape); //part seer shape.
    const TopoDS_Shape &ds = dss.getRootOCCTShape(); //part shape.
    occt::BoundingBox sbbox(ds); //blank bounding box.
    osg::Vec3d tLoc = gu::toOsg(sbbox.getCenter()) + osg::Vec3d(0.0, 50.0, 0.0);
    tLabel->setMatrix(osg::Matrixd::translate(tLoc));
    osg::Vec3d oLoc = gu::toOsg(sbbox.getCenter()) + osg::Vec3d(0.0, -50.0, 0.0);
    oLabel->setMatrix(osg::Matrixd::translate(oLoc));
    
    if (!boost::filesystem::exists(static_cast<path>(tFile)))
      throw std::runtime_error("template file doesn't exist");
    /* we change the entries in a specific sheet and these values are linked into
    * a customer designed sheet. The links are not updated when we open the sheet in calc by default.
    * calc menu: /tools/options/libreoffice calc/formula/Recalculation on file load
    * set that to always.
    */
    boost::filesystem::copy_file
    (
      static_cast<path>(tFile),
      static_cast<path>(oFile),
      boost::filesystem::copy_option::overwrite_if_exists
    );
    
    libzippp::ZipArchive zip(static_cast<path>(oFile).string());
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
          (std::make_pair(std::make_pair(0, 0), std::to_string(quoteData.quoteNumber))),
          (std::make_pair(std::make_pair(1, 0), quoteData.customerName.toStdString())),
          (std::make_pair(std::make_pair(2, 0), gu::idToString(quoteData.customerId))),
          (std::make_pair(std::make_pair(3, 0), quoteData.partName.toStdString())),
          (std::make_pair(std::make_pair(4, 0), quoteData.partNumber.toStdString())),
          (std::make_pair(std::make_pair(5, 0), quoteData.partSetup.toStdString())),
          (std::make_pair(std::make_pair(6, 0), quoteData.partRevision.toStdString())),
          (std::make_pair(std::make_pair(7, 0), quoteData.materialType.toStdString())),
          (std::make_pair(std::make_pair(8, 0), std::to_string(quoteData.materialThickness))),
          (std::make_pair(std::make_pair(9, 0), quoteData.processType.toStdString())),
          (std::make_pair(std::make_pair(10, 0), std::to_string(quoteData.annualVolume))),
          (std::make_pair(std::make_pair(11, 0), std::to_string(sf->stations.size()))),
          (std::make_pair(std::make_pair(12, 0), std::to_string(sf->getPitch()))),
          (std::make_pair(std::make_pair(13, 0), std::to_string(sf->getWidth()))),
          (std::make_pair(std::make_pair(14, 0), std::to_string(sf->getPitch() * sf->getWidth()))),
          (std::make_pair(std::make_pair(15, 0), std::to_string(dsf->getLength()))),
          (std::make_pair(std::make_pair(16, 0), std::to_string(dsf->getWidth()))),
          (std::make_pair(std::make_pair(17, 0), std::to_string(sf->getHeight() + 25.0))),
          (std::make_pair(std::make_pair(18, 0), std::to_string(sf->getHeight() + 25.0)))
        };
        int index = 19;
        for (const auto &s : sf->stations)
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
        if (boost::filesystem::exists(pFile))
          zip.addFile(e.getName(), pFile.string());
      }
    }
    
    zip.close();
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in Quote update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in Quote update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in Quote update. " << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Quote::serialWrite(const QDir &dIn)
{
  prj::srl::FeatureQuote qo
  (
    Base::serialOut(),
    tFile.serialOut(),
    oFile.serialOut(),
    pFile.string(),
    quoteData.quoteNumber,
    quoteData.customerName.toStdString(),
    gu::idToString(quoteData.customerId),
    quoteData.partName.toStdString(),
    quoteData.partNumber.toStdString(),
    quoteData.partSetup.toStdString(),
    quoteData.partRevision.toStdString(),
    quoteData.materialType.toStdString(),
    quoteData.materialThickness,
    quoteData.processType.toStdString(),
    quoteData.annualVolume,
    tLabel->serialOut(),
    oLabel->serialOut()
  );
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).toUtf8().constData());
  prj::srl::quote(stream, qo, infoMap);
}

void Quote::serialRead(const prj::srl::FeatureQuote &qIn)
{
  Base::serialIn(qIn.featureBase());
  tFile.serialIn(qIn.templateFile());
  oFile.serialIn(qIn.outFile());
  pFile = qIn.pictureFile();
  quoteData.quoteNumber = qIn.quoteNumber();
  quoteData.customerName = QString::fromStdString(qIn.customerName());
  quoteData.customerId = gu::stringToId(qIn.customerId());
  quoteData.partName = QString::fromStdString(qIn.partName());
  quoteData.partNumber = QString::fromStdString(qIn.partNumber());
  quoteData.partSetup = QString::fromStdString(qIn.partSetup());
  quoteData.partRevision = QString::fromStdString(qIn.partRevision());
  quoteData.materialType = QString::fromStdString(qIn.materialType());
  quoteData.materialThickness = qIn.materialThickness();
  quoteData.processType = QString::fromStdString(qIn.processType());
  quoteData.annualVolume = qIn.annualVolume();
  tLabel->serialIn(qIn.tLabel());
  oLabel->serialIn(qIn.oLabel());
}

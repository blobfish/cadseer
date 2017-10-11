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
#include <tools/idtools.h>
#include <libreoffice/odshack.h>
#include <feature/strip.h>
#include <feature/dieset.h>
#include <feature/quote.h>

using namespace ftr;

QIcon Quote::icon;

Quote::Quote()
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
}

Quote::~Quote()
{

}

void Quote::updateModel(const UpdatePayload &payloadIn)
{
  setFailure();
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
    
    
    if (!boost::filesystem::exists(tFile))
      throw std::runtime_error("template file doesn't exist");
    /* we change the entries in a specific sheet and these values are linked into
    * a customer designed sheet. The links are not updated when we open the sheet in calc by default.
    * calc menu: /tools/options/libreoffice calc/formula/Recalculation on file load
    * set that to always.
    */
    boost::filesystem::copy_file(tFile, oFile, boost::filesystem::copy_option::overwrite_if_exists);
    
    libzippp::ZipArchive zip(oFile.string());
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
    std::cout << std::endl << "Error in Quote update. " << e.GetMessageString() << std::endl;
  }
  catch (std::exception &e)
  {
    std::cout << std::endl << "Error in Quote update. " << e.what() << std::endl;
  }
  catch (...)
  {
    std::cout << std::endl << "Unknown error in Quote update. " << std::endl;
  }
  setModelClean();
}

void Quote::serialWrite(const QDir&)
{
}

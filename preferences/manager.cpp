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

#include <iostream>
#include <fstream>

#include <application/application.h>
#include <QDir>

#include <preferences/preferencesXML.h>
#include <preferences/manager.h>

//xml, xsd validation website.
//http://www.utilities-online.info/xsdvalidation/#.Vf2u9ZOVvts

//generate parsing files.
//xsdcxx cxx-tree --std c++11 --generate-serialization --generate-doxygen --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --namespace-map =prf  preferencesXML.xsd

//ok this is screwy. the parser, by default, validates the xml so it
//needs the xsd at runtime. We have the xsd and a default xml in
//the qt controlled resources. setup() creates these files in the temp
//directory if they don't already exist.


using namespace prf;

prf::Manager& prf::manager()
{
  static Manager localManager;
  return localManager;
}

Manager::Manager()
{
  ok = false;
  
  appDirectory = static_cast<app::Application*>(qApp)->getApplicationDirectory();
  fileNameXML = "preferences.xml";
  filePathXML = appDirectory.absolutePath() + QDir::separator() + fileNameXML;
  filePathXSD = appDirectory.absolutePath() + QDir::separator() + "preferences.xsd";
  if (!appDirectory.exists(fileNameXML))
  {
    if (!createDefault())
      return;
  }
  
  if (!readConfig())
  {
    //config is bad. backup and copy default.
    QString backUpPath = filePathXML + ".backup";
    if (appDirectory.exists(backUpPath))
      appDirectory.remove(backUpPath);
    appDirectory.rename(filePathXML, backUpPath);
    if (!createDefault())
      return;
    
    //try to read the newly created default
    if (!readConfig())
      return;
  }
    
  assert(rootPtr);
  ok = true;
  
//   std::cout << std::endl <<
//   "linear deflection is: " << rootPtr->visual().mesh().linearDeflection() <<
//   "      angularDeflection is: " << rootPtr->visual().mesh().angularDeflection() << std::endl;
}

bool Manager::createDefault()
{
  //xml
  QString xmlResourceName(":/preferences/preferences.xml");
  QFile resourceFileXML(xmlResourceName);
  if (!resourceFileXML.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    std::cerr << "couldn't open resource file: " << xmlResourceName.toStdString().c_str() << std::endl;
    return false;
  }
  QByteArray bufferXML = resourceFileXML.readAll();
  resourceFileXML.close();
  QFile newFileXML(filePathXML);
  if (!newFileXML.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
  {
    std::cerr << "couldn't open new file: " << filePathXML.toStdString().c_str()  << std::endl;
    return false;
  }
  newFileXML.write(bufferXML);
  newFileXML.close();
  
  //xsd. just over write it.
  QString xsdResourceName(":/preferences/preferencesXML.xsd");
  QFile resourceFileXSD(xsdResourceName);
  if (!resourceFileXSD.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    std::cerr << "couldn't open resource file: " << xsdResourceName.toStdString().c_str() << std::endl;
    return false;
  }
  QByteArray bufferXSD = resourceFileXSD.readAll();
  resourceFileXSD.close();
    
  QFile newFileXSD(filePathXSD);
  if (!newFileXSD.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
  {
    std::cerr << "couldn't open new file" << filePathXSD.toStdString().c_str()  << std::endl;
    return false;
  }
  newFileXSD.write(bufferXSD);
  newFileXSD.close();
  
  return true;
}


void Manager::saveConfig()
{
  //a very simple test has passed.
  xml_schema::NamespaceInfomap map;
  std::ofstream stream(filePathXML.toStdString().c_str());
  root(stream, *rootPtr, map);
}

bool Manager::readConfig()
{
  try
  {
    xml_schema::Properties props;
    props.no_namespace_schema_location (filePathXSD.toStdString());
    props.schema_location ("http://www.w3.org/XML/1998/namespace", "xml.xsd");
    
    auto tempPtr(root(filePathXML.toStdString(), 0, props));
    rootPtr = std::move(tempPtr);
    
    return true;
  }
  catch (const xsd::cxx::xml::invalid_utf16_string&)
  {
    std::cerr << "invalid UTF-16 text in DOM model" << std::endl;
  }
  catch (const xsd::cxx::xml::invalid_utf8_string&)
  {
    std::cerr << "invalid UTF-8 text in object model" << std::endl;
  }
  catch (const xml_schema::Exception& e)
  {
    std::cerr << e << std::endl;
  }
  
  return false;
}


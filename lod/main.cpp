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

/* this is a program for generating lods */

#include <iostream>
#include <cassert>
#include <string>
#include <chrono>
#include <thread>

#include <boost/filesystem.hpp>
#include <boost/uuid/uuid.hpp>

#include <Precision.hxx>
#include <TopoDS_Shape.hxx>
#include <BinTools.hxx>

#include <osg/Switch>
#include <osgDB/WriteFile>

#include <tools/occtools.h>
#include <annex/shapeidhelper.h>
#include <modelviz/shapegeometry.h>

namespace bfs = boost::filesystem;
using boost::uuids::uuid;

int main(int /*argc*/, char ** /*argv*/)
{
  std::vector<std::string> lines;
  for(;;)
  {
    try
    {
      std::string line;
      while(std::getline(std::cin, line))
      {
        if (!line.empty())
          lines.push_back(line);
  //       std::cerr << "child process line: " << line << std::endl;
        if (lines.size() == 7)
          break;
      }
      if (lines.size() != 7)
      {
        std::cout << "FAIL lod generator: Wrong number of lines" << std::endl;
        lines.clear();
        continue;
      }
      assert(lines.at(0) == "BEGIN");
      if (lines.at(0) != "BEGIN")
      {
        std::cout << "FAIL lod generator: First line is NOT 'BEGIN'" << std::endl;
        lines.clear();
        continue;
      }
      bfs::path occtPath = bfs::path(lines.at(1)); assert(bfs::exists(occtPath));
      bfs::path osgPath = bfs::path(lines.at(2));
      bfs::path idsPath = bfs::path(lines.at(3)); assert(bfs::exists(idsPath));
      double linear = std::stod(lines.at(4)); assert(linear > Precision::Confusion());
      double angular = std::stod(lines.at(5)); assert(angular > Precision::Confusion());
      assert(lines.at(6) == "END");
      if (lines.at(6) != "END")
      {
        std::cout << "FAIL lod generator: Last line is NOT 'END'" << std::endl;
        lines.clear();
        continue;
      }
      lines.clear();
      
  //     std::cerr << "got 7 parameters in child" << std::endl;
    
      TopoDS_Shape fileShape;
      bool result = BinTools::Read(fileShape, occtPath.string().c_str());
      if (!result)
      {
        std::cout << "FAIL lod generator: reading of occt file" << std::endl;
        lines.clear();
        continue;
      }
      occt::ShapeVector shapes = occt::mapShapes(fileShape);
      
      std::vector<uuid>ids = ann::ShapeIdHelper::read(idsPath);
      assert(shapes.size() == ids.size());
      ann::ShapeIdHelper helper;
      std::size_t index = 0;
      for (const auto &id : ids)
      {
        helper.add(id, shapes.at(index));
        index++;
      }
      
      mdv::ShapeGeometryBuilder sBuilder(fileShape, helper);
      sBuilder.go(linear, angular);
      if (!sBuilder.success)
      {
        std::cout << "FAIL lod generator: builder failed" << std::endl;
        lines.clear();
        continue;
      }
      if (!osgDB::writeNodeFile(*sBuilder.out, osgPath.string()))
      {
        std::cout << "FAIL lod generator: writing node file" << std::endl;
        lines.clear();
        continue;
      }
      
      std::cout << "SUCCESS lod generator" << std::endl;
    }
    catch(...)
    {
      std::cout << "FAIL lod generator: caught unknown error" << std::endl;
      lines.clear();
      continue;
    }
  }
}

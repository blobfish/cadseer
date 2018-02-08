/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef PRJ_FEATURELOAD_H
#define PRJ_FEATURELOAD_H

#include <memory>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include <Standard_StdAllocator.hxx>

class TopoDS_Shape;

namespace ftr{class Base;}

namespace prj
{
  class FeatureLoad
  {
  public:
    FeatureLoad(const std::string &directoryIn, const TopoDS_Shape&);
    ~FeatureLoad();
    std::shared_ptr<ftr::Base> load(const std::string &idIn, const std::string &typeIn, std::size_t shapeOffsetIn);
  private:
    std::string directory; //!< expects trailing directory separator '/'
    std::string fileExtension;
    std::vector<TopoDS_Shape, Standard_StdAllocator<TopoDS_Shape> > shapeVector;
    
    typedef std::function<std::shared_ptr<ftr::Base> (const std::string &, std::size_t)> LoadFunction;
    typedef std::map<std::string, LoadFunction> FunctionMap;
    FunctionMap functionMap;
    
    std::shared_ptr<ftr::Base> loadBox(const std::string &, std::size_t);
    std::shared_ptr<ftr::Base> loadCylinder(const std::string &, std::size_t);
    std::shared_ptr<ftr::Base> loadSphere(const std::string &, std::size_t);
    std::shared_ptr<ftr::Base> loadCone(const std::string &, std::size_t);
    std::shared_ptr<ftr::Base> loadUnion(const std::string &, std::size_t);
    std::shared_ptr<ftr::Base> loadIntersect(const std::string &, std::size_t);
    std::shared_ptr<ftr::Base> loadSubtract(const std::string &, std::size_t);
    std::shared_ptr<ftr::Base> loadInert(const std::string &, std::size_t);
    std::shared_ptr<ftr::Base> loadBlend(const std::string &, std::size_t);
    std::shared_ptr<ftr::Base> loadChamfer(const std::string &, std::size_t);
    std::shared_ptr<ftr::Base> loadDraft(const std::string &, std::size_t);
    std::shared_ptr<ftr::Base> loadDatumPlane(const std::string &, std::size_t);
    std::shared_ptr<ftr::Base> loadHollow(const std::string &, std::size_t);
    std::shared_ptr<ftr::Base> loadOblong(const std::string &, std::size_t);
    std::shared_ptr<ftr::Base> loadExtract(const std::string &, std::size_t);
    std::shared_ptr<ftr::Base> loadSquash(const std::string &, std::size_t);
    std::shared_ptr<ftr::Base> loadNest(const std::string &, std::size_t);
    std::shared_ptr<ftr::Base> loadDieSet(const std::string &, std::size_t);
    std::shared_ptr<ftr::Base> loadStrip(const std::string &, std::size_t);
    std::shared_ptr<ftr::Base> loadQuote(const std::string &, std::size_t);
    std::shared_ptr<ftr::Base> loadRefine(const std::string &, std::size_t);
    std::shared_ptr<ftr::Base> loadInstanceLinear(const std::string &, std::size_t);
    std::shared_ptr<ftr::Base> loadInstanceMirror(const std::string &, std::size_t);
    std::shared_ptr<ftr::Base> loadInstancePolar(const std::string &, std::size_t);
    std::shared_ptr<ftr::Base> loadOffset(const std::string &, std::size_t);
    std::shared_ptr<ftr::Base> loadThicken(const std::string &, std::size_t);
    std::shared_ptr<ftr::Base> loadSew(const std::string &, std::size_t);
  };
}

#endif // PRJ_FEATURELOAD_H

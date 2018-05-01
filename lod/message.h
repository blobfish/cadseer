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
#ifndef LOD_MESSAGE_H
#define LOD_MESSAGE_H

#include <set>
#include <map>

#include <boost/filesystem/path.hpp>
#include <boost/uuid/uuid.hpp>

namespace lod
{
//   /**
//   * @brief status of processing
//   * 
//   * used in child process.
//   */

//   
//   enum class ErrorCode
//   {
//     OK, //!< No problems detected with generation.
//     Error //!< place holder. expand later.
//   };
//   
//   const static std::map<ErrorCode, std::string> errorCodeStrings
//   {
//     std::make_pair(ErrorCode::OK, "OK"),
//     std::make_pair(ErrorCode::Error, "Error")
//   };
//   
//   /**
//   * @brief Input parameters for generation of one lod entry.
//   */
//   struct Parameter
//   {
//     Parameter(const boost::filesystem::path& pIn) : filePath(pIn){}
//     Parameter(const boost::filesystem::path& pIn, double lIn, double aIn) :
//       filePath(pIn),
//       linear(lIn),
//       angular(aIn)
//     {}
//     boost::filesystem::path filePath; //!< location and name of output file.
//     double linear; //!< the linear deflection.
//     double angular; //!< the angular deflection.
//     ErrorCode eCode = ErrorCode::OK;
//     Status status = Status::Queued;
//   };
  
  /**
  * @brief message to generate all lods for a shape.
  */
  struct Message
  {
    Message(){}
    Message
    (
      const boost::uuids::uuid &featureIdIn,
      const boost::filesystem::path &occtIn,
      const boost::filesystem::path &osgIn,
      const boost::filesystem::path &idsIn,
      double linearIn,
      double angularIn,
      double rangeMinIn,
      double rangeMaxIn
    ):
    featureId(featureIdIn),
    filePathOCCT(occtIn),
    filePathOSG(osgIn),
    filePathIds(idsIn),
    linear(linearIn),
    angular(angularIn),
    rangeMin(rangeMinIn),
    rangeMax(rangeMaxIn)
    {}
    
    boost::uuids::uuid featureId;
    boost::filesystem::path filePathOCCT; //binary occt file containing input shape.
    boost::filesystem::path filePathOSG; //binary osg file containing output.
    boost::filesystem::path filePathIds; //file containing ids.
    double linear; //!< the linear deflection.
    double angular; //!< the angular deflection.
    double rangeMin; //!< minimum range
    double rangeMax; //!< maximum range
  };
}

#endif // LOD_MESSAGE_H

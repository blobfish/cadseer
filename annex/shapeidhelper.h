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

#ifndef ANN_SHAPEIDHELPER_H
#define ANN_SHAPEIDHELPER_H

#include <vector>

#include <boost/optional.hpp>
#include <boost/uuid/uuid.hpp>

#include <tools/occtools.h>

namespace boost{namespace filesystem{class path;}}

namespace ann
{
  /**
  * @brief matching shapes to their ids.
  * 
  * This is lightweight matching used for osg viz
  * generation for both internal and external programs.
  */
  class ShapeIdHelper
  {
  public:
    void add(const boost::uuids::uuid&, const TopoDS_Shape&);
    boost::optional<boost::uuids::uuid> find(const TopoDS_Shape&) const;
    boost::optional<const TopoDS_Shape&> find(const boost::uuids::uuid&) const;
    void write(const boost::filesystem::path&);
    static std::vector<boost::uuids::uuid> read(const boost::filesystem::path&);
  private:
    //@{
    //! parallel vectors. matches at offsets.
    std::vector<boost::uuids::uuid> ids;
    occt::ShapeVector shapes;
    //@}
    friend std::ostream& operator<<(std::ostream&, const ShapeIdHelper&);
  };
  
  std::ostream& operator<<(std::ostream&, const ShapeIdHelper&);
}

#endif // ANN_SHAPEIDHELPER_H

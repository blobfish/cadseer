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

#ifndef FTR_PICK_H
#define FTR_PICK_H

#include <vector>
#include <utility>

#include <osg/Vec3d>

class TopoDS_Edge;
class TopoDS_Face;

namespace prj{namespace srl{class Pick; class Picks;}}

namespace ftr
{
  //! Selection data for features.
  struct Pick
  {
    Pick();
    Pick(const boost::uuids::uuid&, double, double);
    boost::uuids::uuid id; //!< id of edge or face.
    double u; //!< u parameter on edge or face
    double v;//!< v parameter on face
    
    bool operator==(const Pick&) const;
    void setParameter(const TopoDS_Edge&, const osg::Vec3d&);
    void setParameter(const TopoDS_Face&, const osg::Vec3d&);
    osg::Vec3d getPoint(const TopoDS_Edge&) const;
    osg::Vec3d getPoint(const TopoDS_Face&) const;
    
    prj::srl::Pick serialOut() const;
    void serialIn(const prj::srl::Pick&);
    
    static double parameter(const TopoDS_Edge&, const osg::Vec3d&);
    static std::pair<double, double> parameter(const TopoDS_Face&, const osg::Vec3d&);
    
    static osg::Vec3d point(const TopoDS_Edge&, double);
    static osg::Vec3d point(const TopoDS_Face&, double, double);
  };
  
  typedef std::vector<Pick> Picks;
  prj::srl::Picks serialOut(const Picks&);
  Picks serialIn(const prj::srl::Picks&);
}

#endif // FTR_PICK_H
